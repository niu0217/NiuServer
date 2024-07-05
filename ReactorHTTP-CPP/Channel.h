/* ************************************************************************
> File Name:     Channel.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 02:34:18 PM CST
> Description:   
 ************************************************************************/

#pragma once
#include <functional>

// 定义文件描述符的读写事件
// C++11 的强类型枚举
enum class FDEvent
{
  TimeOut = 0x01,
  ReadEvent = 0x02,
  WriteEvent = 0x04
};

class Channel
{
public:
  using handleFunc = std::function<int(void*)>;

  Channel(int fd, FDEvent events, 
          handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, 
          void* arg);

  void writeEventEnable(bool flag); // 修改fd的写事件(检测 or 不检测)
  bool isWriteEventEnable(); // 判断是否需要检测文件描述符的写事件

  inline int getEvent() const
  {
    return m_events;
  }
  inline int getSocket() const
  {
    return m_fd;
  }
  inline const void* getArg()
  {
    return m_arg;
  }

public:
  handleFunc readCallback;
  handleFunc writeCallback;
  handleFunc destroyCallback;

private:
  int m_fd;
  int m_events;
  void* m_arg;
};