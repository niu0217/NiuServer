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
#include "HttpRequest.h"
#include "HttpResponse.h"

// 定义 MSG_SEND_AUTO    --> 代表注册可写事件去给客户发送消息
// 没有定义 MSG_SEND_AUTO --> 代表不需要注册可写事件去给客户发送消息，我们读一点数据就发送一点数据
#define MSG_SEND_AUTO

struct TcpConnection
{
  struct EventLoop* loop;   // 不需要 TcpConnection 管理它的生命周期
  struct Channel* channel;
  struct Buffer* readBuf;
  struct Buffer* writeBuf;
  char name[32];
  // http 协议
  struct HttpRequest* request;
  struct HttpResponse* response;
};

// 初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* loop);
int tcpConnectionDestroy(void* conn);