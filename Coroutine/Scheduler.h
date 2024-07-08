/* ************************************************************************
> File Name:     Scheduler.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 10:17:56 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Fiber.h"
#include <list>

class Scheduler
{
public:
  void schedule(Fiber::ptr task)
  {
    m_tasks.push_back(task);
  }
  void run()
  {
    Fiber::ptr task;
    auto it = m_tasks.begin();
    while (it != m_tasks.end())
    {
      task = *it;
      m_tasks.erase(it++);
      task->resume();
    }
  }
private:
  std::list<Fiber::ptr> m_tasks;
};