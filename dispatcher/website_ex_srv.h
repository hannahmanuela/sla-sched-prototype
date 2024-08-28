
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/resource.h>
#include <sys/syscall.h>


#include <grpcpp/grpcpp.h>
#include <absl/log/check.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

using namespace std;

#include "consts.h"
#include "queue.h"
#include "proc.h"
#include "main.pb.h"
#include "main.grpc.pb.h"

using mainserver::WebsiteServer;
using mainserver::ProcToRun;
using mainserver::RetVal;


struct sched_attr {
    uint32_t size;

    uint32_t sched_policy;
    uint64_t sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    int32_t sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    uint32_t sched_priority;

    /* SCHED_DEADLINE (nsec) */
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

class WebsiteServerImp final {
 public:
  ~WebsiteServerImp() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  WebsiteServerImp(Queue* q, float* curr_util, std::mutex& util_lock) : proc_queue_(q), curr_util_(curr_util), util_lock_(util_lock) {}

  // There is no shutdown handling in this code.
  void Run(string port) {
    ServerBuilder builder;
    string disp_ip = DISPATCHER_IP_ADDR;
    string dispatcher_addr = disp_ip + ":" + port;
    builder.AddListeningPort(dispatcher_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    
    cq_ = builder.AddCompletionQueue();
    
    server_ = builder.BuildAndStart();

    HandleRpcs();
  }

 private:

 // Class encompasing the state and logic needed to serve a request.
  class CallData {
   public:
    CallData(WebsiteServer::AsyncService * service, ServerCompletionQueue* cq, Queue* q, float *curr_util, std::mutex& util_lock)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE), p_q_(q), curr_util_(curr_util), util_lock_(util_lock) {
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        status_ = PROCESS;

        service_->RequestMakeCall(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
      } else if (status_ == PROCESS) {
        new CallData(service_, cq_, p_q_, curr_util_, util_lock_);

        auto start_time = std::chrono::high_resolution_clock::now();

        // run everything else on any core >= 2
        cpu_set_t  mask;
        CPU_ZERO(&mask);
        int num_cores = std::thread::hardware_concurrency() / 2;
        for (int i = 2; i < num_cores; i++) {
          CPU_SET(i, &mask);
        }
        if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
            cout << "set affinity had an error" << endl;
        }

        // generate Proc
        pthread_t tid = pthread_self();
        Proc* proc = new Proc(request_.procinfo().compdeadline(), request_.procinfo().compceil(), 
            request_.procinfo().memusg(), strToPt(request_.typetorun()), start_time, tid);
        p_q_->enq(proc);


        // set slice
        struct sched_attr attr;
        int ret = syscall(SYS_sched_getattr, gettid(), &attr, sizeof(attr), 0);;
        if (ret < 0) {
            perror("ERROR: sched_getattr");
        }
        attr.sched_runtime = request_.procinfo().compdeadline() * NSEC_PER_MSEC;
        ret = syscall(SYS_sched_setattr, gettid(), &attr, 0);
        if (ret < 0) {
            perror("ERROR: sched_setattr");
        }

        cout << "thread id " << gettid() << " w/ deadline " << request_.procinfo().compdeadline() << ", time since start time " << time_since_(start_time) << endl;

        // run proc content function
        int to_sum_to = 0;
        auto ms_to_sleep = 0ms;
        switch (strToPt(request_.typetorun())){
        case STATIC_PAGE_GET:
          to_sum_to = 1000000; // ca 8 ms
          ms_to_sleep = 5ms;
          break;
        case DYNAMIC_PAGE_GET:
          to_sum_to = 10000000; // ca 40 ms 
          ms_to_sleep = 20ms;
          break;
        case DATA_PROCESS_FG:
          to_sum_to = 300000000; // ca 950-1500 ms ie 1.3 sec
          ms_to_sleep = 100ms;
          break;
        case DATA_PROCESS_BG:
          to_sum_to = 1000000000; // ca 3800-4200 ms ie 3.8 sec
          ms_to_sleep = 1000ms;
          break;
        }

        long long sum = 0;
        for (long long i = 0; i < to_sum_to; i++) {
          if (i==50000) {
            std::this_thread::sleep_for(ms_to_sleep);
          }
          sum = 3 * i + 1;
        }

        cout << "thread " << gettid() <<" done w/ compute, time since start time " << time_since_(start_time) << endl;

        // clean up
        p_q_->remove(proc);
        delete proc;
        
        // get stats, print them
        struct rusage usage_stats;
        getrusage(RUSAGE_THREAD, &usage_stats);

        // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
        float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                                + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
        float mem_used = usage_stats.ru_maxrss / 1000;

        reply_.set_retval(sum);
        reply_.set_rusage(runtime);
        reply_.set_timepassed(time_since_(start_time));
        util_lock_.lock();
        reply_.set_currutil(*curr_util_);
        util_lock_.unlock();

        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        CHECK_EQ(status_, FINISH);
        // Once in the FINISH state, deallocate ourselves (CallData).
        delete this;
      }
    }

    bool isDone() {
      return status_ == FINISH;
    }

    int time_since_(std::chrono::high_resolution_clock::time_point since) {
      std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since);
      return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
    }

    ProcType strToPt(string type) {
      if (type == "static") {
        return STATIC_PAGE_GET;
      } else if (type == "dynamic") {
        return DYNAMIC_PAGE_GET;
      } else if (type == "fg") {
        return DATA_PROCESS_FG;
      } else if (type == "bg") {
        return DATA_PROCESS_BG;
      } else {
        cout << "OOPSSS" << endl;
        return STATIC_PAGE_GET;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    WebsiteServer::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    ProcToRun request_;
    RetVal reply_;

    ServerAsyncResponseWriter<RetVal> responder_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_; 

    Queue* p_q_;
    float *curr_util_;
    std::mutex& util_lock_;
  };


  void HandleRpcs() {

    // run main accept loop on cpu 1
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }
    
    new CallData(&service_, cq_.get(), proc_queue_, curr_util_, util_lock_);
    void* tag;  // uniquely identifies a request.
    bool ok;
    std::vector<std::thread> threads;

    while (true) {
      if (cq_->Next(&tag, &ok) && ok) {
        CallData* curr = static_cast<CallData*>(tag);
        if (curr->isDone()) {
          curr->Proceed();
          continue;
        }
        // run this in a new thread
        std::thread t(&CallData::Proceed, curr);
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

  std::unique_ptr<ServerCompletionQueue> cq_;
  WebsiteServer::AsyncService service_;
  std::unique_ptr<Server> server_;
  Queue* proc_queue_;
  float *curr_util_;
  std::mutex& util_lock_;
};

