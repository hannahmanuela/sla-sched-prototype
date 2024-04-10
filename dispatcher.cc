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
#include <thread>
#include <fstream>

#include "dispatcher.h"
#include "message.h"
#include "consts.h"

int Dispatcher::time_since_start_() {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time_);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

float Dispatcher::get_mem_usage_(int pid) {
   using std::ios_base;
   using std::ifstream;
   using std::string;

   float vm_usage     = 0.0;

   // 'file' stat seems to give the most reliable results
   std::string file_name = "/proc/";
   file_name += std::to_string(pid);
   file_name += "/stat";
   ifstream stat_stream(file_name ,ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid_, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   unsigned long vsize;

   stat_stream >> pid_ >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024.0;

   return vm_usage;
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
    cout << "about to clone" << endl;
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
        waitpid(c_pid, NULL, 0);
        double runtime = to_run->get_time_since_spawn(time_since_start_());
        // TODO: how to get mem?
        float mem_used = get_mem_usage_(c_pid);
        // remove it from active q
        active_q_->remove(to_run);
        // write accounting into a file (like I did in the simulator)
        ofstream file;
        file.open("../procs_done.txt", ios::app);
        file << time_since_start_() << ", " <<  id << ", " << to_run->get_type() << ", " << to_run->get_sla() << ", " << runtime << endl;
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
        for (int i = 0; i < TOTAL_NUM_CPUS; i++) {
            if (i != CORE_DISPATCHER_RUNS_ON && i != CORE_LB_WEBSITE_RUN_ON) {
                CPU_SET(i, &mask);
            }
        }
        if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
            cout << "set affinity had an error" << endl;
        }
        cout << "worker process: " << getpid() << endl;
        // if child, execute intense compute
        to_run->exec_proc();
    }
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
        Message lb_msg = Message();
        lb_msg.from_bytes(buffer);

        if (lb_msg.is_exit) {
            break;
        }
        
        cout << "got proc from lb: ";
        lb_msg.msg_proc->print();

        lb_msg.msg_proc->set_time_spawned(time_since_start_());

        // add to hold q or active q? 
        // should adding it to active q be what actually runs clone and exec?
        if (lb_msg.msg_proc->get_sla() < THRESHOLD_PUSH_SLA) {
            std::thread t(&Dispatcher::add_run_active_proc_, this, lb_msg.msg_proc);
            procs.push_back(std::move(t));
        } else {
            cout << "adding to holdq" << endl;
            hold_q_->enq(lb_msg.msg_proc);
            cout << "added" << endl;
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
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
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
                cout << "pushing proc from holdq b/c it passed ratio" << endl;
                hold_q_->remove(p);
                std::thread t(&Dispatcher::add_run_active_proc_, this, p);
                active_procs.push_back(std::move(t));
            } else if ((active_q_->get_q().size() < 2)) {
                cout << "pushing proc from holdq b/c active empty enough" << endl;
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
