/* ************************************************************************
> File Name:     Util.cpp
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:38:28 PM CST
> Description:   
 ************************************************************************/

#include "Util.h"
#include <sys/syscall.h>
#include <unistd.h>

namespace util
{

pid_t GetThreadId()
{
  return syscall(SYS_gettid);
}

}  // util