
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
using mainserver::WebsiteServer;
using mainserver::InpVal;
using mainserver::RetVal;

// Simple POD struct used as an argument wrapper for calls
struct WebsiteCallDataStruct {
  WebsiteServer::AsyncService* service_;
  ServerCompletionQueue* cq_;
};

class WebsiteCall {
 public:
  virtual void Proceed() = 0;
  virtual bool isDone() = 0;
  virtual ProcType getType() = 0;
  virtual float getDeadline() = 0;
  virtual float getCompCeil() = 0;
  virtual float getMemUsg() = 0;
};

class StaticGetCall final : public WebsiteCall {
 public:
  explicit StaticGetCall(WebsiteCallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestStaticGet(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST: {
        new StaticGetCall(data_);

        cout << "starting static get" << endl;
        
        // TODO: make this more real? 
        long long sum = 0;
        for (long long i = 0; i < 1000000000; i++) {
            sum = 3 * i + 1;
        }
        reply_.set_retval(sum);

        cout << "done w/ static get" << endl;
                

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
    return status_ == FINISH;
  }

  ProcType getType() {
    return STATIC_PAGE_GET;
  }
  float getDeadline() {
    return request_.procinfo().compdeadline();
  }
  float getCompCeil() {
    return request_.procinfo().compceil();
  }
  float getMemUsg() {
    return request_.procinfo().memusg();
  }

 private:
  WebsiteCallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<RetVal> responder_;
  InpVal request_;
  RetVal reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};

class DynamicGetCall final : public WebsiteCall {
 public:
  explicit DynamicGetCall(WebsiteCallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestDynamicGet(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST: {
        new DynamicGetCall(data_);
        
        // TODO: make this more real? 
        long long sum = 0;
        for (long long i = 0; i < 1000000000; i++) {
            sum = 3 * i + 1;
        }
        reply_.set_retval(sum);
                

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
    return status_ == FINISH;
  }

  ProcType getType() {
    return DYNAMIC_PAGE_GET;
  }
  float getDeadline() {
    return request_.procinfo().compdeadline();
  }
  float getCompCeil() {
    return request_.procinfo().compceil();
  }
  float getMemUsg() {
    return request_.procinfo().memusg();
  }

 private:
  WebsiteCallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<RetVal> responder_;
  InpVal request_;
  RetVal reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};

class ProcessFgCall final : public WebsiteCall {
 public:
  explicit ProcessFgCall(WebsiteCallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestProcessFg(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST: {
        new ProcessFgCall(data_);
        
        // TODO: make this more real? 
        long long sum = 0;
        for (long long i = 0; i < 1000000000; i++) {
            sum = 3 * i + 1;
        }
        reply_.set_retval(sum);
                

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
    return status_ == FINISH;
  }

  ProcType getType() {
    return DATA_PROCESS_FG;
  }
  float getDeadline() {
    return request_.procinfo().compdeadline();
  }
  float getCompCeil() {
    return request_.procinfo().compceil();
  }
  float getMemUsg() {
    return request_.procinfo().memusg();
  }

 private:
  WebsiteCallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<RetVal> responder_;
  InpVal request_;
  RetVal reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};

class ProcessBgCall final : public WebsiteCall {
 public:
  explicit ProcessBgCall(WebsiteCallDataStruct* data)
      : data_(data), responder_(&ctx_), status_(REQUEST) {
    data->service_->RequestProcessBg(&ctx_, &request_, &responder_, data_->cq_,
                              data_->cq_, this);
  }

  void Proceed() {

    switch (status_) {
      case REQUEST: {
        new ProcessBgCall(data_);
        
        // TODO: make this more real? 
        long long sum = 0;
        for (long long i = 0; i < 1000000000; i++) {
            sum = 3 * i + 1;
        }
        reply_.set_retval(sum);
                

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
    return status_ == FINISH;
  }

  ProcType getType() {
    return DATA_PROCESS_BG;
  }
  float getDeadline() {
    return request_.procinfo().compdeadline();
  }
  float getCompCeil() {
    return request_.procinfo().compceil();
  }
  float getMemUsg() {
    return request_.procinfo().memusg();
  }

 private:
  WebsiteCallDataStruct* data_;
  ServerContext ctx_;

  ServerAsyncResponseWriter<RetVal> responder_;
  InpVal request_;
  RetVal reply_;

  enum CallStatus { REQUEST, FINISH };
  CallStatus status_;
};


class WebsiteServerImp final {
 public:
  ~WebsiteServerImp() {
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
    WebsiteCallDataStruct data{&service_, cq_.get()};
    new StaticGetCall(&data);
    new DynamicGetCall(&data);
    void* tag;  // uniquely identifies a request.
    bool ok;
    std::vector<std::thread> threads;
    while (true) {
      if (cq_->Next(&tag, &ok) && ok) {
        WebsiteCall* curr = static_cast<WebsiteCall*>(tag);
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        if (curr->isDone()) {
          curr->Proceed();
          continue;
        }
        // run this in a new thread
        std::thread t(&WebsiteServerImp::RunAndGatherData, this, curr, start_time);
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

  void RunAndGatherData(WebsiteCall* toRun, std::chrono::high_resolution_clock::time_point start_time) {

    struct rusage usage_stats;
    pthread_t tid = pthread_self();

    Proc* proc = new Proc(toRun->getDeadline(), toRun->getCompCeil(), toRun->getMemUsg(), toRun->getType(), start_time, tid);
    cout << "website srv adding to proc q" << endl;
    proc_queue_->enq(proc);

    toRun->Proceed();

    cout << "time gotten func giving " << proc->time_gotten() << endl;

    proc_queue_->remove(proc);
    delete proc;
    
    getrusage(RUSAGE_THREAD, &usage_stats);
    
    // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
    float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                            + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
    float mem_used = usage_stats.ru_maxrss / 1000;
    cout << "rusage getting runtime: " << runtime << " with wall clock time passed being " << time_since_(start_time) << ", mem used (in MB): " << mem_used << endl; 

  }

  int time_since_(std::chrono::high_resolution_clock::time_point since) {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  WebsiteServer::AsyncService service_;
  std::unique_ptr<Server> server_;
  Queue* proc_queue_;
};

