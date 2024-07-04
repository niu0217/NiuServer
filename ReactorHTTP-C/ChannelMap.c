/* ************************************************************************
> File Name:     ChannelMap.c
> Author:        niu0217
> Created Time:  Mon 01 Jul 2024 10:40:08 AM CST
> Description:   
 ************************************************************************/

#include "ChannelMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// size 初始为128；不够调用 makeMapRoom 扩容
struct ChannelMap* channelMapInit(int size)
{
  struct ChannelMap *map = (struct ChannelMap*)
                           malloc(sizeof(struct ChannelMap));
  map->size = size;
  map->list = (struct Channel**)
              malloc(size * sizeof(struct Channel*));
  return map;
}

void channelMapClear(struct ChannelMap *map)
{
  if (map != NULL)
  {
    for (int i = 0; i < map->size; i++)
    {
      if (map->list[i] != NULL)  // 说明使用过malloc申请内存
      {
        free(map->list[i]);
        map->list[i] = NULL;
      }
    }
    free(map->list);
    map->list = NULL;
  }
  map->size = 0;
}

bool makeMapRoom(struct ChannelMap *map, int newSize, int unitSize)
{
  if (map->size < newSize)
  {
    int curSize = map->size;
    while (curSize < newSize)
    {
      curSize *= 2;
    }
    struct Channel **temp = realloc(map->list, curSize * unitSize);
    if (temp == NULL)
    {
      return false;
    }
    map->list = temp;
    memset(&map->list[map->size], 0, (curSize - map->size) * unitSize);
    map->size = curSize;
  }
  return true;
}