
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
using dummyserver::StatefulDummyServer;
using dummyserver::DummyRequest;
using dummyserver::DummyReply;

// Simple POD struct used as an argument wrapper for calls
struct CallDataStruct {
  StatefulDummyServer::AsyncService* service_;
  ServerCompletionQueue* cq_;
  string globalInfo;
};

// Base class used to cast the void* tags we get from
// the completion queue and call Proceed() on them.
class Call {
 public:
  virtual void Proceed() = 0;
  virtual bool isDone() = 0;
};

class SetStateCall final : public Call {
 public:
  explicit SetStateCall(CallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestSetState(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST:
        new SetStateCall(data_);
        
        data_->globalInfo = request_.param1();
        reply_.set_answer("done");

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        break;

      case FINISH:
        delete this;
        break;
    }
  }

  bool isDone() {
    return status_ == FINISH;
  }

 private:
  CallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<DummyReply> responder_;
  DummyRequest request_;
  DummyReply reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};

class DoStuffCall final : public Call {
 public:
  explicit DoStuffCall(CallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestStatefulDoStuff(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST: {
        new DoStuffCall(data_);
        
        // The actual processing.
        long long sum = 0;
        for (long long i = 0; i < 100000000; i++) {
            sum = 3 * i + 1;
        }
        reply_.set_answer(data_->globalInfo + std::to_string(sum) + request_.param1());

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        break;
      }
      case FINISH:
        delete this;
        break;
    }
  }

  bool isDone() {
    return (status_ == FINISH);
  }

 private:
  CallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<DummyReply> responder_;
  DummyRequest request_;
  DummyReply reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};


class StatefulDummyServerImp final {
 public:
  ~StatefulDummyServerImp() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(DISPATCHER_ADDR_STATEFUL_1, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    
    // assemble the server.
    server_ = builder.BuildAndStart();
    cout << "Server listening on " << DISPATCHER_ADDR_STATEFUL_1 << endl;

    HandleRpcs();
  }

 private:

  void HandleRpcs() {
    
    // Spawn a new CallData instance to serve new clients.
    CallDataStruct data{&service_, cq_.get(), global_info_};
    new SetStateCall(&data);
    new DoStuffCall(&data);
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
        Call* curr = static_cast<Call*>(tag);
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        if (curr->isDone()) {
          curr->Proceed();
          continue;
        }
        std::thread t(&StatefulDummyServerImp::RunAndGatherData, this, curr, start_time);
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

  void RunAndGatherData(Call* toRun, std::chrono::high_resolution_clock::time_point start_time) {

    struct rusage usage_stats;

    std::thread t(&StatefulDummyServerImp::runWrapper, this, toRun, &usage_stats, start_time);
    t.join();
    
    // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
    float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                            + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
    float mem_used = usage_stats.ru_maxrss / 1000;
    cout << "got runtime: " << runtime << " with wall clock time passed being " << time_since_(start_time) << ", mem used (in MB): " << mem_used << endl;  

  }

  void runWrapper(Call* toRun, struct rusage* usage_stats, std::chrono::high_resolution_clock::time_point start_time) {


    toRun->Proceed();

    getrusage(RUSAGE_THREAD, usage_stats);

  }

  int time_since_(std::chrono::high_resolution_clock::time_point since) {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  StatefulDummyServer::AsyncService service_;
  std::unique_ptr<Server> server_;
  string global_info_;
};

