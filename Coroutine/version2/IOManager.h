/* ************************************************************************
> File Name:     IOManager.h
> Author:        niu0217
> Created Time:  Tue 09 Jul 2024 08:59:44 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Scheduler.h"
#include "Timer.h"

class IOManager : public Scheduler,
                  public TimerManager
{
public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  enum Event
  {
    NONE    = 0x0,  /// 无事件
    READ    = 0x1,  /// 读事件(EPOLLIN)
    WRITE   = 0x4,  /// 写事件(EPOLLOUT)
  };

private:
  /// @brief socket fd上下文类
  /// @details 每个socket fd都对应一个FdContext，包括fd值，fd上的事件
  //           以及fd的读写事件上下文
  struct FdContext
  {
    typedef Mutex MutexType;

    /// @brief 事件上下文类
    /// @details fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调
    //           函数的调度器对fd事件做了简化，只预留了读事件和写事件，所有的事件都被
    //           归类到这两类事件中
    struct EventContext
    {
      Scheduler* scheduler = nullptr;  /// 事件执行的调度器
      Fiber::ptr fiber;  /// 事件协程
      std::function<void()> cb;  /// 事件的回调函数
    };

    /// @brief 获取事件上下文类
    /// @param event 事件类型
    /// @return 返回对应事件的上下文
    EventContext& getContext(Event event);

    /// @brief 重置事件上下文
    /// @param ctx 待重置的上下文类
    void resetContext(EventContext& ctx);

    /// @brief 触发事件
    /// @param event 事件类型
    void triggerEvent(Event event);

    /// 读事件上下文
    EventContext read;
    /// 写事件上下文
    EventContext write;
    /// 事件关联的句柄
    int fd = 0;
    /// 当前的事件
    Event events = NONE;
    /// 事件的Mutex
    MutexType mutex;
  };

public:
  /// @brief 构造函数
  /// @param threads 线程数量
  /// @param use_caller 是否将调用线程包含进去
  /// @param name 调度器的名称
  IOManager(size_t threads = 1,
            bool use_caller = true,
            const std::string& name = "");

  /// @brief 析构函数
  /// @details 要等Scheduler调度完所有的任务，然后再关闭epoll句柄和pipe句柄
  ///          然后释放所有的FdContext
  ~IOManager();

  /// @brief 添加事件
  /// @param fd 描述符发生了event事件时执行cb函数
  /// @param event 事件类型
  /// @param cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
  /// @return 添加成功返回0,失败返回-1
  int addEvent(int fd, Event event,
               std::function<void()> cb = nullptr);

  /// @brief 删除事件
  /// @param fd socket句柄
  /// @param event 事件类型
  /// @attention 不会触发事件
  /// @return 是否删除成功
  bool delEvent(int fd, Event event);

  /// @brief 取消事件
  /// @param fd socket句柄
  /// @param event 事件类型
  /// @return 如果事件存在则触发事件
  bool cancelEvent(int fd, Event event);

  /// @brief 取消所有事件
  /// @details 所有被注册的回调事件在cancel之前都会被执行一次
  /// @param fd socket句柄
  /// @return 是否删除成功
  bool cancelAll(int fd);

  /// @brief 返回当前的IOManager
  static IOManager* GetThis();

protected:
  /// @brief 通知调度器有任务要调度
  /// @details 写pipe让idle协程从epoll_wait退出，待idle协程yield之后Scheduler::run就
  ///          可以调度其他任务
  ///          如果当前没有空闲调度线程，那就没必要发通知
  void tickle() override;

  bool stopping() override;

  /// @brief idle协程
  /// @details 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应
  ///          的操作是tickle或注册的IO事件就绪。
  ///          * 调度器无调度任务时会阻塞在idle协程上，对IO调度器而言，idle状态应该关注两件事，
  ///            一是有没有新的调度任务，对应Scheduler::schedule()，如果有新的调度任务，那应该
  //             立即退出idle状态，并执行对应的任务；
  ///            二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行IO事件对应的回调函数
  void idle() override;

  void onTimerInsertedAtFront() override;

  /// @brief 重置socket句柄上下文的容器大小
  /// @param size 容量大小
  void contextResize(size_t size);

  /// @brief 判断是否可以停止
  /// @param timeout 最近要触发的定时器事件间隔
  /// @return 返回是否可以停止
  bool stopping(uint64_t& timeout);

private:
  int m_epfd = 0;  // epoll 文件句柄
  int m_tickleFds[2];  // pipe 文件句柄
  std::atomic<size_t> m_pendingEventCount = {0};  // 当前等待执行的事件数量
  RWMutexType m_mutex;  // IOManager的Mutex
  std::vector<FdContext*> m_fdContexts;  // socket事件上下文的容器
};