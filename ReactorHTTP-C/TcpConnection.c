/* ************************************************************************
> File Name:     TcpConnection.c
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 05:23:31 PM CST
> Description:   
 ************************************************************************/

#include "TcpConnection.h"

int processRead(void* arg)
{
  struct TcpConnection *conn = (struct TcpConnection*)arg;
  // 接收数据
  int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
  if (count > 0)
  {
    // 接收到了http请求，解析http请求
  }
  else
  {
    // 断开连接
  }
}

// 特别注意：这个loop来自于线程池中的一个，也就是属于子线程的loop
// 有意思的点：
//    tcpConnectionInit(cfd, loop) 运行在主线程中，但是参数loop却是
//    属于子线程的；也就是说 eventLoopAddTask(loop, conn->channel, ADD);
//    中我们是在主线程中向子线程loop的任务队列中添加任务
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* loop)
{
  struct TcpConnection* conn = (struct TcpConnection*)
                               malloc(sizeof(struct TcpConnection));
  conn->loop = loop;
  conn->readBuf = bufferInit(10240);
  conn->writeBuf = bufferInit(10240);
  sprintf(conn->name, "Connection-%d", fd);
  conn->channel = channelInit(fd, ReadEvent, processRead, NULL, NULL, conn);
  eventLoopAddTask(loop, conn->channel, ADD);

  return conn;
}