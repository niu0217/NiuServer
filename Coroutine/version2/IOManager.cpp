/* ************************************************************************
> File Name:     IOManager.cpp
> Author:        niu0217
> Created Time:  Tue 09 Jul 2024 08:59:48 PM CST
> Description:   
 ************************************************************************/

#include "IOManager.h"
#include "Log.h"
#include "Macro.h"
#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

IOManager::IOManager(size_t threads,
                     bool use_caller,
                     const std::string& name)
  : Scheduler(threads, use_caller, name)
{
  m_epfd = epoll_create(5000);
  assert(m_epfd > 0);

  // m_tickleFds[0] 读端
  // m_tickleFds[1] 写端
  int rt = pipe(m_tickleFds);
  assert(!rt);

  // 注册pipe读句柄的可读事件，用于tickle调度协程
  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = m_tickleFds[0];

  // 设置m_tickleFds[0]的非阻塞方式，配合边缘触发
  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  assert(!rt);

  // 将m_tickleFds[0]加入epoll多路复用，如果管道可读，idle中的
  // epoll_wait会返回
  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  assert(!rt);

  contextResize(32);

  // 这里直接开启了Scheduler，也就是说IOManager创建即可调度协程
  start();
}

IOManager::~IOManager()
{
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  for(size_t i = 0; i < m_fdContexts.size(); ++i)
  {
    if(m_fdContexts[i])
    {
      delete m_fdContexts[i];
    }
  }
}


void IOManager::tickle()
{
  if(!hasIdleThreads())
  {
    return;
  }
  int rt = write(m_tickleFds[1], "T", 1);
  assert(rt == 1);
}

bool IOManager::stopping()
{
  // 对于IOManager而言，必须等所有待调度的IO事件都执行完了才可以退出
  return m_pendingEventCount == 0 && Scheduler::stopping();
}

void IOManager::idle()
{
  Debug("idle");

  // 一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮
  // epoll_wait继续处理
  const uint64_t MAX_EVNETS = 256;
  epoll_event* events = new epoll_event[MAX_EVNETS]();
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
      delete[] ptr;
  });

  while(true)
  {
    if (stopping())
    {
      Debug("name = %s idle stopping exit", getName());
    }

    int rt = 0;
    static const int MAX_TIMEOUT = 3000;
    rt = epoll_wait(m_epfd, events, MAX_EVNETS, MAX_TIMEOUT);
    if (rt < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      break;
    }

    // 遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，
    // 进行事件处理
    for(int i = 0; i < rt; ++i)
    {
      epoll_event& event = events[i];
      if(event.data.fd == m_tickleFds[0])
      {
        uint8_t dummy[256];
        // m_tickleFds[0]用于通知协程调度，这时只需要把管道中的内容读完，
        // 本轮Scheduler::run()会重新执行协程调度
        while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
          ;
        continue;
      }

      FdContext* fd_ctx = (FdContext*)event.data.ptr;
      FdContext::MutexType::Lock lock(fd_ctx->mutex);
      // EPOLLERR 出错，比如写读端已经关闭的pipe
      // EPOLLHUP 套接字对端关闭
      // 出现这两种事件之一，应该同时触发fd的读和写事件，否则有可能出现注册
      // 的事件永远执行不到的情况
      if(event.events & (EPOLLERR | EPOLLHUP))
      {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
      }
      int real_events = NONE;
      if(event.events & EPOLLIN)
      {
        real_events |= READ;
      }
      if(event.events & EPOLLOUT)
      {
        real_events |= WRITE;
      }

      if((fd_ctx->events & real_events) == NONE)
      {
        continue;
      }

      // 删除已经发生的事件，将剩下的事件重新加入epoll_wait
      // 如果剩下的事件为0，表示这个fd已经不需要关注了，直接从epoll删除
      int left_events = (fd_ctx->events & ~real_events);
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if(rt2)
      {
        Debug("epoll_ctl error");
        continue;
      }

      // 处理已经发生的事件，也就是让调度器调度指定的函数或协程
      if(real_events & READ)
      {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }
      if(real_events & WRITE)
      {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }

    // 上面triggerEvent只是把对应的fiber重新加入调度，要执行的话还要等idle协程
    // 退出
    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->yield();
  }
}

void IOManager::contextResize(size_t size)
{
  m_fdContexts.resize(size);

  for(size_t i = 0; i < m_fdContexts.size(); ++i)
  {
    if(!m_fdContexts[i])
    {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
  FdContext* fd_ctx = nullptr;
  RWMutexType::ReadLock lock(m_mutex);
  if((int)m_fdContexts.size() > fd)
  {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  }
  else
  {
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  // 同一个fd不允许重复添加相同的事件
  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(fd_ctx->events & event))
  {
    assert(!(fd_ctx->events & event));
  }

  // 将新的事件加入epoll_wait
  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if(rt)
  {
    return -1;
  }

  ++m_pendingEventCount;

  fd_ctx->events = (Event)(fd_ctx->events | event);
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  assert(!event_ctx.scheduler
         && !event_ctx.fiber
         && !event_ctx.cb);

  // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
  event_ctx.scheduler = Scheduler::GetThis();
  if(cb)
  {
    event_ctx.cb.swap(cb);
  }
  else
  {
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::RUNNING
                  ,"state=" << event_ctx.fiber->getState());
  }
  return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
  // 找到fd对应的FdContext
  RWMutexType::ReadLock lock(m_mutex);
  if((int)m_fdContexts.size() <= fd)
  {
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(!(fd_ctx->events & event)))
  {
    return false;
  }

  // 清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从
  // epoll_wait中删除该文件描述符
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if(rt)
  {
    return false;
  }

  --m_pendingEventCount;

  // 重置该fd对应的event事件上下文
  fd_ctx->events = new_events;
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
  // 找到fd对应的FdContext
  RWMutexType::ReadLock lock(m_mutex);
  if((int)m_fdContexts.size() <= fd)
  {
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(SYLAR_UNLIKELY(!(fd_ctx->events & event)))
  {
    return false;
  }

  // 删除事件
  Event new_events = (Event)(fd_ctx->events & ~event);
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if(rt)
  {
    return false;
  }

  // 删除之前触发一次事件
  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

bool IOManager::cancelAll(int fd)
{
  // 找到fd对应的FdContext
  RWMutexType::ReadLock lock(m_mutex);
  if((int)m_fdContexts.size() <= fd)
  {
    return false;
  }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if(!fd_ctx->events)
  {
    return false;
  }

  // 删除全部事件
  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if(rt)
  {
    return false;
  }

  // 触发全部已注册的事件
  if(fd_ctx->events & READ)
  {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if(fd_ctx->events & WRITE)
  {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }

  SYLAR_ASSERT(fd_ctx->events == 0);
  return true;
}