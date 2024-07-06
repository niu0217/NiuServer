/* ************************************************************************
> File Name:     TcpConnection.cpp
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 03:45:23 PM CST
> Description:   
 ************************************************************************/

#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "Log.h"

int TcpConnection::processRead(void* arg)
{
  TcpConnection* conn = static_cast<TcpConnection*>(arg);
  // 接收数据
  int socket = conn->m_channel->getSocket();
  int count = conn->m_readBuf->socketRead(socket);

  Debug("接收到的http请求数据: %s", conn->m_readBuf->data());

  if (count > 0)
  {
    // 接收到了 http 请求, 解析http请求
#ifdef MSG_SEND_AUTO
    conn->m_channel->writeEventEnable(true);
    conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif
    bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf,
                                                 conn->m_response,
                                                 conn->m_writeBuf,
                                                 socket);
    if (!flag)
    {
      // 解析失败, 回复一个简单的html
      string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
      conn->m_writeBuf->appendString(errMsg);
#ifndef MSG_SEND_AUTO
      conn->m_writeBuf->sendData(socket);
#endif
    }
  }
  else
  {
#ifdef MSG_SEND_AUTO
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
  }
#ifndef MSG_SEND_AUTO
  // 断开连接
  conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
  return 0;
}

int TcpConnection::processWrite(void* arg)
{
  Debug("开始发送数据了(基于写事件发送)....");

  TcpConnection* conn = static_cast<TcpConnection*>(arg);

  int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
  if (count > 0)
  {
    if (conn->m_writeBuf->readableSize() == 0)
    {
      conn->m_channel->writeEventEnable(false);
      conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
      conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
    }
  }
  return 0;
}

int TcpConnection::destroy(void* arg)
{
  TcpConnection* conn = static_cast<TcpConnection*>(arg);

  if (conn != nullptr)
  {
    delete conn;
  }
  return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
  m_evLoop = evloop;
  m_readBuf = new Buffer(10240);
  m_writeBuf = new Buffer(10240);
  m_request = new HttpRequest;
  m_response = new HttpResponse;
  m_name = "Connection-" + to_string(fd);
  m_channel = new Channel(fd, FDEvent::ReadEvent,
                          processRead, processWrite, destroy, 
                          this);
  m_evLoop->addTask(m_channel, ElemType::ADD);
}

TcpConnection::~TcpConnection()
{
  if (m_readBuf && m_readBuf->readableSize() == 0 &&
      m_writeBuf && m_writeBuf->readableSize() == 0)
  {
    delete m_readBuf;
    delete m_writeBuf;
    delete m_request;
    delete m_response;
    m_evLoop->freeChannel(m_channel);
  }
  Debug("连接断开, 释放资源, gameover, connName: %s", m_name.data());
}
