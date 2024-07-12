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

  /// @brief 调用注册在fd上的回调函数
  /// @details 主要是通过fd在m_channelMap中找到其对应的Channel，然后执行Channel上的事件回调函数
  /// @param fd 发生事件的文件描述符
  /// @param event 发生的事件
  int eventActive(int fd, int event);

  int addTask(Channel* channel, ElemType type); // 添加任务到任务队列
  int processTaskQ();  // 处理任务队列中的任务
  int add(Channel* channel);
  int remove(Channel* channel);
  int modify(Channel* channel);

  /// @brief 删除Channel
  /// @param channel 要删除的Channel
  /// @details Channel由EventLoop管理，所以必须由它来删除
  int freeChannel(Channel* channel);

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