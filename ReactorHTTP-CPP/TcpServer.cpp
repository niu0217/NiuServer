/* ************************************************************************
> File Name:     TcpServer.cpp
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 09:46:29 AM CST
> Description:   
 ************************************************************************/

#include "TcpServer.h"
#include "TcpConnection.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

int TcpServer::acceptConnection(void* arg)
{
  TcpServer* server = static_cast<TcpServer*>(arg);
  int cfd = accept(server->m_lfd, NULL, NULL);  // 和客户端建立连接
  // 从线程池中取出一个子线程的反应堆实例, 去处理这个cfd
  EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
  // 将cfd放到 TcpConnection中处理
  new TcpConnection(cfd, evLoop);
  return 0;
}

TcpServer::TcpServer(unsigned short port, int threadNum)
  : m_port(port),
    m_lfd(setListen()),
    m_threadNum(threadNum),
    m_mainLoop(new EventLoop),
    m_threadPool(new ThreadPool(m_mainLoop, threadNum))
{
}

int TcpServer::setListen()
{
  int listenFd;
  // 1. 创建监听的fd
  listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd == -1)
  {
    perror("socket");
    return -1;
  }
  // 2. 设置端口复用
  int opt = 1;
  int ret = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  if (ret == -1)
  {
    perror("setsockopt");
    return -1;
  }
  // 3. 绑定
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  addr.sin_addr.s_addr = INADDR_ANY;
  ret = bind(listenFd, (struct sockaddr*)&addr, sizeof addr);
  if (ret == -1)
  {
    perror("bind");
    return -1;
  }
  // 4. 设置监听
  ret = listen(listenFd, 128);
  if (ret == -1)
  {
    perror("listen");
    return -1;
  }
  return listenFd;
}

void TcpServer::run()
{
  Debug("服务器程序已经启动了...");
  m_threadPool->run();  // 启动线程池
  // 添加检测的任务
  // 初始化一个channel实例
  Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, 
                                 acceptConnection, nullptr, nullptr, 
                                 this);
  m_mainLoop->addTask(channel, ElemType::ADD);
  m_mainLoop->run();  // 启动反应堆模型
}