/* ************************************************************************
> File Name:     main.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 10:25:08 AM CST
> Description:   
 ************************************************************************/

#include "Scheduler.h"

void testFiber(int i)
{
  std::cout << "hello niu0217 " << i << std::endl; 
}

int main()
{
  Fiber::GetThis();  // 初始化当前线程的主协程

  Scheduler sc;  // 创建调度器

  // 添加调度任务
  // 创建了10个协程，执行任务testFiber
  for (int i = 0; i < 10; i++)
  {
    auto obj = std::bind(testFiber, i);
    Fiber::ptr fiber(new Fiber(obj));
    sc.schedule(fiber);
  }

  // 执行调度任务
  sc.run();
}