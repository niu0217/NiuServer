/* ************************************************************************
> File Name:     WorkerThread.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 08:54:49 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

// 定义子线程对应的结构体
class WorkerThread
{
public:
  WorkerThread(int index);
  ~WorkerThread();

  void run();
  inline EventLoop* getEventLoop()
  {
    return m_evLoop;
  }

private:
  void running();

private:
  thread* m_thread;   // 保存线程的实例
  thread::id m_threadID; // ID
  string m_name;
  mutex m_mutex;  // 互斥锁
  condition_variable m_cond;    // 条件变量
  EventLoop* m_evLoop;   // 反应堆模型
};