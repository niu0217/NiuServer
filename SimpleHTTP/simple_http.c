/* ************************************************************************
> File Name:     simple_http.c
> Author:        niu0217
> Created Time:  Sun 30 Jun 2024 10:18:47 AM CST
> Description:   
 ************************************************************************/

#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // chdir

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    printf("./a.out port path\n");
    return -1;
  }
  unsigned int port = atoi(argv[1]);
  chdir(argv[2]);  // 切换服务器的工作路径
  int lisendFd = initListenFd(port);  // 监听

  epollRun(lisendFd); // 运行
  
  return 0;
}