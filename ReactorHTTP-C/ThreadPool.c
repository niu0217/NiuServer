/* ************************************************************************
> File Name:     ThreadPool.c
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 10:55:24 AM CST
> Description:   
 ************************************************************************/

#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>

struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
  struct ThreadPool *pool = (struct ThreadPool*)
                            malloc(sizeof(struct ThreadPool));
  pool->index = 0;
  pool->isStart = false;
  pool->mainLoop = mainLoop;  // 再次特别注意！！！ 这是主线程的EventLoop
  pool->threadNum = count;
  pool->workerThreads = (struct WorkerThread*)
                        malloc(count * sizeof(struct WorkerThread));
  return pool;                        
}

void threadPoolRun(struct ThreadPool* pool)
{
  assert(pool && !pool->isStart);
  if (pool->mainLoop->threadID != pthread_self())
  {
    // 线程池 必须必须必须 在主线程中启动
    exit(0);
  }
  pool->isStart = true;
  if (pool->threadNum)
  {
    for (int i = 0; i < pool->threadNum; i++)
    {
      workerThreadInit(&pool->workerThreads[i], i);
      workerThreadRun(&pool->workerThreads[i]);
    }
  }
}

// 调用者通过返回的 loop 可以向其加入要运行的任务，这样loop就可以处理它
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
  assert(pool->isStart);
  if (pool->mainLoop->threadID != pthread_self())
  {
    // 必须必须必须 在主线程中
    exit(0);
  }
  struct EventLoop *loop = pool->mainLoop;
  if (pool->threadNum > 0)
  {
    // 采用轮叫的方式选择
    loop = pool->workerThreads[pool->index].loop;  // 这个是子线程的 loop
    ++pool->index;
    pool->index = pool->index % pool->threadNum;
  }
  return loop;
}