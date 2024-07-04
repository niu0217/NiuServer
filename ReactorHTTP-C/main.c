/* ************************************************************************
> File Name:     main.c
> Author:        niu0217
> Created Time:  Thu 04 Jul 2024 04:17:24 PM CST
> Description:   
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"

int main()
{
  unsigned short port = 10000;
  chdir("../Sources/");

  // 启动服务器
  struct TcpServer* server = tcpServerInit(port, 4);
  tcpServerRun(server);
}