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

  /// @brief 创建m_threadNum个线程并运行，然后将线程保存到m_workerThreads
  void run();

  /// @brief 从线程池中取出一个子线程的EventLoop
  /// @return 返回子线程的EventLoop
  EventLoop* takeWorkerEventLoop();

private:
  EventLoop* m_mainLoop;  // 主线程的反应堆模型
  bool m_isStart;
  int m_threadNum;
  vector<WorkerThread*> m_workerThreads;
  int m_index;
};