#include <iostream>

#include "echo-server.pb.h"
#include "echo-server.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using echoserver::EchoServer;
using echoserver::HelloReply;
using echoserver::HelloRequest;


// Logic and data behind the server's behavior.
class EchoServiceImpl final : public EchoServer::Service {

  Status Echo(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
    std::string prefix("Echoing: ");

    reply->set_message(prefix + request->name());
    return grpc::Status::OK;
  }

};


// actually, this will be happening on the dispatcher?
int main(int argc, char* argv[]) {
    ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

    EchoServiceImpl my_service;
    builder.RegisterService(&my_service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    server->Wait();
    
    return 0;
}