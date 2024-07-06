/* ************************************************************************
> File Name:     Buffer.h
> Author:        niu0217
> Created Time:  Fri 05 Jul 2024 09:04:59 PM CST
> Description:   
 ************************************************************************/

#pragma once

#include <string>
using namespace std;

class Buffer
{
public:
  Buffer(int size);
  ~Buffer();

  void extendRoom(int size);  // 扩容
  inline int writeableSize()  // 得到剩余的可写的内存容量
  {
    return m_capacity - m_writePos;
  }
  inline int readableSize()  // 得到剩余的可读的内存容量
  {
    return m_writePos - m_readPos;
  }

  int appendString(const char* data, int size);
  int appendString(const char* data);
  int appendString(const string data);
  int socketRead(int fd);

  char* findCRLF();  // 根据\r\n取出一行, 找到其在数据块中的位置, 返回该位置
  int sendData(int socket);    // 指向内存的指针

  inline char* data()  // 得到读数据的起始位置
  {
    return m_data + m_readPos;
  }
  inline int readPosIncrease(int count)
  {
    m_readPos += count;
    return m_readPos;
  }

private:
  int m_capacity;
  char* m_data;
  int m_readPos = 0;
  int m_writePos = 0;
};