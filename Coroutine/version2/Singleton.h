/* ************************************************************************
> File Name:     Singleton.h
> Author:        niu0217
> Created Time:  Fri 12 Jul 2024 10:43:13 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include <memory>

template<class T, class X, int N>
T& GetInstanceX()
{
  static T v;
  return v;
}

template<class T, class X, int N>
std::shared_ptr<T> GetInstancePtr()
{
  static std::shared_ptr<T> v(new T);
  return v;
}

/// @brief 单例模式封装类
/// @tparam T 类型
/// @tparam X 为了创造多个实例对应的Tag
/// @tparam N 同一个Tag创造多个实例索引
template<class T, class X = void, int N = 0>
class Singleton
{
public:
  static T* GetInstance()
  {
    static T v;
    return &v;
  }
};

/// @brief 单例模式智能指针封装类
/// @tparam T 类型
/// @tparam X 为了创造多个实例对应的Tag
/// @tparam N 同一个Tag创造多个实例索引
template<class T, class X = void, int N = 0>
class SingletonPtr
{
public:
  static std::shared_ptr<T> GetInstance()
  {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};