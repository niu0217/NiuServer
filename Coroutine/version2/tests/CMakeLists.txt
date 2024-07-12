add_executable(Thread_test Thread_test.cpp ../Thread.cpp ../Util.cpp)
target_link_libraries(Thread_test pthread)