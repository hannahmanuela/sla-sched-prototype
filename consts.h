#define USING_PD3 false
#define USING_CLOUDLAB true

#define LB_MACHINE_LISTEN_PORT 8080
#if USING_PD3
    #define LB_IP "18.26.5.3"
#elif USING_CLOUDLAB
    #define LB_IP "130.127.133.141"
#endif
#define LB_CLIENT_LISTEN_PORT 9000

#define BUF_SZ 1024
// TOTAL_NUM_CPUS - 2 is how many cpus the dispatcher gets to run stuff
// we are avoiding core 0 for now
#define TOTAL_NUM_CPUS 3

// for single machine setup
#define CORE_LB_WEBSITE_RUN_ON 1

// for client
#define NUM_STATIC_PROCS_GEN 1
#define NUM_DYNAMIC_PROCS_GEN 1
#define NUM_DATA_FG_PROCS_GEN 1

#if USING_PD3
    #define STATIC_PATH "/home/hannahmanuela/lnx-test/build/static_page_get"
    #define DYANMIC_PATH "/home/hannahmanuela/lnx-test/build/dynamic_page_get"
    #define FG_PATH "/home/hannahmanuela/lnx-test/build/data_process_fg"
#elif USING_CLOUDLAB
    #define STATIC_PATH "/users/hmng/lnx-test/build/static_page_get"
    #define DYANMIC_PATH "/users/hmng/lnx-test/build/dynamic_page_get"
    #define FG_PATH "/users/hmng/lnx-test/build/data_process_fg"
#endif


// for dispatcher
#define CORE_DISPATCHER_RUNS_ON 1
#define THRESHOLD_PUSH_SLA 10
#define THESHOLD_PUSH_ACTIVEQ_SIZE 2
#define THRESHOLD_PUSH_TIME_PASSED_TO_SLA_RATIO 0.3
#define HOLDQ_CHECK_SLEEP_TIME_MILLISEC 5

// for lb
#define THRESHOLD_SPRAY_SLA 10
#define MACHINE_HEARTBEAT_SLEEP_TIME_MILLISEC 50
#define MACHINE_HEARTBEAT_TIMEOUT_MILLISEC 5