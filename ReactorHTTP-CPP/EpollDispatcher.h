/* ************************************************************************
> File Name:     EpollDispatcher.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 04:16:16 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>
using namespace std;

class EpollDispatcher : public Dispatcher
{
public:
  EpollDispatcher(EventLoop* evloop);
  ~EpollDispatcher();

  int add() override;
  int remove() override;
  int modify() override;
  int dispatch(int timeout = 2) override; // 单位: s

private:
  int epollCtl(int op);

private:
  int m_epfd;
  struct epoll_event* m_events;
  const int m_maxNode = 520;
};