/* ************************************************************************
> File Name:     Noncopyable.h
> Author:        niu0217
> Created Time:  Mon 08 Jul 2024 03:53:56 PM CST
> Description:   
 ************************************************************************/

#pragma once

// 对象无法拷贝,赋值
class Noncopyable
{
public:
  Noncopyable() = default;
  ~Noncopyable() = default;

  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator=(const Noncopyable&) = delete;
};