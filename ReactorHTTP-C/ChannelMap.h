/* ************************************************************************
> File Name:     ChannelMap.h
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 10:40:03 AM CST
> Description:   
 ************************************************************************/

#pragma once
#include <stdbool.h>

struct ChannelMap
{
  int size;
  struct Channel **list;
};

// 初始化
struct ChannelMap* channelMapInit(int size);
// 清空map
void channelMapClear(struct ChannelMap *map);
// 重新分配内存空间
bool makeMapRoom(struct ChannelMap *map, int newSize, int unitSize);