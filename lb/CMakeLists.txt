
# ======= code =======

add_executable(lb lb.cc)
target_link_libraries(lb util mainproto protobuf::libprotobuf gRPC::grpc gRPC::grpc++)

# target_link_libraries(lb PRIVATE )

# target_link_libraries(lb PRIVATE util)
