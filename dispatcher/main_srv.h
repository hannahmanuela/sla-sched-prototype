
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/resource.h>


#include <grpcpp/grpcpp.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

using namespace std;

#include "consts.h"
#include "queue.h"
#include "proc.h"
#include "main.pb.h"
#include "main.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using mainserver::MainServer;
using mainserver::ProcInfo;
using mainserver::PlacementReply;

// Simple POD struct used as an argument wrapper for calls
struct MainCallDataStruct {
  MainServer::AsyncService* service_;
  ServerCompletionQueue* cq_;
  Queue* proc_queue;
};

// Base class used to cast the void* tags we get from
// the completion queue and call Proceed() on them.
class Call {
 public:
  virtual void Proceed() = 0;
  virtual bool isDone() = 0;
};

class OkToPlaceCall final : public Call {
 public:
  explicit OkToPlaceCall(MainCallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestOkToPlace(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST:
        new OkToPlaceCall(data_);

        // TODO: check mem usage
        cout << "running ok to place check w/ curr q length of " << data_->proc_queue->get_qlen() << endl;
        reply_.set_oktoplace(data_->proc_queue->ok_to_place(request_.compdeadline(), request_.compceil()));
        reply_.set_ratio(data_->proc_queue->get_max_ratio());

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
  MainCallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<PlacementReply> responder_;
  ProcInfo request_;
  PlacementReply reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};


class MainServerImp final {
 public:
  ~MainServerImp() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run(string port, Queue* q) {
    ServerBuilder builder;
    string dispatcher_addr = "0.0.0.0:" + port;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(dispatcher_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    
    // assemble the server.
    server_ = builder.BuildAndStart();
    cout << "Server listening on " << dispatcher_addr << endl;

    proc_queue_ = q;

    HandleRpcs();
  }

 private:

  void HandleRpcs() {
    
    // Spawn a new CallData instance to serve new clients.
    MainCallDataStruct data{&service_, cq_.get(), proc_queue_};
    new OkToPlaceCall(&data);
    void* tag;  // uniquely identifies a request.
    bool ok;
    std::vector<std::thread> threads;
    while (true) {
      if (cq_->Next(&tag, &ok) && ok) {
        Call* curr = static_cast<Call*>(tag);
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        if (curr->isDone()) {
          curr->Proceed();
          continue;
        }
        // run this in a new thread
        std::thread t(&MainServerImp::RunAndGatherData, this, curr, start_time);
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

    std::thread t(&MainServerImp::runWrapper, this, toRun, &usage_stats, start_time);
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
  MainServer::AsyncService service_;
  std::unique_ptr<Server> server_;
  Queue* proc_queue_;
};

