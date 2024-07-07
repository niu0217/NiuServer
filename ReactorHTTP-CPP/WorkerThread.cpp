/* ************************************************************************
> File Name:     WorkerThread.cpp
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 08:54:54 PM CST
> Description:   
 ************************************************************************/

#include "WorkerThread.h"
#include <stdio.h>

// 子线程的回调函数
void WorkerThread::running()
{
  m_mutex.lock();
  m_evLoop = new EventLoop(m_name);
  m_mutex.unlock();
  m_cond.notify_one();  // 唤醒主线程
  m_evLoop->run();
}

WorkerThread::WorkerThread(int index)
{
  m_evLoop = nullptr;
  m_thread = nullptr;
  m_threadID = thread::id();
  m_name =  "SubThread-" + to_string(index);
}

WorkerThread::~WorkerThread()
{
  if (m_thread != nullptr)
  {
    delete m_thread;
  }
}

void WorkerThread::run()
{
  // 创建子线程
  m_thread = new thread(&WorkerThread::running, this);
  unique_lock<mutex> locker(m_mutex);
  while (m_evLoop == nullptr)
  {
    m_cond.wait(locker);  // 阻塞主线程直到 m_evLoop 被子线程赋值
  }
}
