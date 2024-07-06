/* ************************************************************************
> File Name:     main.cpp
> Author:        niu0217
> Created Time:  Sat 06 Jul 2024 04:54:06 PM CST
> Description:   
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"

int main(int argc, char* argv[])
{
  unsigned short port = 10000;
  chdir("/home/ubuntu/GithubFile/NiuServer/Sources");
  // 启动服务器
  TcpServer* server = new TcpServer(port, 4);
  server->run();

  return 0;
}