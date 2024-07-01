/* ************************************************************************
> File Name:     Dispatcher.h
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 02:27:11 PM CST
> Description:   
 ************************************************************************/

#pragma once
#include "Channel.h"
#include "EventLoop.h"

// 一个 Dispatcher 属于一个 EventLoop
struct Dispatcher
{
  // init
  void* (*init)();
  // 添加
  int (*add)(struct Channel *channel, struct EventLoop *loop);
  // 删除
  int (*remove)(struct Channel *channel, struct EventLoop *loop);
  // 修改
  int (*modify)(struct Channel *channel, struct EventLoop *loop);
  // 事件检测
  int (*dispatch)(struct EventLoop *loop, int timeout);  // 单位：s
  // 清除数据（关闭fd或者释放内存）
  int (*clear)(struct EventLoop *loop);
};