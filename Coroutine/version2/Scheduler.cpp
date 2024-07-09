/* ************************************************************************
> File Name:     Scheduler.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 08:13:24 PM CST
> Description:   
 ************************************************************************/

#include "Scheduler.h"
#include "Util.h"
#include <assert.h>
#include <functional>

// 当前线程的调度器，同一个调度器下的所有线程都指向同一个调度器实例
static thread_local Scheduler* t_scheduler = nullptr;
// 当前线程的调度器，每个线程独有一份，包括caller线程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string &name = "Scheduler")
{
  assert(threads > 0);
  m_userCaller = use_caller;
  m_name = name;

  if (use_caller)
  {
    --threads;
    Fiber::GetThis();
    // assert(GetThis() != nullptr);
    t_scheduler = this;
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

    Thread::SetName(m_name);
    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread = util::GetThreadId();
    m_threadIds.push_back(m_rootThread);
  }
  else
  {
    m_rootThread = -1;
  }
  m_threadCount = threads;
}

Scheduler* Scheduler::GetThis()
{
  return t_scheduler;
}

void Scheduler::start()
{
  m_mutex.lock();
  if (m_stopping)
  {
    perror("Scheduler is stopped");
    return;
  }
  assert(m_threads.empty());
  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; i++)
  {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId());
  }
}

void Scheduler::run()
{
  
}