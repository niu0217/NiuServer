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
#include <vector>

namespace util
{

/// @brief 获取当前线程的ID
pid_t GetThreadId();

/// @brief 获取当前的调用栈
/// @param bt 保存调用栈
/// @param size 最多返回层数
/// @param skip 跳过栈顶的层数
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/// @brief 获取当前栈信息的字符串
/// @param size 栈的最大层数
/// @param skip 跳过栈顶的层数
/// @param prefix 栈信息前输出的内容
std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string& prefix = "");

/// @brief 获取当前时间的毫秒
uint64_t GetCurrentMS();

/// @brief 获取当前时间的微秒
uint64_t GetCurrentUS();

}  // util