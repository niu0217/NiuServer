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

// eventLoopInitEx 只会在子线程中被调用
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
    dispatcher->dispatch(loop, 2);  // 最终调用 epollDispatch
    eventLoopProcessTask(loop);     // 处理任务队列中的任务
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

// 为什么访问任务队列（节点ChannelElement） 需要加锁?
//   原因：主线程可以调用 eventLoopAddTask(subloop, channel, type)向subloop（子线程）的
//        任务队列中添加元素；而此时如果子线程正在操作任务队列，那么就会出错；
//
// 参数解释：
//   loop -- 有可能是子线程的，也有可能是主线程的
//   channel -- 之前已经调用过 channelInit；也就是它其中的fd、回调函数、事件都被初始化好了
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
    // Question 这里如果free(channelMap->list[fd]) 会有问题吗？
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
  // Question 这里需要修改 channelMap 吗
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