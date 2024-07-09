/* ************************************************************************
> File Name:     Mutex.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 03:52:19 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Noncopyable.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>

// 互斥锁
class Mutex : Noncopyable
{
public:
  Mutex()
  {
    pthread_mutex_init(&m_mutex, nullptr);
  }

  ~Mutex()
  {
    pthread_mutex_destroy(&m_mutex);
  }

  void lock()
  {
    pthread_mutex_lock(&m_mutex);
  }
  void unlock()
  {
    pthread_mutex_unlock(&m_mutex);
  }
private:
  pthread_mutex_t m_mutex;
};

// 信号量
class Semaphore : Noncopyable
{
public:
  Semaphore(uint32_t count = 0)
  {
    if(sem_init(&m_semaphore, 0, count))
    {
      throw std::logic_error("sem_init error");
    }
  }
  ~Semaphore()
  {
    sem_destroy(&m_semaphore);
  }
  
  void wait()
  {
    if(sem_wait(&m_semaphore))
    {
      throw std::logic_error("sem_wait error");
    }
  }
  void notify()
  {
    if(sem_post(&m_semaphore))
    {
      throw std::logic_error("sem_post error");
    }
  }
private:
  sem_t m_semaphore;
};