/* ************************************************************************
> File Name:     WorkerThread.c
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 10:55:09 AM CST
> Description:   
 ************************************************************************/

#include "WorkerThread.h"
#include <stdio.h>

int workerThreadInit(struct WorkerThread* thread, int index)
{
  thread->loop = NULL;
  thread->threadID = 0;
  snprintf(thread->name, "SubThread-%d", index);
  pthread_mutex_init(&thread->mutex, NULL);
  pthread_cond_init(&thread->cond, NULL);
  return 0;
}

// 子线程的回调函数
void* subThreadRunning(void* arg)
{
  struct WorkerThread* thread = (struct WorkerThread*)arg;

  pthread_mutex_lock(&thread->mutex);
  thread->loop = eventLoopInitEx(thread->name);
  pthread_mutex_unlock(&thread->mutex);

  pthread_cond_signal(&thread->cond);
  eventLoopRun(thread->loop);  // 循环处理事件
  return NULL;
}

void workerThreadRun(struct WorkerThread* thread)
{
  pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
  // 这里必须加锁的原因：
  //   thread 是一个共享资源，我们在子线程中需要对其进行修改
  //   同时在主线程中需要访问 thread，因此必须加锁
  pthread_mutex_lock(&thread->mutex);
  while (thread->loop == NULL)
  {
    pthread_cond_wait(&thread->cond, &thread->mutex);
  }
  pthread_mutex_unlock(&thread->mutex);
}
