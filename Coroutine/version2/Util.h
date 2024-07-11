/* ************************************************************************
> File Name:     Util.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 04:38:24 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include <pthread.h>
#include <unistd.h>
#include <cstdint>

namespace util
{

/// @brief 获取当前线程的ID
pid_t GetThreadId();

/// @brief 获取当前时间的毫秒
uint64_t GetCurrentMS();

/// @brief 获取当前时间的微秒
uint64_t GetCurrentUS();

}  // util