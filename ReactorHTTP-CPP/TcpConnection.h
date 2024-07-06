/* ************************************************************************
> File Name:     TcpConnection.h
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 03:45:19 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

// #define MSG_SEND_AUTO

class TcpConnection
{
public:
  TcpConnection(int fd, EventLoop* evloop);
  ~TcpConnection();

  static int processRead(void* arg);
  static int processWrite(void* arg);
  static int destroy(void* arg);

private:
  string m_name;
  EventLoop* m_evLoop;
  Channel* m_channel;
  Buffer* m_readBuf;   // 保存从客户端接收到的数据
  Buffer* m_writeBuf;  // 保存要发送给客户端的数据
  HttpRequest* m_request;
  HttpResponse* m_response;
};