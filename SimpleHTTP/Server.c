/* ************************************************************************
> File Name:     Server.c
> Author:        niu0217
> Created Time:  Sun 30 Jun 2024 10:18:26 AM CST
> Description:   
 ************************************************************************/

#include "Server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>  // EAGAIN
#include <strings.h> // strcasecmp
#include <string.h>
#include <sys/stat.h> // stat
#include <assert.h>
#include <sys/sendfile.h> //sendfile 高效发送文件
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

struct FdInfo
{
  int fd;
  int epfd;
  pthread_t tid;
};

int initListenFd(unsigned short port)
{
  // 1、创建socket
  int listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == listenFd)
  {
    perror("socket");
    return -1;
  }

  // 2、设置端口复用
  int opt = 1;
  int ret = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  if (-1 == ret)
  {
    perror("setsockopt");
    return -1;
  }

  // 3、绑定
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  ret = bind(listenFd, (struct sockaddr*)&addr, sizeof addr);
  if (-1 == ret)
  {
    perror("bind");
    return -1;
  }

  // 4、监听
  ret = listen(listenFd, 128);
  if (-1 == ret)
  {
    perror("listen");
    return -1;
  }

  // 5、返回
  return listenFd;
}

int epollRun(int listenFd)
{
  // 1、创建epoll实例
  int epfd = epoll_create(1);
  if (-1 == epfd)
  {
    perror("epoll_create");
    return -1;
  }

  // 2、listenFd 上树
  struct epoll_event ev;
  ev.data.fd = listenFd;
  ev.events = EPOLLIN;
  int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenFd, &ev);
  if (-1 == ret)
  {
    perror("epoll_ctl");
    return -1;
  }

  // 3、检测
  struct epoll_event evs[1024];
  int size = sizeof(evs) / sizeof(struct epoll_event);
  while (1)
  {
    int num = epoll_wait(epfd, evs, size, -1);
    for (int i = 0; i < num; i++)
    {
      struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
      int fd = evs[i].data.fd;
      info->epfd = epfd;
      info->fd = fd;
      if (fd == listenFd)
      {
        pthread_create(&info->tid, NULL, acceptClient, info);
        pthread_detach(info->tid);
      }
      else
      {
        pthread_create(&info->tid, NULL, recvHttpRequest, info);
        pthread_detach(info->tid);
      }
    }
  }
  return 0;
}

void* acceptClient(void *arg)
{
  struct FdInfo* info = (struct FdInfo*)arg;

  // 1、建立连接
  int connFd = accept(info->fd, NULL, NULL);
  if (-1 == connFd)
  {
    perror("accept");
    return NULL;
  }

  // 2、设置文件描述符非阻塞
  int flag = fcntl(connFd, F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(connFd, F_SETFL, flag);

  // 3、将已连接的客户描述符上树，添加到epfd中
  struct epoll_event ev;
  ev.data.fd = connFd;
  ev.events = EPOLLIN | EPOLLET;   // 在这里 connFd 已经被设置为 ET + 非阻塞
  int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, connFd, &ev);
  if (-1 == ret)
  {
    perror("epoll_ctl");
    return NULL;
  }

  printf("acceptcliet threadId: %ld\n", info->tid);
  free(info);
  return NULL;
}

void* recvHttpRequest(void* arg)
{
  struct FdInfo* info = (struct FdInfo*)arg;
  printf("开始接收客户端数据了...\n");
  printf("recvMsg threadId: %ld\n", info->tid);
  int len = 0;
  int hasReadidx = 0;
  char tmp[1024] = { 0 };
  char buf[4096] = { 0 };  // 用于存放客户端发送过来的全部信息
  while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
  {
    if (hasReadidx + len < sizeof buf)
    {
      memcpy(buf + hasReadidx, tmp, len);
    }
    hasReadidx += len;
  }
  if (-1 == len && errno == EAGAIN)  // 数据接收完毕
  {
    // 解析请求行
    char* pt = strstr(buf, "\r\n");
    int reqLen = pt - buf;
    buf[reqLen] = '\0';
    parseRequestLine(buf, info->fd);
  }
  else if (len == 0)
  {
    // 客户端断开了连接
    epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
    close(info->fd);
  }
  else
  {
    perror("recv");
  }
  free(info);
  return NULL;
}

int parseRequestLine(const char* line, int connFd)
{
  // 解析请求行 get /xxx/1.jpg http/1.1
  char method[12];
  char path[1024];
  sscanf(line, "%[^ ] %[^ ]", method, path);
  decodeMsg(path, path); // 解析特殊字符
  printf("method: %s, path: %s\n\n", method, path);
  if (strcasecmp(method, "get") != 0)  // 不区分大小写进行对比
  {
    // 目前只处理 get 请求
    return -1;
  }

  // 处理客户端请求的静态资源（目录或者文件）
  char *file = NULL;
  if (strcmp(path, "/") == 0)
  {
    file = "./";
  }
  else
  {
    file = path + 1;
  }
  // 获取文件属性
  struct stat st;
  int ret = stat(file, &st);
  if (ret == -1)
  {
    // 文件不存在 -- 回复404
    sendHeadMsg(connFd, 404, "Not Found", getFileType(".html"), -1);
    sendFile("404.html", connFd);
    return 0;
  }
  if (S_ISDIR(st.st_mode))
  {
    // 是一个目录 把这个目录中的内容发送给客户端
    sendHeadMsg(connFd, 200, "OK", getFileType(".html"), -1);
    sendDir(file, connFd);
  }
  else
  {
    // 是一个文件 把文件的内容发送给客户端
    sendHeadMsg(connFd, 200, "OK", getFileType(file), st.st_size);
    sendFile(file, connFd);
  }
  return 0;
}

