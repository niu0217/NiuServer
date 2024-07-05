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
using namespace std;

class EventLoop;
class Dispatcher
{
public:
  Dispatcher(EventLoop* evloop);
  virtual ~Dispatcher();

  virtual int add();
  virtual int remove();
  virtual int modify();
  virtual int dispatch(int timeout = 2); // 单位: s
  
  inline void setChannel(Channel* channel)
  {
    m_channel = channel;
  }

protected:
  string m_name = string();
  Channel* m_channel;
  EventLoop* m_evLoop;
};