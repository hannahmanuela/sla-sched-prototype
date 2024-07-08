#include "echo-server.pb.h"
#include "echo-server.grpc.pb.h"


#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>

#include <iostream>


int main(int argc, char* argv[]) {
    // Setup request
    echoserver::HelloRequest query;
    echoserver::HelloReply result;
    query.set_name("John");

    // Call
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    std::unique_ptr<echoserver::EchoServer::Stub> stub = echoserver::EchoServer::NewStub(channel);

    grpc::ClientContext context;
    grpc::Status status = stub->Echo(&context, query, &result);

    // Output result
    std::cout << "I got:" << result.message() << std::endl;

    return 0;
}