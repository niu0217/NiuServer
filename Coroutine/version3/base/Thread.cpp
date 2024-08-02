/* ************************************************************************
> File Name:     Thread.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:04:09 PM CST
> Description:   
 ************************************************************************/

#include "Thread.h"
#include "Util.h"

// 指向当前线程的指针
static thread_local Thread* t_thread = nullptr;
// 当前线程的名字
static thread_local std::string t_thread_name = "UNKNOW";

Thread* Thread::GetThis()
{
  return t_thread;
}

const std::string& Thread::GetName()
{
  return t_thread_name;
}

void Thread::SetName(const std::string& name)
{
  if(name.empty())
  {
    return;
  }
  if(t_thread)
  {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
  : m_cb(cb),
    m_name(name)
{
  if(name.empty())
  {
    m_name = "UNKNOW";
  }
  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if(rt)
  {
    throw std::logic_error("pthread_create error");
  }
  m_semaphore.wait();
}

Thread::~Thread()
{
  if(m_thread)
  {
    pthread_detach(m_thread);
  }
}

void Thread::join()
{
  if(m_thread)
  {
    int rt = pthread_join(m_thread, nullptr);
    if(rt)
    {
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void* Thread::run(void* arg)
{
  Thread* thread = (Thread*)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = util::GetThreadId();
  // 设置线程显示的名称，便于调试
  pthread_setname_np(pthread_self(),
                     thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->m_cb);

  thread->m_semaphore.notify();

  cb();
  return 0;
}