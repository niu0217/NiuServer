/* ************************************************************************
> File Name:     WorkerThread.h
> Author:        niu0217
> Created Time:  Tue 02 Jul 2024 10:55:04 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include <pthread.h>
#include "EventLoop.h"

struct WorkerThread
{
  pthread_t threadID;          // ID
  char name[24];
  pthread_mutex_t mutex;      // 互斥锁
  pthread_cond_t cond;        // 条件变量
  struct EventLoop* loop;   // 反应堆模型
};

int workerThreadInit(struct WorkerThread* thread, int index);
void workerThreadRun(struct WorkerThread* thread);