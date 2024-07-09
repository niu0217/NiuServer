/* ************************************************************************
> File Name:     Thread_test.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 05:01:38 PM CST
> Description:   
 ************************************************************************/

// 内存泄漏检测：valgrind --leak-check=full ./Thread_test

#include "../Thread.h"
#include <vector>
#include <string>
#include <stdio.h>
using namespace std;

void threadFunc()
{
  printf("hello niu0217\n");
}

void test()
{
  vector<Thread*> threads;
  for (int i = 0; i < 10; i++)
  {
    string name = "Thread " + to_string(i);
    Thread* t = new Thread(threadFunc, name);
    threads.push_back(t);
  }
  for (auto &t : threads)
  {
    t->join();
  }
  for (auto &t : threads)
  {
    delete t;
  }
}

int main()
{
  test();
}