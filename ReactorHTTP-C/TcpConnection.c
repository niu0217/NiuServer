/* ************************************************************************
> File Name:     TcpConnection.c
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 05:23:31 PM CST
> Description:
 ************************************************************************/

#include "TcpConnection.h"
#include "HttpRequest.h"
#include "Log.h"
#include <stdlib.h>
#include <stdio.h>

int processRead(void *arg)
{
  struct TcpConnection *conn = (struct TcpConnection *)arg;

  int count = bufferSocketRead(conn->readBuf, conn->channel->fd);

  // Debug("接收到的http请求数据: %s", conn->readBuf->data + conn->readBuf->readPos);

  if (count > 0)
  {
    // 接收到了 http 请求, 解析http请求
    int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
    // 注册可写事件
    writeEventEnable(conn->channel, true);
    eventLoopAddTask(conn->loop, conn->channel, MODIFY);
#endif
    bool flag = parseHttpRequest(conn->request, conn->readBuf,
                                conn->response, conn->writeBuf,
                                socket);
    if (!flag)
    {
      // 解析失败, 回复一个简单的html
      char *errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
      bufferAppendString(conn->writeBuf, errMsg);
    }
  }
  else
  {
    eventLoopAddTask(conn->loop, conn->channel, DELETE);
  }
  return 0;
}

int processWrite(void *arg)
{
  struct TcpConnection *conn = (struct TcpConnection *)arg;
  // 发送数据
  int count = bufferSendData(conn->writeBuf, conn->channel->fd);
  if (count > 0)
  {
    // 判断数据是否被全部发送出去了
    // 数据全部发送出去之后也就不需要检测可写事件
    if (bufferReadableSize(conn->writeBuf) == 0)
    {
      // 1. 不再检测写事件 -- 修改channel中保存的事件
      writeEventEnable(conn->channel, false);
      // 2. 修改dispatcher检测的集合 -- 添加任务节点
      eventLoopAddTask(conn->loop, conn->channel, MODIFY);
      // 3. 删除这个节点
      eventLoopAddTask(conn->loop, conn->channel, DELETE);
    }
  }
  return 0;
}

int tcpConnectionDestroy(void *arg)
{
  struct TcpConnection* conn = (struct TcpConnection*)arg;
  if (conn != NULL)
  {
    if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0 &&
        conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0)
    {
      destroyChannel(conn->loop, conn->channel);
      bufferDestroy(conn->readBuf);
      bufferDestroy(conn->writeBuf);
      httpRequestDestroy(conn->request);
      httpResponseDestroy(conn->response);
      free(conn);
    }
  }

  Debug("连接断开, 释放资源, gameover, connName: %s", conn->name);

  return 0;
}

// 特别注意：这个loop来自于线程池中的一个，也就是属于子线程的loop
// 有意思的点：
//    tcpConnectionInit(cfd, loop) 运行在主线程中，但是参数loop却是
//    属于子线程的；也就是说 eventLoopAddTask(loop, conn->channel, ADD);
//    中我们是在主线程中向子线程loop的任务队列中添加任务
struct TcpConnection *tcpConnectionInit(int fd, struct EventLoop *loop)
{
  struct TcpConnection *conn = (struct TcpConnection *)
                               malloc(sizeof(struct TcpConnection));
  conn->loop = loop;
  conn->readBuf = bufferInit(10240);
  conn->writeBuf = bufferInit(10240);

  // http
  conn->request = httpRequestInit();
  conn->response = httpResponseInit();

  sprintf(conn->name, "Connection-%d", fd);
  conn->channel = channelInit(fd, ReadEvent,
                              processRead, processWrite, tcpConnectionDestroy,
                              conn);
  eventLoopAddTask(loop, conn->channel, ADD);

  Debug("和客户端建立连接, threadName: %s, threadID:%ld, connName: %s",
        loop->threadName, loop->threadID, conn->name);

  return conn;
}