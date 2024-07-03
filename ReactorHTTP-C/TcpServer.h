/* ************************************************************************
> File Name:     TcpServer.h
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 04:47:16 PM CST
> Description:
 ************************************************************************/

#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

struct Listener
{
  int lfd;
  unsigned short port;
};

struct TcpServer
{
  int threadNum;
  struct EventLoop *mainLoop;
  struct ThreadPool *threadPool;
  struct Listener *listener;
};

struct TcpServer *tcpServerInit(unsigned short port, int threadNum);
struct Listener *listenerInit(unsigned short port);
void tcpServerRun(struct TcpServer *server);