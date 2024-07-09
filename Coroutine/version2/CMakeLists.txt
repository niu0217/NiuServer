cmake_minimum_required(VERSION 2.6)

project(Coroutine)

# 设置编译选项
set(CXX_FLAGS
 -g
 -O0
 -Wall
 -Wextra
 -Werror # 把警告当成错误
 -Wno-unused-parameter # 函数中出现未使用的参数，不给出警告
 # -MMD
 -std=c++11
 -rdynamic
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 设置头文件路径
include_directories(${PROJECT_SOURCE_DIR})

# 打印编译选项 -- 调试使用
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS})

set(base_SRCS
 Util.cpp
 Thread.cpp
 Fiber.cpp
 Scheduler.cpp
 main.cpp
 )

 # 编译可执行文件
 add_executable(coroutine ${base_SRCS})
 target_link_libraries(coroutine pthread)