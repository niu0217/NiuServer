/* ************************************************************************
> File Name:     Scheduler.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 08:13:24 PM CST
> Description:   
 ************************************************************************/

#include "Scheduler.h"
#include "Util.h"
#include "Log.h"
#include <assert.h>
#include <functional>

// 当前线程的调度器，同一个调度器下的所有线程都指向同一个调度器实例
static thread_local Scheduler* t_scheduler = nullptr;
// 当前线程的调度器，每个线程独有一份，包括caller线程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller,
                     const std::string &name)
{
  assert(threads > 0);
  m_userCaller = use_caller;
  m_name = name;

  if (use_caller)
  {
    --threads;
    Fiber::GetThis();
    t_scheduler = this;
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

    Thread::SetName(m_name);
    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread = util::GetThreadId();
    m_threadIds.push_back(m_rootThread);
  }
  else
  {
    m_rootThread = -1;   // 选择任意线程
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler()
{
  assert(m_stopping);
  if (GetThis() == this)
  {
    t_scheduler = nullptr;
  }
}

Scheduler* Scheduler::GetThis()
{
  return t_scheduler;
}

Fiber* Scheduler::GetMainFiber()
{
  return t_scheduler_fiber;
}

void Scheduler::setThis()
{
  t_scheduler = this;
}

void Scheduler::tickle()
{

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
  m_threads.resize(m_threadCount);   // 设置线程池的大小
  for (size_t i = 0; i < m_threadCount; i++)
  {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId());
  }
  m_mutex.unlock();
}

void Scheduler::run()
{
  setThis(); 
  if (util::GetThreadId() != m_rootThread)
  {
    t_scheduler_fiber = Fiber::GetThis().get();
  }

  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  ScheduleTask task;
  while (true)
  {
    task.reset();
    bool tickle_me = false; // 是否tickle其他线程进行任务调度
    {
      m_mutex.lock();
      auto it = m_tasks.begin();
      // 遍历所有调度任务
      while (it != m_tasks.end())
      {
        if (it->thread != - 1 && it->thread != util::GetThreadId())
        {
          // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度
          // 然后跳过这个任务，处理下一个
          ++it;
          tickle_me = true;
          continue;
        }
        // 找到一个未指定线程，或是指定了当前线程的任务
        assert(it->fiber || it->cb);
        if (it->fiber)
        {
          // 任务队列的协程一定是READY状态
          assert(it->fiber->getState() == Fiber::READY);
        }
        // 当前调度器找到一个任务，准备开始调度，将其从任务队列中删除，活动线程数+1
        task = *it;
        m_tasks.erase(it++);
        ++m_activeThreadCount;
        break;
      }
      // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
      tickle_me |= (it != m_tasks.end());
      m_mutex.unlock();
    }

    if (tickle_me)
    {
      tickle();
    }

    if (task.fiber)
    {
      // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算
      // 完成了，活跃线程数-1
      task.fiber->resume();
      --m_activeThreadCount;
      task.reset();
    }
    else if (task.cb)
    {
      if (cb_fiber)
      {
        cb_fiber->reset(task.cb);
      }
      else
      {
        cb_fiber.reset(new Fiber(task.cb));
      }
      task.reset();
      cb_fiber->resume();
      --m_activeThreadCount;
      cb_fiber.reset();
    }
    else
    {
      // 进入到这个分支情况一定是任务队列空了，调用idle协程即可
      if (idle_fiber->getState() == Fiber::TERM)
      {
        // 如果调度器没有调度任务，那么idle协程会不停的resume/yield，不会结束，如果
        // idle结束了，那么一定是调度器停止了
        Debug("ifle fiber term");
        break;
      }
      ++m_idleThreadCount;
      idle_fiber->resume();
      --m_idleThreadCount;
    }
  }
  Debug("Schedule::run() exit");
}

void Scheduler::idle()
{

}

void Scheduler::stop()
{
  Debug("stop");
  if (stopping())
  {
    return;
  }
  m_stopping = true;

  if (m_userCaller)
  {
    assert(GetThis() == this);
  }
  else
  {
    assert(GetThis() != this);
  }

  for (size_t i = 0; i < m_threadCount; i++)
  {
    tickle();
  }

  if (m_rootFiber)
  {
    tickle();
  }

  if (m_rootFiber)
  {
    m_rootFiber->resume();
    Debug("m_rootFiber end");
  }

  std::vector<Thread::ptr> threads;
  {
    m_mutex.lock();
    threads.swap(m_threads);
    m_mutex.unlock();
  }
  for (auto &t : threads)
  {
    t->join();
  }
}

bool Scheduler::stopping()
{
  m_mutex.lock();
  bool result = m_stopping && m_tasks.empty()
                && m_activeThreadCount == 0;
  m_mutex.unlock();
  return result;
}