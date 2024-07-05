/* ************************************************************************
> File Name:     SelectDispatcher.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 05:04:21 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/select.h>
using namespace std;

class SelectDispatcher : public Dispatcher
{
public:
  SelectDispatcher(EventLoop* evloop);
  ~SelectDispatcher();

  int add() override;
  int remove() override;
  int modify() override;
  int dispatch(int timeout = 2) override; // 单位: s

private:
  void setFdSet();
  void clearFdSet();

private:
  fd_set m_readSet;
  fd_set m_writeSet;
  const int m_maxSize = 1024;
};