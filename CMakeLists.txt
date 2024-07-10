cmake_minimum_required(VERSION 3.22)
project(sla-sched)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
# find_package(Threads)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)



# fyi helpful github for cmakefiles wehre proto, clnt, srv all in diff folders: https://github.com/faaxm/exmpl-cmake-grpc/tree/master

add_subdirectory(util)

add_subdirectory(dispatcher)

add_subdirectory(lb)

