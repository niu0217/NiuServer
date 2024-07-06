/* ************************************************************************
> File Name:     ThreadPool.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 08:55:17 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdbool.h>
#include <vector>
using namespace std;

// 定义线程池
class ThreadPool
{
public:
  ThreadPool(EventLoop* mainLoop, int count);
  ~ThreadPool();

  void run();
  EventLoop* takeWorkerEventLoop();
private:
  EventLoop* m_mainLoop;  // 主线程的反应堆模型
  bool m_isStart;
  int m_threadNum;
  vector<WorkerThread*> m_workerThreads;
  int m_index;
};