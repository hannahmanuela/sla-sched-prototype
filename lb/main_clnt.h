#include <grpc/grpc.h>


#include "main.grpc.pb.h"


using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using mainserver::MainServer;
using mainserver::ProcInfo;
using mainserver::PlacementReply;

class MainClient {
 public:
  explicit MainClient(std::shared_ptr<Channel> channel)
      : stub_(MainServer::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  PlacementReply OkToPlace(float mem_usg, float comp_ceil, float deadline) {
    // Data we are sending to the server.
    ProcInfo request;
    request.set_compceil(comp_ceil);
    request.set_compdeadline(deadline);
    request.set_memusg(mem_usg);

    PlacementReply reply;
    ClientContext context;
    CompletionQueue cq;
    Status status;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    std::unique_ptr<ClientAsyncResponseReader<PlacementReply> > rpc(
        stub_->PrepareAsyncOkToPlace(&context, request, &cq));

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
    // GPR_ASSERT(cq.Next(&got_tag, &ok));

    // Verify that the result from "cq" corresponds, by its tag, our previous
    // request.
    // GPR_ASSERT(got_tag == (void*)1);
    // ... and that the request was completed successfully. Note that "ok"
    // corresponds solely to the request for updates introduced by Finish().
    // GPR_ASSERT(ok);

    return reply;
  }

 private:
  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<MainServer::Stub> stub_;
};
