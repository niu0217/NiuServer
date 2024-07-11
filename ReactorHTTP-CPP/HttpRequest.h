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

  /// @brief 解析Http请求消息
  /// @param readBuf 保存从客户端读取到的Http请求数据
  /// @param response 构建一个响应数据结构体
  /// @param sendBuf 保存将要发送给客户端的Http响应数据
  /// @param socket 客户端的文件描述符
  bool parseHttpRequest(Buffer* readBuf,
                        HttpResponse* response,  
                        Buffer* sendBuf,
                        int socket);

  /// @brief 将Http响应数据保存到response中
  /// @param response 保存要发送给客户端的Http响应数据
  bool processHttpRequest(HttpResponse* response);

  string decodeMsg(string from);  // 解码字符串 针对GET请求中m_url的特殊字符进行处理
  const string getFileType(const string name);

  /// @brief 给客户端发送一个html的目录页面
  /// @param dirName 目录的名字
  /// @param sendBuf 发送给客户的html的目录页面保存在这里
  /// @param cfd 要发送的客户端fd
  static void sendDir(string dirName, Buffer* sendBuf, int cfd);

  /// @brief 将文件内容发送给客户端
  /// @param fileName 要发送给客户端的文件的名字
  /// @param sendBuf 保存发送给客户的数据缓冲区
  /// @param cfd 客户端的fd
  static void sendFile(string fileName, Buffer* sendBuf, int cfd);

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

/* 一个简单的Http请求

GET /index.html HTTP/1.1
Host: www.example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
Accept: text/html,application/xhtml+xml

*/