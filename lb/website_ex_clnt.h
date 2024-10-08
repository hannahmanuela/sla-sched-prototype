#include <grpc/grpc.h>
#include <absl/log/check.h>


#include "main.grpc.pb.h"


using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using mainserver::WebsiteServer;
using mainserver::ProcToRun;
using mainserver::RetVal;

class WebsiteClient {
 public:
  explicit WebsiteClient(std::shared_ptr<Channel> channel, int given_id)
      : stub_(WebsiteServer::NewStub(channel)), id(given_id) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  RetVal MakeCall(float mem_usg, float comp_ceil, float deadline, string type) {
    // Data we are sending to the server.
    ProcToRun request;
    request.set_typetorun(type);
    request.mutable_procinfo()->set_compceil(comp_ceil);
    request.mutable_procinfo()->set_compdeadline(deadline);
    request.mutable_procinfo()->set_memusg(mem_usg);


    RetVal reply;
    ClientContext context;
    CompletionQueue cq;
    Status status;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    std::unique_ptr<ClientAsyncResponseReader<RetVal> > rpc(
        stub_->PrepareAsyncMakeCall(&context, request, &cq));

    rpc->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the integer 1.
    rpc->Finish(&reply, &status, (void*)1);
    void* got_tag;
    bool ok = false;
    // Block until the next result is available in the completion queue "cq".
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or the cq_ is shutting down.
    CHECK(cq.Next(&got_tag, &ok));

    // Verify that the result from "cq" corresponds, by its tag, our previous
    // request.
    CHECK_EQ(got_tag, (void*)1);
    // ... and that the request was completed successfully. Note that "ok"
    // corresponds solely to the request for updates introduced by Finish().
    CHECK(ok);

    return reply;
  }

  int id;

 private:
  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<WebsiteServer::Stub> stub_;
};
