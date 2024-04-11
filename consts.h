#define LB_MACHINE_LISTEN_PORT 8080
#define LB_IP "127.0.0.1"
#define LB_CLIENT_LISTEN_PORT 9000

#define CORE_LB_WEBSITE_RUN_ON 0
#define CORE_DISPATCHER_RUNS_ON 0
#define TOTAL_NUM_CPUS 4

#define NUM_STATIC_PROCS_GEN 2
#define NUM_DYNAMIC_PROCS_GEN 2
#define NUM_DATA_FG_PROCS_GEN 2

#define BUF_SZ 1024

// for dispatcher
#define THRESHOLD_PUSH_SLA 10
#define THESHOLD_PUSH_ACTIVEQ_SIZE 2
#define THRESHOLD_PUSH_TIME_PASSED_TO_SLA_RATIO 0.3
#define HOLDQ_CHECK_SLEEP_TIME_MILLISEC 5

