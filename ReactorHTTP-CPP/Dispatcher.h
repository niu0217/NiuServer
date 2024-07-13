/* ************************************************************************
> File Name:     Dispatcher.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 03:29:12 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include <string>

class EventLoop;
class Dispatcher
{
public:
  Dispatcher(EventLoop* evloop);

  /// @attention 必须是虚函数 不然父类指针指向子类对象后，内存释放会有问题
  virtual ~Dispatcher();

  virtual int add();
  virtual int remove();
  virtual int modify();
  virtual int dispatch(int timeout = 2); // 单位: s

  /// @brief 将外面的channel设置给m_channel
  /// @param channel 一个封装好了的channel，需要我们处理这个channel
  inline void setChannel(Channel* channel)
  {
    m_channel = channel;
  }

protected:
  std::string m_name = std::string();
  Channel* m_channel;   // 通过setChannel()设置
  EventLoop* m_evLoop;  // 通过构造函数设置
};