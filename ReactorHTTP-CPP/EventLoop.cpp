/* ************************************************************************
> File Name:     EventLoop.cpp
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 03:55:46 PM CST
> Description:   
 ************************************************************************/

#include "EventLoop.h"
#include "SelectDispatcher.h"
#include "PollDispatcher.h"
#include "EpollDispatcher.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

EventLoop::EventLoop()
  : EventLoop(string())
{
}

EventLoop::EventLoop(const string threadName)
{
  m_isQuit = true;    // 默认没有启动

  m_threadID = this_thread::get_id();
  m_threadName = threadName == string() ? "MainThread" : threadName;

  m_dispatcher = new EpollDispatcher(this);

  m_channelMap.clear();

  int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
  if (ret == -1)
  {
    perror("socketpair");
    exit(0);
  }
#if 0
  // 指定规则: evLoop->socketPair[0] 发送数据, evLoop->socketPair[1] 接收数据
  Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent,
                                 readLocalMessage, nullptr, nullptr, 
                                 this);
#else
  // 绑定 - bind
  auto obj = bind(&EventLoop::readMessage, this);
  Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent,
                                 obj, nullptr, nullptr, 
                                 this);
#endif
  // channel 添加到任务队列
  addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
  m_isQuit = false;
  if (m_threadID != this_thread::get_id())
  {
    return -1;
  }
  // 循环进行事件处理
  while (!m_isQuit)
  {
    m_dispatcher->dispatch();    // 超时时长 2s
    processTaskQ();
  }
  return 0;
}

int EventLoop::eventActive(int fd, int event)
{
  if (fd < 0)
  {
    return -1;
  }
  Channel* channel = m_channelMap[fd];
  assert(channel->getSocket() == fd);
  if (event & (int)FDEvent::ReadEvent && channel->readCallback)
  {
    channel->readCallback(const_cast<void*>(channel->getArg()));
  }
  if (event & (int)FDEvent::WriteEvent && channel->writeCallback)
  {
    channel->writeCallback(const_cast<void*>(channel->getArg()));
  }
  return 0;
}

int EventLoop::addTask(Channel* channel, ElemType type)
{
  m_mutex.lock();  // 加锁, 保护共享资源
  ChannelElement* node = new ChannelElement;
  node->channel = channel;
  node->type = type;
  m_taskQ.push(node);
  m_mutex.unlock();

  if (m_threadID == this_thread::get_id())
  {
    processTaskQ();
  }
  else
  {
    taskWakeup();
  }
  return 0;
}

int EventLoop::processTaskQ()
{
  while (!m_taskQ.empty())
  {
    m_mutex.lock();
    ChannelElement* node = m_taskQ.front();
    m_taskQ.pop();
    m_mutex.unlock();

    Channel* channel = node->channel;
    if (node->type == ElemType::ADD)
    {
      add(channel);
    }
    else if (node->type == ElemType::DELETE)
    {
      remove(channel);
    }
    else if (node->type == ElemType::MODIFY)
    {
      modify(channel);
    }
    delete node;
    node = nullptr;
  }
  return 0;
}

int EventLoop::add(Channel* channel)
{
  int fd = channel->getSocket();
  if (m_channelMap.find(fd) == m_channelMap.end())
  {
    m_channelMap.insert(make_pair(fd, channel));
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->add();
    return ret;
  }
  return -1;
}

int EventLoop::remove(Channel* channel)
{
  int fd = channel->getSocket();
  if (m_channelMap.find(fd) == m_channelMap.end())
  {
    return -1;
  }
  // TODO 删除fd和channel的映射关系
  m_dispatcher->setChannel(channel);
  int ret = m_dispatcher->remove();
  return ret;
}

int EventLoop::modify(Channel* channel)
{
  int fd = channel->getSocket();
  if (m_channelMap.find(fd) == m_channelMap.end())
  {
    return -1;
  }
  m_dispatcher->setChannel(channel);
  int ret = m_dispatcher->modify();
  return ret;
}

void EventLoop::taskWakeup()
{
  const char* msg = "我是要成为海贼王的男人!!!";
  write(m_socketPair[0], msg, strlen(msg));
}

int EventLoop::freeChannel(Channel* channel)
{
  auto it = m_channelMap.find(channel->getSocket());
  if (it != m_channelMap.end())
  {
    m_channelMap.erase(it);
    close(channel->getSocket());
    delete channel;
  }
  return 0;
}

int EventLoop::readLocalMessage(void* arg)
{
  EventLoop* evLoop = static_cast<EventLoop*>(arg);
  char buf[256];
  read(evLoop->m_socketPair[1], buf, sizeof(buf));
  return 0;
}

int EventLoop::readMessage()
{
  char buf[256];
  read(m_socketPair[1], buf, sizeof(buf));
  return 0;
}