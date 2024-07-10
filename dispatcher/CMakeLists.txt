


# Add Library target with protobuf sources
add_library(dummyproto dummy.proto)
target_link_libraries(dummyproto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++
)
target_include_directories(dummyproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})


# Compile protobuf and grpc files in myproto target to cpp
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET dummyproto LANGUAGE cpp)
protobuf_generate(TARGET dummyproto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")



# ======= code =======

add_executable(dispatcher dispatcher.cc)
target_link_libraries(dispatcher util dummyproto protobuf::libprotobuf gRPC::grpc gRPC::grpc++)

# target_link_libraries(dispatcher PRIVATE util)


