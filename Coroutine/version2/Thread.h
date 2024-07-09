/* ************************************************************************
> File Name:     Thread.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:04:05 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Mutex.h"
#include <string>
#include <functional>
#include <memory>

class Thread : Noncopyable
{
public:
  typedef std::shared_ptr<Thread> ptr;

  Thread(std::function<void()> cb, const std::string& name);
  ~Thread();

  pid_t getId() const
  {
    return m_id;
  }
  const std::string& getName() const
  {
    return m_name;
  }

  void join();   // 等待线程执行完成

  static Thread* GetThis();  // 获取当前的线程指针
  static const std::string& GetName();  // 获取当前的线程名称
  static void SetName(const std::string& name);  // 设置当前线程名称

private:
  static void* run(void* arg); // 线程执行函数

private:
  std::function<void()> m_cb;  // 线程执行函数
  std::string m_name;          // 线程名称
  pid_t m_id = -1;             // 线程id
  pthread_t m_thread = 0;      // 线程结构
  Semaphore m_semaphore;       // 信号量
};

// syscall(SYS_gettid) 返回的是线程在内核中的唯一标识符，由操作系统管理。
// pthread_create      返回的是用户空间的线程标识符，用于在用户空间进行线程管理。
//                     这个 ID 只在当前进程内部唯一。