cmake_minimum_required(VERSION 3.22)
project(sla-sched)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


add_subdirectory(echo-server-ex)


add_executable(dispatcher dispatcher.cc)
add_executable(lb lb.cc)
