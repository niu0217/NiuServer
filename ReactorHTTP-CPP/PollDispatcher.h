/* ************************************************************************
> File Name:     PollDispatcher.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 05:03:52 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>
using namespace std;

class PollDispatcher : public Dispatcher
{
public:
  PollDispatcher(EventLoop* evloop);
  ~PollDispatcher();

  int add() override;
  int remove() override;
  int modify() override;
  int dispatch(int timeout = 2) override; // 单位: s

private:
  int m_maxfd;
  struct pollfd *m_fds;
  const int m_maxNode = 1024;
};