/* ************************************************************************
> File Name:     EventLoop.h
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 02:41:22 PM CST
> Description:   
 ************************************************************************/

#pragma once
#include <stdbool.h>
#include <pthread.h>
#include "Dispatcher.h"
#include "ChannelMap.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// 处理该节点中的channel的方式
enum ElemType{ ADD, DELETE, MODIFY };
// 定义任务队列的节点
struct ChannelElement
{
  int type;   // 如何处理该节点中的channel
  struct Channel* channel;
  struct ChannelElement* next;
};

struct Dispatcher;  // 前向声明
struct EventLoop
{
  bool isQuit;

  // dispatcher 可以指向 EpollDispatcher 或者 PollDispatcher
  // 或者 SelectDispatcher，而这三个当中的每一个都保存了相应
  // 的函数指针，也就是回调函数的概念。我们在EventLoop中调用
  // 相应的回调函数处理事件；
  struct Dispatcher* dispatcher;
  void* dispatcherData;

  // 任务队列
  struct ChannelElement *head;
  struct ChannelElement *tail;

  // map
  struct ChannelMap* channelMap;

  // 线程相关 id name mutex
  pthread_t threadID;
  char threadName[32];
  pthread_mutex_t mutex;

  // 存储本地通信的fd 通过socketpair 初始化
  int socketPair[2];
};

// 初始化
struct EventLoop* eventLoopInit();  // 主线程使用
struct EventLoop* eventLoopInitEx(const char* threadName); // 子线程使用

// 启动反应堆模型
int eventLoopRun(struct EventLoop* loop);

// 调用读写事件的回调函数
int eventActivate(struct EventLoop* loop, int fd, int event);

// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop* loop, struct Channel* channel, int type);

// 处理线程间通信
void taskWakeup(struct EventLoop* loop);
int readLocalMessage(void* arg);

// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* loop);
// 处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* loop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* loop, struct Channel* channel);
int eventLoopModify(struct EventLoop* loop, struct Channel* channel);

// 释放channel
int destroyChannel(struct EventLoop* loop, struct Channel* channel);