int sendFile(const char* fileName, int connFd)
{
  int fd = open(fileName, O_RDONLY);
  assert(fd > 0);
#if 0
  while (1)
  {
    char buf[1024];
    int len = read(fd, buf, sizeof buf);
    if (len > 0)
    {
      send(connFd, buf, len, 0);
      usleep(10); // 非常重要，避免发送速度过快 接收端处理不过来
    }
    else if (len == 0)
    {
      // 读取文件数据完毕
      break;
    }
    else
    {
      perror("read");
    }
  }
#else
  off_t offset = 0;
  int size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  while (offset < size)  // 用while是为了处理大文件的发送
  {
    // sendfile的第三个参数：
    //   发送数据之前 根据该偏移量开始读文件数据
    //   发送数据之后 更新该偏移量
    int ret = sendfile(connFd, fd, &offset, size - offset);
    printf("ret value: %d\n", ret);
    if (ret == -1 && errno == EAGAIN)
    {
      printf("客户端正在处理，稍等片刻...\n");
      usleep(100);  // 客户端可能处理的比较慢，需要等一等
      continue;
    }
  }
#endif
  close(fd);
  return 0;
}

int sendHeadMsg(int connFd, int status, const char* descr, const char* type, int length)
{
  // 状态行
  char buf[4096] = { 0 };
  sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
  // 响应头
  sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
  sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

  send(connFd, buf, strlen(buf), 0);  // 健壮性判断
  return 0; 
}

// https://tool.oschina.net/commons
const char* getFileType(const char* name)
{
  // a.jpg a.mp4 a.html
  // 自右向左查找‘.’字符, 如不存在返回NULL
  const char* dot = strrchr(name, '.');
  if (dot == NULL)
    return "text/plain; charset=utf-8";	// 纯文本
  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    return "text/html; charset=utf-8";
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(dot, ".gif") == 0)
    return "image/gif";
  if (strcmp(dot, ".png") == 0)
    return "image/png";
  if (strcmp(dot, ".css") == 0)
    return "text/css";
  if (strcmp(dot, ".au") == 0)
    return "audio/basic";
  if (strcmp(dot, ".wav") == 0)
    return "audio/wav";
  if (strcmp(dot, ".avi") == 0)
    return "video/x-msvideo";
  if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    return "video/quicktime";
  if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
    return "video/mpeg";
  if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
    return "model/vrml";
  if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
    return "audio/midi";
  if (strcmp(dot, ".mp3") == 0)
    return "audio/mpeg";
  if (strcmp(dot, ".mp4") == 0)
    return "video/mpeg4";
  if (strcmp(dot, ".ogg") == 0)
    return "application/ogg";
  if (strcmp(dot, ".pac") == 0)
    return "application/x-ns-proxy-autoconfig";

  return "text/plain; charset=utf-8";
}

/*
<html>
    <head>
        <title>test</title>
    </head>
    <body>
        <table>
            <tr>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td></td>
                <td></td>
            </tr>
        </table>
    </body>
</html>
*/
int sendDir(const char* dirName, int connFd)
{
  char buf[4096] = { 0 };
  sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
  struct dirent** namelist;
  int num = scandir(dirName, &namelist, NULL, alphasort);
  for (int i = 0; i < num; i++)
  {
    // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
    char* name = namelist[i]->d_name;
    struct stat st;
    char subPath[1024] = { 0 };
    sprintf(subPath, "%s/%s", dirName, name);
    stat(subPath, &st);
    if (S_ISDIR(st.st_mode))
    {
      // a标签 <a href="">name</a>
      sprintf(buf + strlen(buf), 
              "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", 
              name, name, st.st_size);
    }
    else
    {
      sprintf(buf + strlen(buf),
              "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
              name, name, st.st_size);
    }
    send(connFd, buf, strlen(buf), 0);
    memset(buf, 0, sizeof(buf));
    free(namelist[i]);  // 特别注意
  }
  sprintf(buf, "</table></body></html>");
  send(connFd, buf, strlen(buf), 0);
  free(namelist);
  return 0;
}

// 将字符转换为整形数
int hexToDec(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
  for (; *from != '\0'; ++to, ++from)
  {
    // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
    // Linux%E5%86%85%E6%A0%B8.jpg
    if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
    {
      // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
      // B2 == 178
      // 将3个字符, 变成了一个字符, 这个字符就是原始数据
      *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

      // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
      from += 2;
    }
    else
    {
      // 字符拷贝, 赋值
      *to = *from;
    }
  }
  *to = '\0';
}
