#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <thread>
#include <fstream>

#include "dispatcher.h"
#include "message.h"
#include "consts.h"

int Dispatcher::time_since_start_() {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time_);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

tuple<string, string> get_time_(int pid) {
   using std::ios_base;
   using std::ifstream;
   using std::string;

   // 'file' stat seems to give the most reliable results
   std::string file_name = "/proc/";
   file_name += std::to_string(pid);
   file_name += "/stat";
   ifstream stat_stream(file_name ,ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid_, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime;


   stat_stream >> pid_ >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime; // don't care about the rest

   return make_tuple(utime, stime);
}

// runs in a thread
// clones proc, adds it to active q, waits for it to finish, removes it from active q
void Dispatcher::add_run_active_proc_(Proc* to_run) {

    // clone
    int fd_to_use;
    if (to_run->get_sla() < 10) {
        fd_to_use = fd_one_digit_ms_cgroup_;
    } else if (to_run->get_sla() < 100) {
        fd_to_use = fd_two_digit_ms_cgroup_;
    } else {
        fd_to_use = fd_three_digit_ms_cgroup_;
    }

    struct clone_args args = {
        .flags = CLONE_INTO_CGROUP,
        .exit_signal = SIGCHLD,
        .cgroup = (__u64)fd_to_use,
        };
    int c_pid = syscall(SYS_clone3, &args, sizeof(struct clone_args));

    if (c_pid == -1) { 
        cout << "clone3 failed: " << strerror(errno) << endl;
        perror("clone3"); 
        exit(EXIT_FAILURE); 
    } else if (c_pid > 0) { 
        // PARENT
        // add proc to active q
        active_q_->enq(to_run);
        // wait for proc to finish
        struct rusage usage_stats;
        int ret = wait4(c_pid, NULL, 0, &usage_stats);
        if (ret < 0) {
            perror("wait4 did bad");
        }
        // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
        // float runtime = (usage_stats.ru_utime.tv_usec + usage_stats.ru_stime.tv_usec)/1000;
        float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                                + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
        float mem_used = usage_stats.ru_maxrss / 1000;
        cout << "pid " << c_pid << " for sla " << to_run->get_sla() << ", with runtime: " << runtime << ", mem used (in MB): " << mem_used << endl;
        // remove it from active q
        active_q_->remove(to_run);
        // write accounting into a file (like I did in the simulator)
        ofstream file;
        file.open("../procs_done.txt", ios::app);
        file << time_since_start_() << ", " <<  id << ", " << to_run->get_sla() << ", " << runtime << ", " << to_run->get_time_since_spawn(time_since_start_()) << endl;
        // update profile
        ProcTypeProfile* profile = get_profile(to_run->get_type());
        if (profile != nullptr) {
            profile->compute->update(runtime);
        } else {
            // make new profile
            ProcTypeProfile* new_profile = new ProcTypeProfile(mem_used, runtime);
            proc_type_profiles_.push_back(new_profile);
        }
        // free the procs mem? I think this just calls the desctructor?
        free(to_run);
    } else {
        // setting affinity manually for now - use all cores not used by the lb/website or dispatcher
        cpu_set_t mask;
        CPU_ZERO(&mask);
        // don't use core 0
        for (int i = 1; i < TOTAL_NUM_CPUS; i++) {
            if (i != CORE_DISPATCHER_RUNS_ON && i != CORE_LB_WEBSITE_RUN_ON) {
                CPU_SET(i, &mask);
            }
        }
        if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
            cout << "set affinity had an error" << endl;
        }
        // if child, execute intense compute
        to_run->exec_proc();
    }
}

ProcHist Dispatcher::gen_proc_hist_() {
    ProcHist hist = ProcHist();

    for (Proc* p : active_q_->get_q()) {
        hist.put(true, p->get_sla());
    }
    for (Proc* p : hold_q_->get_q()) {
        hist.put(false, p->get_sla());
    }

    return hist;
}

