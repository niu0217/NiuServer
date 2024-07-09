/* ************************************************************************
> File Name:     Fiber.cpp
> Author:        niu0217
> Created Time:  Sun 07 Jul 2024 04:34:23 PM CST
> Description:   
 ************************************************************************/

#include "Fiber.h"
#include "Log.h"
#include "Scheduler.h"
#include <atomic>
#include <assert.h>
#include <stdio.h> // perror

const int g_fiber_stack_size = 128 * 1024;

// 当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
// 当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
static thread_local Fiber::ptr t_thread_fiber = nullptr;

static std::atomic<uint64_t> s_fiber_id { 0 };    // 保存下一个协程的id
static std::atomic<uint64_t> s_fiber_count{ 0 };  // 保存协程个数

class MallocStackAllocator
{
public:
  static void* Alloc(size_t size)
  {
    return malloc(size);
  }

  static void Dealloc(void* vp, size_t size)
  {
    return free(vp);
  }
};
using StackAllocator = MallocStackAllocator;

// 只用于创建线程的第一个协程，也就是线程主函数对应的协程
// 这个协程只能由GetThis()方法调用
Fiber::Fiber()
{
  SetThis(this);
  m_state = RUNNING;
  if (getcontext(&m_ctx))
  {
    perror("getcontext");
  }
  ++s_fiber_count;
  m_id = s_fiber_id++;  // 协程id从0开始，用完加1
  Debug("Fiber::Fiber() main id = %ld\n", m_id);
}

// 参数：
//   cb        协程入口函数
//   stacksize 栈大小，默认为128k
Fiber::Fiber(std::function<void()> cb,
        size_t stacksize,
        bool run_in_scheduler)
  : m_id(s_fiber_id++),
    m_cb(cb),
    m_runInScheduler(run_in_scheduler)
{
  ++s_fiber_count;  // 协程个数+1

  // 在堆上分配协程栈地址
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size;
  m_stack = StackAllocator::Alloc(m_stacksize);

  if (getcontext(&m_ctx))
  {
    perror("getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  makecontext(&m_ctx, &Fiber::MainFunc, 0);

  Debug("Fiber::Fiber() main id = %ld\n", m_id);
}

Fiber::~Fiber()
{
}

void Fiber::SetThis(Fiber *f)
{
  t_fiber = f;
}

// 如果是位于子协程，则返回一个指向子协程的智能指针
// 如果没有子协程，则创建一个主协程，并返回一个指向主协程的智能指针
Fiber::ptr Fiber::GetThis()
{
  if (t_fiber)
  {
    return t_fiber->shared_from_this();
  }

  Fiber::ptr main_fiber(new Fiber);
  assert(t_fiber == main_fiber.get());
  t_thread_fiber = main_fiber;
  return t_fiber->shared_from_this();
}

// 特别注意：我们实现的是非对称协程，因此执行resume()时的当前执行环境一定是
//         位于线程主协程中，所以这里的swapcontext操作的结果是把主协程的上下文
//         保存到t_thread_fiber->m_ctx中，并且激活子协程的上下文m_ctx
void Fiber::resume()
{
  assert(m_state != TERM && m_state != RUNNING);
  SetThis(this);
  m_state = RUNNING;
  
  // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
  if (m_runInScheduler)
  {
    if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx))
    {
      perror("swapcontext");
    }
  }
  else
  {
    if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx))
    {
      perror("swapcontext");
    }
  }
}

// 特别注意：我们实现的是非对称协程，因此运行yield()时的当前执行环境一定是位于子协程中。
//         所以这里的swapcontext是把子协程的上下文保存到m_ctx，同时从t_thread_fiber->m_ctx
//         获得主协程的上下文并激活
void Fiber::yield()
{
  assert(m_state == RUNNING || m_state == TERM);
  SetThis(t_thread_fiber.get());
  if (m_state != TERM)
  {
    m_state = READY;
  }

  if (m_runInScheduler)
  {
    if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx)))
    {
      perror("swapcontext");
    }
  }
  else
  {
    if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)))
    {
      perror("swapcontext");
    }
  }
}

// 主要作用：封装协程入口函数，实现协程在结束时自动执行yield操作
void Fiber::MainFunc()
{
  Fiber::ptr cur = GetThis(); // GetThis的shared_from_this()让引用计数+1
  assert(cur != nullptr);

  cur->m_cb();   // 这里真正执行协程的入口函数
  cur->m_cb = nullptr;
  cur->m_state = TERM;

  auto raw_ptr = cur.get();
  cur.reset();  // 手动让t_fiber的引用计数-1
  raw_ptr->yield();  // 协程结束时自动yield，为了回到主协程
}

void Fiber::reset(std::function<void()> cb)
{
  assert(m_stack != nullptr);
  assert(m_state = TERM);
  m_cb = cb;
  if (getcontext(&m_ctx))
  {
    perror("getcontext");
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = READY;
}

uint64_t Fiber::TotalFibers()
{
  return s_fiber_count;
}

uint64_t Fiber::GetFiberId()
{
  return s_fiber_id;
}