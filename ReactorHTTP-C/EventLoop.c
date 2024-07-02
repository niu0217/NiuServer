/* ************************************************************************
> File Name:     EventLoop.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 02:41:25 PM CST
> Description:   
 ************************************************************************/

#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// 写数据
void taskWakeup(struct EventLoop* loop)
{
  const char* msg = "hello";
  write(loop->socketPair[0], msg, strlen(msg));
}

// 读数据
int readLocalMessage(void* arg)
{
  struct EventLoop* loop = (struct EventLoop*)arg;
  char buf[256];
  read(loop->socketPair[1], buf, sizeof(buf));
  return 0;
}

struct EventLoop* eventLoopInit()
{
  return eventLoopInitEx(NULL);
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
  struct EventLoop *loop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
  loop->isQuit = false;

  loop->threadID = pthread_self();
  pthread_mutex_init(&loop->mutex, NULL);
  strcpy(loop->threadName, threadName == NULL ? "MainThread" : threadName);

  loop->dispatcher = &EpollDispatcher;
  loop->dispatcherData = loop->dispatcher->init(); // 也就是调用 epollInit()

  loop->head = loop->tail = NULL;
  loop->channelMap = channelMapInit(128);

  int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, loop->socketPair);
  if (ret == -1)
  {
    perror("socketpair");
    exit(0);
  }
  // 指定规则: loop->socketPair[0] 发送数据, loop->socketPair[1] 接收数据
  struct Channel *channel = channelInit(loop->socketPair[1], ReadEvent,
                                       readLocalMessage, NULL, NULL, loop);
  eventLoopAddTask(loop, channel, ADD);                                     
  return loop;
}

int eventLoopRun(struct EventLoop* loop)
{
  assert(loop != NULL);
  if (loop->threadID != pthread_self())
  {
    return -1;
  }
  struct Dispatcher *dispatcher = loop->dispatcher;
  while (!loop->isQuit)
  {
    dispatcher->dispatch(loop, 2);
    eventLoopProcessTask(loop);
  }
  return 0;
}

int eventActivate(struct EventLoop* loop, int fd, int event)
{
  if (fd < 0 || loop == NULL)
  {
    return -1;
  }
  struct Channel *channel = loop->channelMap->list[fd];
  assert(channel->fd == fd);
  if (event & ReadEvent && channel->readCallback)
  {
    channel->readCallback(channel->arg);
  }
  if(event & WriteEvent && channel->writeCallback)
  {
    channel->writeCallback(channel->arg);
  }
  return 0;
}

int eventLoopAddTask(struct EventLoop* loop, struct Channel* channel, int type)
{
  pthread_mutex_lock(&loop->mutex);
  struct ChannelElement *node = (struct ChannelElement*)
                                malloc(sizeof(struct ChannelElement));
  node->channel = channel;
  node->type = type;
  node->next = NULL;
  if (loop->head == NULL)
  {
    loop->head = loop->tail = node;
  }                              
  else
  {
    loop->tail->next = node;
    loop->tail = node;
  }
  pthread_mutex_unlock(&loop->mutex);

  if (loop->threadID == pthread_self())
  {
    // 当前线程 基于子线程的角度分析
    eventLoopProcessTask(loop);
  }
  else
  {
    // 主线程 -- 告诉子线程处理任务队列中的任务
    //          1、子线程在工作
    //          2、子线程被阻塞了：select poll epoll
    taskWakeup(loop);
  }
  return 0;
}

int eventLoopProcessTask(struct EventLoop* loop)
{
  pthread_mutex_lock(&loop->mutex);
  struct ChannelElement *head = loop->head;
  while (head != NULL)
  {
    struct Channel *channel = head->channel;
    if (head->type == ADD)
    {
      eventLoopAdd(loop, channel);
    }
    else if (head->type == DELETE)
    {
      eventLoopRemove(loop, channel);
    }
    else if (head->type == MODIFY)
    {
      eventLoopModify(loop, channel);
    }
    struct ChannelElement *temp = head;
    head = head->next;
    free(temp);
    temp = NULL;
  }
  loop->head = loop->tail = NULL;
  pthread_mutex_unlock(&loop->mutex);
  return 0;
}

int eventLoopAdd(struct EventLoop* loop, struct Channel* channel)
{
  int fd = channel->fd;
  struct ChannelMap *channelMap = loop->channelMap;
  if (fd >= channelMap->size)
  {
    if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*)))
    {
      return -1;
    }
  }
  if (channelMap->list[fd] == NULL)
  {
    channelMap->list[fd] = channel;
    loop->dispatcher->add(channel, loop);
  }
  return 0;
}

int eventLoopRemove(struct EventLoop* loop, struct Channel* channel)
{
  int fd = channel->fd;
  struct ChannelMap *channelMap = loop->channelMap;
  if (fd >= channelMap->size)
  {
    return -1;
  }
  int ret = loop->dispatcher->remove(channel, loop);
  return ret;
}

int eventLoopModify(struct EventLoop* loop, struct Channel* channel)
{
  int fd = channel->fd;
  struct ChannelMap *channelMap = loop->channelMap;
  if (channelMap->list[fd] == NULL)
  {
    return -1;
  }
  int ret = loop->dispatcher->modify(channel, loop);
  return ret;
}

int destroyChannel(struct EventLoop* loop, struct Channel* channel)
{
 loop->channelMap->list[channel->fd] = NULL;
 close(channel->fd);
 free(channel);
 channel = NULL;
 return 0; 
}