void Dispatcher::run_lb_conn_(int lb_conn_fd) {

    vector<std::thread> procs;

    cout << "dispatcher running lb conn: " << getpid() << endl;

    // listen on LB connection, take incoming proc reqs, clone for each
    while(1) {
        char buffer[BUF_SZ];
        ssize_t n = read(lb_conn_fd, buffer, BUF_SZ);
        if (n < 0) {
            perror("ERROR reading from socket");
            break;
        } else if (n == 0) {
            cout << "lb died, we return" << endl;
            return;
        }
        MessageType msg_type;
        std::memcpy(&msg_type, buffer, sizeof(MessageType));
        
        if (msg_type == EXIT) {
            break;
        } else if (msg_type == HEARTBEAT) {
            // respond to heartbeat
            ProcHist hist = gen_proc_hist_();
            HeartbeatResponseMessage to_send = HeartbeatResponseMessage(hist);

            char buffer[BUF_SZ];
            to_send.to_bytes(buffer);
            ssize_t n = send(lb_conn_fd, buffer, sizeof(buffer), 0);
            if (n < 0) {
                perror("ERROR sending to socket");
            }

        } else if (msg_type == PROC) {
            ProcMessage lb_proc_msg = ProcMessage();
            lb_proc_msg.from_bytes(buffer);

            lb_proc_msg.msg_proc->set_time_spawned(time_since_start_());

            // add to hold q or active q? 
            // should adding it to active q be what actually runs clone and exec?
            if (lb_proc_msg.msg_proc->get_sla() < THRESHOLD_PUSH_SLA) {
                std::thread t(&Dispatcher::add_run_active_proc_, this, lb_proc_msg.msg_proc);
                procs.push_back(std::move(t));
            } else {
                cout << "adding to holdq" << endl;
                hold_q_->enq(lb_proc_msg.msg_proc);
            }
        } else {
            cout << "unexpected msg type on dispatcher" << endl;
            break;
        }
    }
    
    // wait for remaining procs to finish before exiting
    for(auto& t : procs){ 
        t.join();
    }
}

void Dispatcher::create_run_lb_conn_() {

    int status, lb_conn_fd;
    struct sockaddr_in serv_addr;
    if ((lb_conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(LB_MACHINE_LISTEN_PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LB_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }
 
    if ((status = connect(lb_conn_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr))) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    cout << "connected to lb" << endl;

    run_lb_conn_(lb_conn_fd);
 
    // closing the connected socket
    close(lb_conn_fd);
}

void Dispatcher::run_holdq_() {

    cout << "dispatcher running holdq: " << getpid() << endl;

    vector<std::thread> active_procs;

    while (1) {
        for (Proc* p : hold_q_->get_q()) {
            if (((p->get_time_since_spawn(time_since_start_()) / p->get_sla()) > THRESHOLD_PUSH_TIME_PASSED_TO_SLA_RATIO)) {
                cout << "pushing proc w/ sla " << p->get_sla() << " from holdq b/c it passed ratio" << endl;
                hold_q_->remove(p);
                std::thread t(&Dispatcher::add_run_active_proc_, this, p);
                active_procs.push_back(std::move(t));
            } else if ((active_q_->get_q().size() < THESHOLD_PUSH_ACTIVEQ_SIZE)) {
                cout << "pushing proc w/ sla " << p->get_sla() << " from holdq b/c active empty enough" << endl;
                hold_q_->remove(p);
                std::thread t(&Dispatcher::add_run_active_proc_, this, p);
                active_procs.push_back(std::move(t));
            }
        }
        std::this_thread::sleep_for(chrono::milliseconds(HOLDQ_CHECK_SLEEP_TIME_MILLISEC));
    }

    for(auto& t : active_procs){ 
        t.join();
    }
    
}

void Dispatcher::run() {

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE_DISPATCHER_RUNS_ON, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    start_time_ = std::chrono::high_resolution_clock::now();

    cout << "dispatcher main process: " << getpid() << endl;

    std::thread hold(&Dispatcher::run_holdq_, this);
    std::thread lb(&Dispatcher::create_run_lb_conn_, this);

    // "graceful" exit: if the lb conn is done, we are done
    lb.join();
}



void empty_files() {
    ofstream procs_done_file;
    procs_done_file.open("../procs_done.txt");
    procs_done_file << "";
    procs_done_file.close();
}




int main() {
    empty_files();
    
    // create dispatcher instance
    std::vector<ProcTypeProfile*> profiles;

    // TODO: just hardcoding id for now, should probably get it from the lb
    Dispatcher d = Dispatcher(profiles, 0);
    // run it
    d.run();
}
