/* ************************************************************************
> File Name:     noncopyable.h
> Author:        niu0217
> Created Time:  Sat 13 Jul 2024 06:19:07 PM CST
> Description:   
 ************************************************************************/

#pragma once

namespace niu
{

/// @brief 对象无法拷贝,赋值
class Noncopyable
{
public:
  Noncopyable() = default;
  ~Noncopyable() = default;

  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator=(const Noncopyable&) = delete;
};

} // namespace niu