/* ************************************************************************
> File Name:     Scheduler.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 10:17:56 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Fiber.h"
#include "Mutex.h"
#include "Thread.h"
#include <vector>
#include <list>
#include <atomic>

// 功能：协程调度器
// 介绍：封装的是N-M的协程调度器
//      内部有一个线程池，支持协程在线程池里面切换
class Scheduler
{
public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;

  /// @brief 创建调度器
  /// @param threads 线程数
  /// @param use_caller 是否将当前线程也作为调度线程
  /// @param name 名称
  Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string &name = "Scheduler");
  virtual ~Scheduler();

  /// @brief 获取调度器名称
  const std::string& getName() const
  {
    return m_name;
  }

  /// @brief 获取当前线程调度器指针
  static Scheduler* GetThis();

  /// @brief 获取当前线程的主协程
  static Fiber* GetMainFiber();

  /// @brief 添加调度任务
  /// @tparam FiberOrCb 调度任务类型，可以是协程对象或函数指针
  /// @param fc 协程对象或指针
  /// @param thread 指定运行该任务的线程号，-1表示任意线程
  template<class FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1)
  {
    bool need_tickle = false;
    {
      MutexType::lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread);
    }
    if (need_tickle)
    {
      tickle();   // 唤醒idle协程
    }
  }

  /// @brief 启动调度器
  void start();
  
  /// @brief 停止调度器，等所有调度任务都执行完了再返回
  void stop();

protected:
  /// @brief 通知协程调度器有任务了
  virtual void tickle();

  /// @brief 协程调度函数
  void run();

  /// @brief 无任务调度时执行idle协程
  virtual void idle();

  /// @brief 返回是否可以停止
  virtual bool stopping();

  /// @brief 设置当前的协程调度器
  void setThis();

  /// @brief 返回是够有空闲线程
  /// @details 当调度协程进入idle时空闲线程数+1，从idle协程返回时空闲线程数-1
  bool hasIdleThreads()
  {
    return m_idleThreadCount > 0;
  }

private:
  template<class FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread)
  {
    bool need_tickle = m_tasks.empty();
    ScheduleTask ft(fc, thread);
    if(ft.fiber || ft.cb)
    {
      m_tasks.push_back(ft);
    }
    return need_tickle;
  }

private:
  /// @brief 调度任务，协程/函数二选一，可指定在哪个线程上调度
  struct ScheduleTask
  {
    Fiber::ptr fiber;
    std::function<void()> cb;
    int thread;

    ScheduleTask(Fiber::ptr f, int thr)
    {
      fiber = f;
      thread = thr;
    }
    ScheduleTask(Fiber::ptr *f, int thr)
    {
      fiber.swap(*f);
      thread = thr;
    }
    ScheduleTask(std::function<void()> f, int thr)
    {
      cb = f;
      thread = thr;
    }
    ScheduleTask()
    {
      thread = -1;
    }
    void reset()
    {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

private:
  std::string m_name;   // 协程调度器名称
  MutexType m_mutex;    // 互斥锁
  std::vector<Thread::ptr> m_threads;  // 线程池
  std::list<ScheduleTask> m_tasks;     // 任务队列
  std::vector<int> m_threadIds;        // 线程池的线程ID数组
  size_t m_threadCount = 0;            // 工作线程数量，不包含use_caller的主线程
  std::atomic<size_t> m_activeThreadCount = { 0 };  // 活跃线程数
  std::atomic<size_t> m_idleThreadCount = { 0 };    // idle线程数

  bool m_userCaller;      // 是否 use caller
  Fiber::ptr m_rootFiber; // use_caller=true时，调度器所在线程的调度协程
  int m_rootThread = 0;   // use_caller=true时，调度器所在线程的id

  bool m_stopping = false;  // 是否正在停止
};