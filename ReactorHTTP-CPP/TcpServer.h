/* ************************************************************************
> File Name:     TcpServer.h
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 09:46:25 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include "EventLoop.h"
#include "ThreadPool.h"

class TcpServer
{
public:
  TcpServer(unsigned short port, int threadNum);

  int setListen();
  void run();
  static int acceptConnection(void* arg);

private:
  unsigned short m_port;
  int m_lfd;
  int m_threadNum;  // 线程池中线程的个数
  EventLoop* m_mainLoop;  // 主线程的反应堆模型
  ThreadPool* m_threadPool;
};