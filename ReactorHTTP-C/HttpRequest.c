/* ************************************************************************
> File Name:     HttpRequest.c
> Author:        niu0217
> Created Time:  Wed 03 Jul 2024 09:34:29 AM CST
> Description:   
 ************************************************************************/
#define _GNU_SOURCE

#include "HttpRequest.h"
#include "TcpConnection.h"
#include "Log.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#define HeaderSize 12

struct HttpRequest* httpRequestInit()
{
  struct HttpRequest *request = (struct HttpRequest*)
                                malloc(sizeof(struct HttpRequest));
  httpRequestReset(request);
  request->reqHeaders = (struct RequestHeader*)
                        malloc(HeaderSize * sizeof(struct RequestHeader));
  return request;
}

void httpRequestReset(struct HttpRequest* req)
{
  req->curState = ParseReqLine;
  req->method = NULL;
  req->url = NULL;
  req->version = NULL;
  req->reqHeadersNum = 0;
}

void httpRequestResetEx(struct HttpRequest* req)
{
  free(req->url);
  free(req->method);
  free(req->version);
  
  if (req->reqHeaders != NULL)
  {
    for (int i = 0; i < req->reqHeadersNum; ++i)
    {
      free(req->reqHeaders[i].key);
      free(req->reqHeaders[i].value);
    }
    free(req->reqHeaders);
  }
  httpRequestReset(req);
}

void httpRequestDestroy(struct HttpRequest* req)
{
  if (req != NULL)
  {
    httpRequestResetEx(req);
    free(req);
    req = NULL;
  }
}

enum HttpRequestState httpRequestState(struct HttpRequest* request)
{
  return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value)
{
  request->reqHeaders[request->reqHeadersNum].key = (char*)key;
  request->reqHeaders[request->reqHeadersNum].value = (char*)value;
  request->reqHeadersNum++;
}

char* httpRequestGetHeader(struct HttpRequest* request, const char* key)
{
  if (request)
  {
    for (int i = 0; i < request->reqHeadersNum; i++)
    {
      if (strncasecmp(request->reqHeaders[i].key,
                      key, strlen(key)) == 0)
      {
        return request->reqHeaders[i].value;
      }
    }
  }
  return NULL;
}

char* splitRequestLine(char* start, char* end, char* sub, char** ptr)
{
  char* space = end;
  if (sub != NULL)
  {
    space = memmem(start, end - start, sub, strlen(sub));
    assert(space != NULL);
  }
  int length = space - start;
  char* tmp = (char*)malloc(length + 1);
  strncpy(tmp, start, length);
  tmp[length] = '\0';
  *ptr = tmp;
  return space + 1;
}

bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf)
{
  // 读出请求行, 保存字符串结束地址
  char* end = bufferFindCRLF(readBuf);
  // 保存字符串起始地址
  char* start = readBuf->data + readBuf->readPos;
  // 请求行总长度
  int lineSize = end - start;

  if (lineSize)
  {
    start = splitRequestLine(start, end, " ", &request->method);
    start = splitRequestLine(start, end, " ", &request->url);
    splitRequestLine(start, end, NULL, &request->version);

    // 为解析请求头做准备
    readBuf->readPos += lineSize;
    readBuf->readPos += 2;
    // 修改状态
    request->curState = ParseReqHeaders;
    return true;
  }
  return false;
}

bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf)
{
  char* end = bufferFindCRLF(readBuf);
  if (end != NULL)
  {
    char* start = readBuf->data + readBuf->readPos;
    int lineSize = end - start;
    // 基于: 搜索字符串
    char* middle = memmem(start, lineSize, ": ", 2);
    if (middle != NULL)
    {
      char* key = malloc(middle - start + 1);
      strncpy(key, start, middle - start);
      key[middle - start] = '\0';

      char* value = malloc(end - middle - 2 + 1);
      strncpy(value, middle + 2, end - middle - 2);
      value[end - middle - 2] = '\0';

      httpRequestAddHeader(request, key, value);
      // 移动读数据的位置
      readBuf->readPos += lineSize;
      readBuf->readPos += 2;
    }
    else
    {
      // 请求头被解析完了, 跳过空行
      readBuf->readPos += 2;
      // 修改解析状态
      // 忽略 post 请求, 按照 get 请求处理
      request->curState = ParseReqDone;
    }
    return true;
  }
  return false;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
                      struct HttpResponse* response, struct Buffer* sendBuf, 
                      int socket)
{
  bool flag = true;
  while (request->curState != ParseReqDone)
  {
    switch (request->curState)
    {
    case ParseReqLine:
      flag = parseHttpRequestLine(request, readBuf);
      break;
    case ParseReqHeaders:
      flag = parseHttpRequestHeader(request, readBuf);
      break;
    case ParseReqBody:
      break;
    default:
      break;
    }
    if (!flag)
    {
      return flag;
    }
    // 判断是否解析完毕了, 如果完毕了, 需要准备回复的数据
    if (request->curState == ParseReqDone)
    {
      // 1. 根据解析出的原始数据, 对客户端的请求做出处理
      processHttpRequest(request, response);
      // 2. 组织响应数据并发送给客户端
      httpResponsePrepareMsg(response, sendBuf, socket);
    }
  }

  request->curState = ParseReqLine;   // 状态还原, 保证还能继续处理第二条及以后的请求
  return flag;
}

bool processHttpRequest(struct HttpRequest* request, 
                        struct HttpResponse* response)
{
  if (strcasecmp(request->method, "get") != 0)
  {
    return -1;
  }
  decodeMsg(request->url, request->url);
  // 处理客户端请求的静态资源(目录或者文件)
  char* file = NULL;
  if (strcmp(request->url, "/") == 0)
  {
    file = "./";
  }
  else
  {
    file = request->url + 1;
  }
  // 获取文件属性
  struct stat st;
  int ret = stat(file, &st);
  if (ret == -1)
  {
    // 文件不存在 -- 回复404
    strcpy(response->fileName, "404.html");
    response->statusCode = NotFound;
    strcpy(response->statusMsg, "Not Found");
    // 响应头
    httpResponseAddHeader(response, "Content-type", getFileType(".html"));
    response->sendDataFunc = sendFile;
    return 0;
  }

  strcpy(response->fileName, file);
  response->statusCode = OK;
  strcpy(response->statusMsg, "OK");
  // 判断文件类型
  if (S_ISDIR(st.st_mode))
  {
    // 把这个目录中的内容发送给客户端
    httpResponseAddHeader(response, "Content-type", getFileType(".html"));
    response->sendDataFunc = sendDir;
  }
  else
  {
    // 把文件的内容发送给客户端
    char tmp[12] = { 0 };
    sprintf(tmp, "%ld", st.st_size);
    httpResponseAddHeader(response, "Content-type", getFileType(file));
    httpResponseAddHeader(response, "Content-length", tmp);
    response->sendDataFunc = sendFile;
  }

  return false;
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
  if (strcmp(dot, ".ogg") == 0)
    return "application/ogg";
  if (strcmp(dot, ".pac") == 0)
    return "application/x-ns-proxy-autoconfig";

  return "text/plain; charset=utf-8";
}

void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
  char buf[4096] = { 0 };
  sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
  struct dirent** namelist;
  int num = scandir(dirName, &namelist, NULL, alphasort);
  for (int i = 0; i < num; ++i)
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
    bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd);
#endif
    memset(buf, 0, sizeof(buf));
    free(namelist[i]);
  }
  sprintf(buf, "</table></body></html>");
  bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
  bufferSendData(sendBuf, cfd);
#endif
  free(namelist);
}

void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd)
{
  // 1. 打开文件
  int fd = open(fileName, O_RDONLY);
  assert(fd > 0);

  while (1)
  {
    char buf[1024];
    int len = read(fd, buf, sizeof buf);
    if (len > 0)
    {
      bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
      bufferSendData(sendBuf, cfd);
#endif
    }
    else if (len == 0)
    {
      break;
    }
    else
    {
      close(fd);
      perror("read");
    }
  }
  close(fd);
}

