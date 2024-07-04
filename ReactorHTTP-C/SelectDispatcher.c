/* ************************************************************************
> File Name:     SelectDispatcher.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 05:29:01 PM CST
> Description:   
 ************************************************************************/

#include "Dispatcher.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 1024
struct SelectData
{
  fd_set readSet;
  fd_set writeSet;
};

static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* loop);
static int selectRemove(struct Channel* channel, struct EventLoop* loop);
static int selectModify(struct Channel* channel, struct EventLoop* loop);
static int selectDispatch(struct EventLoop* loop, int timeout); // 单位: s
static int selectClear(struct EventLoop* loop);
static void setFdSet(struct Channel* channel, struct SelectData* data);
static void clearFdSet(struct Channel* channel, struct SelectData* data);

struct Dispatcher SelectDispatcher = {
  selectInit,
  selectAdd,
  selectRemove,
  selectModify,
  selectDispatch,
  selectClear
};

// 返回值最终给到了 EventLoop 的 dispatcherData
// 我们得到一个 EventLoop 就可以得到一个 dispatcherData
// 从而得到 readSet 和 writeSet
static void* selectInit()
{
  struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
  FD_ZERO(&data->readSet);
  FD_ZERO(&data->writeSet);
  return data;
}

static void setFdSet(struct Channel* channel, struct SelectData* data)
{
  if (channel->events & ReadEvent)
  {
    FD_SET(channel->fd, &data->readSet);
  }
  if (channel->events & WriteEvent)
  {
    FD_SET(channel->fd, &data->writeSet);
  }
}

static void clearFdSet(struct Channel* channel, struct SelectData* data)
{
  if (channel->events & ReadEvent)
  {
    FD_CLR(channel->fd, &data->readSet);
  }
  if (channel->events & WriteEvent)
  {
    FD_CLR(channel->fd, &data->writeSet);
  }
}

static int selectAdd(struct Channel* channel, struct EventLoop* loop)
{
  struct SelectData* data = (struct SelectData*)loop->dispatcherData;
  if (channel->fd >= MAX)
  {
    return -1;
  }
  setFdSet(channel, data);
  return 0;
}

static int selectRemove(struct Channel* channel, struct EventLoop* loop)
{
  struct SelectData* data = (struct SelectData*)loop->dispatcherData;
  clearFdSet(channel, data);

  // 通过 channel 释放对应的 TcpConnection 资源
  channel->destroyCallback(channel->arg);

  return 0;
}

static int selectModify(struct Channel* channel, struct EventLoop* loop)
{
  struct SelectData* data = (struct SelectData*)loop->dispatcherData;
  setFdSet(channel, data);
  clearFdSet(channel, data);  // 什么也没干
  return 0;
}

static int selectDispatch(struct EventLoop* loop, int timeout)
{
  struct SelectData* data = (struct SelectData*)loop->dispatcherData;
  struct timeval val;
  val.tv_sec = timeout;
  val.tv_usec = 0;
  fd_set rdtmp = data->readSet;
  fd_set wrtmp = data->writeSet;
  // select 的第2、3、4个参数都是传入传出参数
  select(MAX, &rdtmp, &wrtmp, NULL, &val);
  for (int i = 0; i < MAX; i++)
  {
    if (FD_ISSET(i, &rdtmp))
    {
      eventActivate(loop, i, ReadEvent);
    }
    if (FD_ISSET(i, &wrtmp))
    {
      eventActivate(loop, i, WriteEvent);
    }
  }
  return 0;
}

static int selectClear(struct EventLoop* loop)
{
  struct SelectData* data = (struct SelectData*)loop->dispatcherData;
  free(data);
  data = NULL;
  return 0;
}