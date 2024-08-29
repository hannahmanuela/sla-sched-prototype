


# Add Library target with protobuf sources
add_library(mainproto main.proto)
target_link_libraries(mainproto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++
)
target_include_directories(mainproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})


# Compile protobuf and grpc files in myproto target to cpp
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET mainproto LANGUAGE cpp)
protobuf_generate(TARGET mainproto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")



# ======= code =======

add_executable(dispatcher dispatcher.cc)
target_link_libraries(dispatcher util mainproto protobuf::libprotobuf gRPC::grpc gRPC::grpc++)

# target_link_libraries(dispatcher PRIVATE util)


