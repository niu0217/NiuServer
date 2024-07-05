/* ************************************************************************
> File Name:     Channel.cpp
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 02:34:22 PM CST
> Description:   
 ************************************************************************/

#include "Channel.h"
#include <stdlib.h>

Channel::Channel(int fd, FDEvent events, 
                 handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, 
                 void* arg)
  : readCallback(readFunc),
    writeCallback(writeFunc),
    destroyCallback(destroyFunc),
    m_fd(fd),
    m_events((int)events),
    m_arg(arg)
{
}

void Channel::writeEventEnable(bool flag)
{
  if (flag)
  {
    m_events |= static_cast<int>(FDEvent::WriteEvent);
  }
  else
  {
    m_events = m_events & ~(int)FDEvent::WriteEvent;
  }
}

bool Channel::isWriteEventEnable()
{
  return m_events & (int)FDEvent::WriteEvent;
}