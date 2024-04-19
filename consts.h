#define LB_MACHINE_LISTEN_PORT 8080
#define LB_IP "172.31.75.20"
#define LB_CLIENT_LISTEN_PORT 9000

#define BUF_SZ 1024
#define TOTAL_NUM_CPUS 4

// for single machine setup
#define CORE_LB_WEBSITE_RUN_ON 0

// for client
#define NUM_STATIC_PROCS_GEN 2
#define NUM_DYNAMIC_PROCS_GEN 2
#define NUM_DATA_FG_PROCS_GEN 2

#define STATIC_PATH "/home/arch/lnx-test/build/static_page_get"
#define DYANMIC_PATH "/home/arch/lnx-test/build/dynamic_page_get"
#define FG_PATH "/home/arch/lnx-test/build/data_process_fg"

// for dispatcher
#define CORE_DISPATCHER_RUNS_ON 0
#define THRESHOLD_PUSH_SLA 10
#define THESHOLD_PUSH_ACTIVEQ_SIZE 2
#define THRESHOLD_PUSH_TIME_PASSED_TO_SLA_RATIO 0.3
#define HOLDQ_CHECK_SLEEP_TIME_MILLISEC 5

// for lb
#define THRESHOLD_SPRAY_SLA 10
#define MACHINE_HEARTBEAT_SLEEP_TIME_MILLISEC 50
#define MACHINE_HEARTBEAT_TIMEOUT_MILLISEC 5