/* ************************************************************************
> File Name:     Util.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:38:28 PM CST
> Description:   
 ************************************************************************/

#include "Util.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>

namespace util
{

pid_t GetThreadId()
{
  return syscall(SYS_gettid);
}

uint64_t GetCurrentMS()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

}  // util