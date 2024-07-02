/* ************************************************************************
> File Name:     Channel.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 10:04:31 AM CST
> Description:   
 ************************************************************************/

#include "Channel.h"

struct Channel* channelInit(int fd, int events, 
                            EventCallback readCb, 
                            EventCallback writeCb, 
                            EventCallback destoryCb,
                            void *arg)
{
  struct Channel *channel = (struct Channel*)malloc(sizeof(struct Channel));
  channel->fd = fd;
  channel->events = events;
  channel->readCallback = readCb;
  channel->writeCallback = writeCb;
  channel->destoryCallback = destoryCb;
  channel->arg = arg;
  return channel;
}

void enableWriteEvent(struct Channel* channel, bool flag)
{
  if (flag)
  {
    channel->events |= WriteEvent;
  }
  else
  {
    channel->events = channel->events & (~WriteEvent);
  }
}

bool isEnableWrite(struct Channel *channel)
{
  return channel->events & WriteEvent;
}