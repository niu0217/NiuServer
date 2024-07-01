/* ************************************************************************
> File Name:     EventLoop.h
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 02:41:22 PM CST
> Description:   
 ************************************************************************/

#pragma once
#include "Dispatcher.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

struct EventLoop
{
  // dispatcher 可以指向 EpollDispatcher 或者 PollDispatcher
  // 或者 SelectDispatcher，而这三个当中的每一个都保存了相应
  // 的函数指针，也就是回调函数的概念。我们在EventLoop中调用
  // 相应的回调函数处理事件；
  struct Dispatcher* dispatcher;
  void* dispatcherData;
};