project(sla-sched)
cmake_minimum_required(VERSION 3.22)

add_executable(static_page_get website-ex/static_page_get.cc)
add_executable(dynamic_page_get website-ex/dynamic_page_get.cc)
add_executable(data_process_fg website-ex/data_process_fg.cc)

add_executable(website-client website-ex/website-client.cc)

add_executable(dispatcher dispatcher.cc)

add_executable(lb lb.cc)
