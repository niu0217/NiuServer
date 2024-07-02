/* ************************************************************************
> File Name:     ThreadPool.h
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 10:55:20 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include <stdbool.h>
#include "EventLoop.h"
#include "WorkerThread.h"

/**************************调用关系******************************
> 线程池是由主线程调用的，在主线程中设置线程池中线程的个数
***************************************************************/

// 为什么要把主线程的EventLoop传过来？
//   因为当 threadNum = 0的时候，线程池中没有线程，也就没有EventLoop
//   线程池也就不能工作了，而当我们把主线程的EventLoop传过来后，当
//   threadNum = 0的时候，我们就可以使用它来处理事件，线程池正常工作
struct ThreadPool
{
  // 主线程的反应堆模型
  struct EventLoop* mainLoop; // 特别注意!!! 这个是主线程的 EventLoop
  bool isStart;
  int threadNum;  // 子线程的数量
  struct WorkerThread* workerThreads;  // 保存子线程的数组指针
  int index;  // 子线程在 workerThreads 中的编号
};

// 初始化线程池
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);
// 启动线程池
void threadPoolRun(struct ThreadPool* pool);
// 取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);