/* ************************************************************************
> File Name:     Fiber.h
> Author:        niu0217
> Created Time:  Sun 07 Jul 2024 04:34:18 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include <iostream>
#include <memory>  // shared_ptr
#include <functional>  // function
#include <ucontext.h>  // ucontext_t

class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
  typedef std::shared_ptr<Fiber> ptr;
  
  enum State
  {
    READY,
    RUNNING,
    TERM
  };

private:
  Fiber();

public:
  Fiber(std::function<void()> cb,
        size_t stacksize = 0,
        bool run_in_scheduler = true);
  ~Fiber();

  // 重置协程状态和入口函数，复用栈空间，不重新创建栈
  void reset(std::function<void()> cb);
  // 功能：将当前协程切换到运行状态
  // 细节：当前协程和正在运行的协程进行交换，前者变为RUNNING，后者变为READY
  void resume();
  // 功能：当前协程让出执行权
  // 细节：当前协程与上次resume时退到后台的协程进行交换，前者变为READY，后者变为RUNNING
  void yield();

  uint64_t getId() const
  {
    return m_id;
  }
  State getState() const
  {
    return m_state;
  }

public:
  // 设置当前正在运行的协程，即设置线程局部变量t_fiber的值
  static void SetThis(Fiber *f);
  // 功能：返回当前线程正在执行的协程
  // 细节：如果当前线程还未创建协程，则创建线程的第一个协程，且该协程为当前
  //      线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程结束
  //      时，都要切回到主协程，由主协程重新选择新的协程进行resume
  // 注意：线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作
  //      以初始化主函数协程
  static Fiber::ptr GetThis();
  static void MainFunc();         // 协程入口函数
  static uint64_t TotalFibers();  // 获取总协程数
  static uint64_t GetFiberId();   // 获取当前协程id

private:
  uint64_t m_id = 0;           // 协程ID
  std::function<void()> m_cb;  // 协程入口函数
  bool m_runInScheduler;       // 本协程是否参与调度器调度
  uint32_t m_stacksize = 0;    // 协程栈大小 
  State m_state = READY;       // 协程状态
  ucontext_t m_ctx;            // 协程上下文
  void* m_stack = nullptr;     // 协程栈地址
};