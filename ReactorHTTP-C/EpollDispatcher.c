/* ************************************************************************
> File Name:     EpollDispatcher.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 03:02:19 PM CST
> Description:   
 ************************************************************************/

#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define MAX 520
struct EpollData
{
  int epfd;
  struct epoll_event* events;
};

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* loop);
static int epollRemove(struct Channel* channel, struct EventLoop* loop);
static int epollModify(struct Channel* channel, struct EventLoop* loop);
static int epollDispatch(struct EventLoop* loop, int timeout); // 单位: s
static int epollClear(struct EventLoop* loop);
static int epollCtl(struct Channel* channel, struct EventLoop* loop, int op);

// 在 EventLoop 中使用
struct Dispatcher EpollDispatcher = {
  epollInit,
  epollAdd,
  epollRemove,
  epollModify,
  epollDispatch,
  epollClear
};

// 返回值最终给到了 EventLoop 的 dispatcherData
// 我们得到一个 EventLoop 就可以得到一个 dispatcherData
// 从而得到 epfd 和它上面的事件
static void* epollInit()
{
  struct EpollData *data = (struct EpollData*)malloc(sizeof(struct EpollData));
  data->epfd = epoll_create(10);
  if (data->epfd == -1)
  {
    perror("epoll_create");
    exit(0);
  }
  // calloc 动态分配内存并将其初始化为零
  data->events = (struct epoll_event*)calloc(MAX, sizeof(struct epoll_event));
  return data;
}

static int epollCtl(struct Channel* channel, struct EventLoop* loop, int op)
{
  struct EpollData *data = (struct EpollData*)loop->dispatcherData;
  struct epoll_event ev;
  ev.data.fd = channel->fd;
  int events = 0;
  if (channel->events & ReadEvent)
  {
    events |= EPOLLIN;
  }
  if (channel->events & WriteEvent)
  {
    events |= EPOLLOUT;
  }
  ev.events = events;
  int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
}

static int epollAdd(struct Channel* channel, struct EventLoop* loop)
{
  int ret = epollCtl(channel, loop, EPOLL_CTL_ADD);
  if (ret == -1)
  {
    perror("epoll_ctl add");
    exit(0);
  }
  return ret;
}

static int epollRemove(struct Channel* channel, struct EventLoop* loop)
{
  int ret = epollCtl(channel, loop, EPOLL_CTL_DEL);
  if (ret == -1)
  {
    perror("epoll_ctl delete");
    exit(0);
  }
  // 通过 channel 释放对应的 TcpConnection 资源
  channel->destroyCallback(channel->arg);
  return ret;
}

static int epollModify(struct Channel* channel, struct EventLoop* loop)
{
  int ret = epollCtl(channel, loop, EPOLL_CTL_MOD);
  if (ret == -1)
  {
    perror("epoll_ctl modify");
    exit(0);
  }
  return ret;
}

// 核心 事件循环
static int epollDispatch(struct EventLoop* loop, int timeout)
{
  struct EpollData *data = (struct EpollData*)loop->dispatcherData;
  int count = epoll_wait(data->epfd, data->events, MAX, timeout * 1000);
  for (int i = 0; i < count; i++)
  {
    int events = data->events[i].events;
    int fd = data->events[i].data.fd;
    if (events & EPOLLERR || events & EPOLLHUP)
    {
      // 对端断开了连接，目前暂时这样
      continue;
    }
    if (events & EPOLLIN)
    {
      eventActivate(loop, fd, ReadEvent);
    }
    if (events & EPOLLOUT)
    {
      eventActivate(loop, fd, WriteEvent);
    }
  }
  return 0;
}

static int epollClear(struct EventLoop* loop)
{
  struct EpollData *data = (struct EpollData*)loop->dispatcherData;
  free(data->events);
  close(data->epfd);
  free(data);
  data->events = NULL;
  data = NULL;
  return 0;
}