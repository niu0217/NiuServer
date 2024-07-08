/* ************************************************************************
> File Name:     EventLoop.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 03:55:42 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>
using namespace std;

// 处理该节点中的channel的方式
// 强类型枚举
enum class ElemType:char{ ADD, DELETE, MODIFY };
// 定义任务队列的节点
struct ChannelElement
{
  ElemType type;
  Channel* channel;
};

class Dispatcher;
class EventLoop
{
public:
  EventLoop();   // 主线程调用
  EventLoop(const string threadName); // 子线程调用
  ~EventLoop();

  int run();  // 启动反应堆模型
  int eventActive(int fd, int event);  // 调用注册在fd上的回调函数

  int addTask(struct Channel* channel, ElemType type); // 添加任务到任务队列
  int processTaskQ();  // 处理任务队列中的任务
  int add(Channel* channel);
  int remove(Channel* channel);
  int modify(Channel* channel);

  int freeChannel(Channel* channel);  // 释放channel

  int readMessage();
  static int readLocalMessage(void* arg);

  inline thread::id getThreadID()
  {
    return m_threadID;
  }
  inline string getThreadName()
  {
    return m_threadName;
  }

private:
  void taskWakeup();

private:
  bool m_isQuit;
  Dispatcher* m_dispatcher;         // 该指针指向子类的实例 epoll, poll, select
  queue<ChannelElement*> m_taskQ;   // 任务队列
  map<int, Channel*> m_channelMap;  // 保存fd和Channel的关系
  thread::id m_threadID;
  string m_threadName;
  mutex m_mutex;
  int m_socketPair[2];  // 存储本地通信的fd 通过socketpair 初始化
};