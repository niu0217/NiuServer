cmake_minimum_required(VERSION 2.6)

project(CServer)

# 设置编译选项
set(C_FLAGS
 -g
 -O0
 -Wall
 -Wextra
 -Werror # 把警告当成错误
 -Wno-unused-parameter # 函数中出现未使用的参数，不给出警告
 # -MMD
 -std=gnu11
 -rdynamic
 )
string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 设置头文件路径
include_directories(${PROJECT_SOURCE_DIR})

# 打印编译选项 -- 调试使用
message(STATUS "C_FLAGS = " ${CMAKE_C_FLAGS})

set(base_SRCS
 Channel.c
 ChannelMap.c
 SelectDispatcher.c
 PollDispatcher.c
 EpollDispatcher.c
 EventLoop.c
 WorkerThread.c
 ThreadPool.c
 Buffer.c
 TcpServer.c
 TcpConnection.c
 HttpRequest.c
 HttpResponse.c
 main.c
 )

 # 编译可执行文件
 add_executable(server ${base_SRCS})
 target_link_libraries(server pthread)