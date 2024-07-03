/* ************************************************************************
> File Name:     TcpConnection.h
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 05:23:27 PM CST
> Description:   
 ************************************************************************/

#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"

//#define MSG_SEND_AUTO

struct TcpConnection
{
  struct EventLoop* loop;
  struct Channel* channel;
  struct Buffer* readBuf;
  struct Buffer* writeBuf;
  char name[32];
  // http 协议
  // struct HttpRequest* request;
  // struct HttpResponse* response;
};

// 初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* loop);
int tcpConnectionDestroy(void* conn);