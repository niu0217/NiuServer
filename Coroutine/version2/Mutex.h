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

/// @brief 局部锁的模版实现
template<class T>
struct ScopedLockImpl
{
public:
  ScopedLockImpl(T& mutex)
    : m_mutex(mutex)
  {
    m_mutex.lock();
    m_locked = true;
  }

  ~ScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if(!m_locked)
    {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if(m_locked)
    {
      m_mutex.unlock();
      m_locked = false;
    }
  }
private:
  T& m_mutex;
  bool m_locked;  /// 是否已上锁
};

/// @brief 局部读锁模板实现
template<class T>
struct ReadScopedLockImpl
{
public:
  ReadScopedLockImpl(T& mutex)
    : m_mutex(mutex)
  {
    m_mutex.rdlock();
    m_locked = true;
  }

  ~ReadScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if(!m_locked)
    {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if(m_locked)
    {
      m_mutex.unlock();
      m_locked = false;
    }
  }
private:
  T& m_mutex;
  bool m_locked;  /// 是否已上锁
};

/// @brief 局部写锁模板实现
template<class T>
struct WriteScopedLockImpl
{
public:
  WriteScopedLockImpl(T& mutex)
    : m_mutex(mutex)
  {
    m_mutex.wrlock();
    m_locked = true;
  }

  ~WriteScopedLockImpl()
  {
    unlock();
  }

  void lock()
  {
    if(!m_locked)
    {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if(m_locked)
    {
      m_mutex.unlock();
      m_locked = false;
    }
  }
private:
  T& m_mutex;
  bool m_locked;  /// 是否已上锁
};

// 互斥锁
class Mutex : Noncopyable
{
public:
  /// 局部锁
  typedef ScopedLockImpl<Mutex> Lock;

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

// 读写互斥锁
class RWMutex : Noncopyable
{
public:
   /// 局部读锁
  typedef ReadScopedLockImpl<RWMutex> ReadLock;

  /// 局部写锁
  typedef WriteScopedLockImpl<RWMutex> WriteLock;

public:
  RWMutex()
  {
    pthread_rwlock_init(&m_lock, nullptr);
  }
  
  ~RWMutex()
  {
    pthread_rwlock_destroy(&m_lock);
  }

  void rdlock()
  {
    pthread_rwlock_rdlock(&m_lock);
  }

  void wrlock()
  {
    pthread_rwlock_wrlock(&m_lock);
  }

  void unlock()
  {
    pthread_rwlock_unlock(&m_lock);
  }
private:
  pthread_rwlock_t m_lock;
};