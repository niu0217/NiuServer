/* ************************************************************************
> File Name:     HttpResponse.h
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 10:37:40 AM CST
> Description:   
 ************************************************************************/

#pragma once

#include "Buffer.h"
#include <map>
#include <functional>
using namespace std;

// 定义状态码枚举
enum class StatusCode
{
  Unknown,
  OK = 200,
  MovedPermanently = 301,
  MovedTemporarily = 302,
  BadRequest = 400,
  NotFound = 404
};

// 定义结构体
class HttpResponse
{
public:
  function<void(const string, struct Buffer*, int)> sendDataFunc;

  HttpResponse();
  ~HttpResponse();

  /// @brief 将响应头数据保存到m_headers中
  void addHeader(const string key, const string value);

  /// @brief 将构造好的Http响应数据保存到sendBuf中，发送给socket对应的客户端
  /// @param sendBuf 保存构造好的Http响应数据
  /// @param socket 要发送的客户端的fd
  void prepareMsg(Buffer* sendBuf, int socket);  // 组织http响应数据
  
  inline void setFileName(string name)
  {
    m_fileName = name;
  }
  inline void setStatusCode(StatusCode code)
  {
    m_statusCode = code;
  }

private:
  StatusCode m_statusCode;
  string m_fileName;
  map<string, string> m_headers;
  // 定义状态码和描述的对应关系
  const map<int, string> m_info = {
    {200, "OK"},
    {301, "MovedPermanently"},
    {302, "MovedTemporarily"},
    {400, "BadRequest"},
    {404, "NotFound"},
  };
};

/* 一个简单Http响应的样子

HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 26

*/