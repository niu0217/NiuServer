/* ************************************************************************
> File Name:     HttpRequest.h
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 10:35:58 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Buffer.h"
#include "HttpResponse.h"
#include <stdbool.h>
#include <map>
using namespace std;

// 当前的解析状态
enum class PrecessState:char
{
  ParseReqLine,
  ParseReqHeaders,
  ParseReqBody,
  ParseReqDone
};

// 定义http请求结构体
class HttpRequest
{
public:
  HttpRequest();
  ~HttpRequest();

  void reset();
  void addHeader(const string key, const string value);  // 添加请求头
  string getHeader(const string key);  // 根据key得到请求头的value
  bool parseRequestLine(Buffer* readBuf);  // 解析请求行
  bool parseRequestHeader(Buffer* readBuf);  // 解析请求头

  // 解析http请求协议
  bool parseHttpRequest(Buffer* readBuf,         // 需要解析的Http请求消息
                        HttpResponse* response,  
                        Buffer* sendBuf,         // 需要发送给客户端的响应消息
                        int socket);
  // 处理http请求协议
  bool processHttpRequest(HttpResponse* response);

  string decodeMsg(string from);  // 解码字符串 针对GET请求中的特殊字符进行处理
  const string getFileType(const string name);
  static void sendDir(string dirName, Buffer* sendBuf, int cfd);
  static void sendFile(string dirName, Buffer* sendBuf, int cfd);

  inline void setMethod(string method)
  {
    m_method = method;
  }
  inline void setUrl(string url)
  {
    m_url = url;
  }
  inline void setVersion(string version)
  {
    m_version = version;
  }
  
  // 获取处理状态
  inline PrecessState getState()
  {
    return m_curState;
  }
  inline void setState(PrecessState state)
  {
    m_curState = state;
  }

private:
  char* splitRequestLine(const char* start, 
                         const char* end,
                         const char* sub, 
                         function<void(string)> callback);
  int hexToDec(char c);

private:
  string m_method;
  string m_url;
  string m_version;
  map<string, string> m_reqHeaders;
  PrecessState m_curState;
};