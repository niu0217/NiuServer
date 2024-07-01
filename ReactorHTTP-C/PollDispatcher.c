/* ************************************************************************
> File Name:     PollDispatcher.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 04:27:04 PM CST
> Description:   
 ************************************************************************/

#include "Dispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX 1024
struct PollData
{
  int maxfd;
  struct pollfd fds[MAX];
};

static void* pollInit();
static int pollAdd(struct Channel* channel, struct EventLoop* loop);
static int pollRemove(struct Channel* channel, struct EventLoop* loop);
static int pollModify(struct Channel* channel, struct EventLoop* loop);
static int pollDispatch(struct EventLoop* loop, int timeout); // 单位: s
static int pollClear(struct EventLoop* loop);

struct Dispatcher PollDispatcher = {
  pollInit,
  pollAdd,
  pollRemove,
  pollModify,
  pollDispatch,
  pollClear
};

// 返回值最终给到了 EventLoop 的 dispatcherData
// 我们得到一个 EventLoop 就可以得到一个 dispatcherData
// 从而得到 maxfd 和 fds[MAX]
static void* pollInit()
{
  struct PollData* data = (struct PollData*)malloc(sizeof(struct PollData));
  data->maxfd = 0;
  for (int i = 0; i < MAX; i++)
  {
    data->fds[i].fd = -1;
    data->fds[i].events = 0;
    data->fds[i].revents = 0;
  }
  return data;
}

static int pollAdd(struct Channel* channel, struct EventLoop* loop)
{
  struct PollData* data = (struct PollData*)loop->dispatcherData;
  int events = 0;
  if (channel->events & ReadEvent)
  {
    events |= POLLIN;
  }
  if (channel->events & WriteEvent)
  {
    events |= POLLOUT;
  }
  int i = 0;
  for (; i < MAX; i++)
  {
    // 找到第一个空闲的位置来放数据
    if (data->fds[i].fd == -1)
    {
      data->fds[i].fd = channel->fd;
      data->fds[i].events = events;
      // data->maxfd = channel->fd > data->maxfd ? channel->fd : data->maxfd;
      data->maxfd = i > data->maxfd ? i : data->maxfd;
      break;
    }
  }
  if (i >= MAX)
  {
    return -1;
  }
  return 0;
}

static int pollRemove(struct Channel* channel, struct EventLoop* loop)
{
  struct PollData* data = (struct PollData*)loop->dispatcherData;
  int i = 0;
  for (; i < MAX; i++)
  {
    if (channel->fd == data->fds[i].fd)
    {
      data->fds[i].fd = -1;
      data->fds[i].events = 0;
      data->fds[i].revents = 0;
      break;
    }
  }
  if (i >= MAX)
  {
    return -1;
  }
  return 0;
}

static int pollModify(struct Channel* channel, struct EventLoop* loop)
{
  struct PollData* data = (struct PollData*)loop->dispatcherData;
  int events = 0;
  if (channel->events & ReadEvent)
  {
    events |= POLLIN;
  }
  if (channel->events & WriteEvent)
  {
    events |= POLLOUT;
  }
  int i = 0;
  for (; i < MAX; i++)
  {
    if (data->fds[i].fd == channel->fd)
    {
      data->fds[i].events = events;
      break;
    }
  }
  if (i >= MAX)
  {
    return -1;
  }
  return 0;
}

static int pollDispatch(struct EventLoop* loop, int timeout)
{
  struct PollData* data = (struct PollData*)loop->dispatcherData;
  int count = poll(data->fds, data->maxfd + 1, timeout * 1000);
  for (int i = 0; i <= data->maxfd; i++)
  {
    if (data->fds[i].fd == -1)
    {
      continue;
    }
    if (data->fds[i].revents & POLLIN)
    {
    }
    if (data->fds[i].revents & POLLOUT)
    {
    }
  }
  return 0;
}

static int pollClear(struct EventLoop* loop)
{
  struct PollData* data = (struct PollData*)loop->dispatcherData;
  free(data);
  return 0;
}