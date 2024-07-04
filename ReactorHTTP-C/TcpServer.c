/* ************************************************************************
> File Name:     TcpServer.c
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 04:47:20 PM CST
> Description:
 ************************************************************************/

#include "TcpServer.h"
#include "TcpConnection.h"
#include "Log.h"
#include <arpa/inet.h>
#include <stdio.h>  // perror
#include <stdlib.h>  // malloc

struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
  struct TcpServer* tcp = (struct TcpServer*)
                          malloc(sizeof(struct TcpServer));
  tcp->listener = listenerInit(port);
  tcp->mainLoop = eventLoopInit();
  tcp->threadNum = threadNum;
  tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
  return tcp;
}

struct Listener* listenerInit(unsigned short port)
{
  struct Listener* listener = (struct Listener*)
                              malloc(sizeof(struct Listener));
  // 1. 创建监听的fd
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1)
  {
    perror("socket");
    return NULL;
  }
  // 2. 设置端口复用
  int opt = 1;
  int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  if (ret == -1)
  {
    perror("setsockopt");
    return NULL;
  }
  // 3. 绑定
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
  if (ret == -1)
  {
    perror("bind");
    return NULL;
  }
  // 4. 设置监听
  ret = listen(lfd, 128);
  if (ret == -1)
  {
    perror("listen");
    return NULL;
  }
  // 返回fd
  listener->lfd = lfd;
  listener->port = port;
  return listener;
}

int acceptConnection(void* arg)
{
  struct TcpServer* server = (struct TcpServer*)arg;
  // 和客户端建立连接
  int cfd = accept(server->listener->lfd, NULL, NULL);
  // 从线程池中取出一个子线程的反应堆实例, 去处理这个cfd
  struct EventLoop* loop = takeWorkerEventLoop(server->threadPool);

  // 将cfd放到 TcpConnection中处理
  tcpConnectionInit(cfd, loop);
  return 0;
}

void tcpServerRun(struct TcpServer* server)
{
  Debug("服务器程序已经启动了...");

  threadPoolRun(server->threadPool); // 启动线程池
  // 在这之后程序中就有了 threadNum + 1 个线程存在
  // 也就是 1个主线程 + threadNum个子线程
  
  // 添加检测的任务
  // 初始化一个channel实例
  struct Channel* channel = channelInit(server->listener->lfd, ReadEvent, 
                                        acceptConnection, NULL, NULL,
                                        server);
  eventLoopAddTask(server->mainLoop, channel, ADD);
  eventLoopRun(server->mainLoop); // 启动反应堆模型
}