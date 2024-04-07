project(sla-sched)
cmake_minimum_required(VERSION 3.22)

add_executable(static_page_get static_page_get.cc)

add_executable(dispatcher dispatcher.cc)

add_executable(website website.cc)

add_executable(lb lb.cc)
