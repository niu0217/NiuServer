/* ************************************************************************
> File Name:     Server.h
> Author:        niu0217
> Created Time:  Sun 30 Jun 2024 10:18:18 AM CST
> Description:   
 ************************************************************************/

#pragma once

int initListenFd(unsigned short port);
int epollRun(int listenFd);
void* acceptClient(void* arg); 
void* recvHttpRequest(void* arg);
int parseRequestLine(const char* line, int connFd);
int sendFile(const char* fileName, int connFd);
// 发送响应头(状态行+响应头)
int sendHeadMsg(int connFd, int status, const char* descr, const char* type, int length);
const char* getFileType(const char* name);
// 发送目录
int sendDir(const char* dirName, int connFd);

int hexToDec(char c);
void decodeMsg(char* to, char* from);  // 解析特殊字符