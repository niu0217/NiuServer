/* ************************************************************************
> File Name:     Channel.h
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 10:04:26 AM CST
> Description:   
 ************************************************************************/

#pragma once
#include <stdbool.h>

typedef int(*EventCallback)(void *arg);

enum FDEvent
{
  TimeOut = 0x01,
  ReadEvent = 0x02,
  WriteEvent = 0x04
};

struct Channel
{
  int fd;
  int events;
  EventCallback readCallback;
  EventCallback writeCallback;
  EventCallback destoryCallback;
  void *arg;
};

// 初始化一个Channel
struct Channel* channelInit(int fd, int events, 
                            EventCallback readCb, 
                            EventCallback writeCb, 
                            EventCallback destoryCb,
                            void *arg);
// 修改fd的写事件（检测或者不检测）
void enableWriteEvent(struct Channel* channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isEnableWrite(struct Channel *channel);