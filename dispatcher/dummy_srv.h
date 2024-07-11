
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/resource.h>


#include <grpcpp/grpcpp.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

using namespace std;

#include "consts.h"
#include "dummy.pb.h"
#include "dummy.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using dummyserver::DummyServer;
using dummyserver::DummyRequest;
using dummyserver::DummyReply;

class DummyServerImp final {
 public:
  ~DummyServerImp() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run(int num_threads) {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(DISPATCHER_ADDR, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    
    // assemble the server.
    server_ = builder.BuildAndStart();
    cout << "Server listening on " << DISPATCHER_ADDR << endl;

    HandleRpcs();
  }

 private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData {
   public:
    // Take in the "service" instance (in this case representing an asynchronous
    // server) and the completion queue "cq" used for asynchronous communication
    // with the gRPC runtime.
    CallData(DummyServer::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        status_ = PROCESS;

        //  reusing threads might be a problem, given lag/eligibility calculations...
        // could just start a thread in here if we wanted to? how would that work?

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different CallData
        // instances can serve different requests concurrently), in this case
        // the memory address of this CallData instance.
        service_->RequestDoStuff(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
      } else if (status_ == PROCESS) {
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallData(service_, cq_);

        cout << "running in thread " << gettid() << endl;

        // The actual processing.
        std::string prefix("Hello ");
        int to_factor = 99995349;
        std::vector<int> factors;

        for (int i = 1; i <= to_factor; i++) {
            if (to_factor % i == 0) {
                factors.push_back(i);
            }
        }
        reply_.set_answer(prefix + request_.param1());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        cout << "processing in thread: " << gettid() << endl;

        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        
      } else {
        CHECK_EQ(status_, FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    DummyServer::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    DummyRequest request_;
    // What we send back to the client.
    DummyReply reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<DummyReply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  void HandleRpcs() {
    // Spawn a new CallData instance to serve new clients.
    new CallData(&service_, cq_.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    std::vector<std::thread> threads;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      // TODO: can use the tag to host different services? (here: https://groups.google.com/g/grpc-io/c/bXMmfah57h4/m/EUg2B8o1AgAJ)
      if (cq_->Next(&tag, &ok) && ok) {
        // run this in a new thread
        cout << "got something!" << endl;
        CallData* curr = static_cast<CallData*>(tag);
        std::thread t(&DummyServerImp::RunAndGatherData, this, curr);
        threads.push_back(std::move(t));
      } else {
        cout << "closing b/c returned false" << endl;
        break;
      }      
    }
    for (std::thread& t : threads) {
      t.join();
    }
  }

  void RunAndGatherData(CallData* toRun) {

    struct rusage usage_stats;

    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    std::thread t(&DummyServerImp::runWrapper, this, toRun, &usage_stats);
    t.join();
    
    // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
    // float runtime = (usage_stats.ru_utime.tv_usec + usage_stats.ru_stime.tv_usec)/1000;
    float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                            + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
    float mem_used = usage_stats.ru_maxrss / 1000;
    cout << "got runtime: " << runtime << " with wall clock time passed being " << time_since_(start_time) << ", mem used (in MB): " << mem_used << endl;  

  }

  void runWrapper(CallData* toRun, struct rusage* usage_stats) {

    toRun->Proceed();

    getrusage(RUSAGE_THREAD, usage_stats);

  }

  int time_since_(std::chrono::high_resolution_clock::time_point since) {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  DummyServer::AsyncService service_;
  std::unique_ptr<Server> server_;
};

