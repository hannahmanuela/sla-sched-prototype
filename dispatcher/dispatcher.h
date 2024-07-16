#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <chrono>
#include <ctime>


#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>


using namespace std;

#ifndef DISPATCHER_H
#define DISPATCHER_H


class Dispatcher {

    public:
        int id;

        Dispatcher(int id_given) {
            id = id_given;
        };

        void run(string port);


    private:
        std::chrono::high_resolution_clock::time_point start_time_;

        int time_since_start_();

};


#endif  // DISPATCHER_H