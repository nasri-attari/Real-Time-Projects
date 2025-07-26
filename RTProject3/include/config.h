#ifndef CONFIG_H
#define CONFIG_H

#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <mqueue.h>

#define MAX_TARGET_NAME 64
#define MAX_REPORT_TEXT 256

#define SIG_ARREST SIGUSR1
#define SIGUSR3 (SIGRTMIN + 1)

#define FTOK_PATH "/tmp"
#define FTOK_PROJ_ID 65

//CRIME TYPES
#define ROB_BANKS            2
#define ROB_JEWELRY_SHOPS    3
#define DRUG_TRAFFICKING     6
#define ROB_ART_WORK         4
#define KIDNAPPING           0
#define BLACKMAIL            1
#define ARM_TRAFFICKING      5


#define ESCAPE_PLAN  0
#define EQUIPMENT_USED 1
#define POLICE_EXPECTED 2
#define ENTRY_POINT 3
#define BACKUP_PLAN 4
#define MEETING_POINT 5
#define ROLES_DISTRIBUTION 6
#define COMM_METHOD 7

#define MAX_GANGS 10
#define SEM_GANG_BASE 0
#define SEM_POLICE    MAX_GANGS
#define SEM_GLOBAL    MAX_GANGS+1
#define SEM_COUNT     MAX_GANGS+2
extern pid_t gang_pids[MAX_GANGS];
#define MAX_MEMBERS 10
#define SIG_PLANT_AGENT SIGUSR2
#define MQ_NAME "/agent_reports"
#define OPTION_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

extern const char *escape_plan_options[];
extern const char *equipment_options[];
extern const char *police_expected_options[];
extern const char *entry_point_options[];
extern const char *backup_plan_options[];
extern const char *meeting_point_options[];
extern const char *roles_distribution_options[];
extern const char *comm_method_options[];


// Configuration Struct
typedef struct {
    int gang_count;
    int members_per_gang;
    int agents_percentage;
    int promotion_interval;
    int max_preparation_level;
    int false_info_chance;
    int success_rate_base;
    int suspicion_threshold;
    int arrest_duration;
    int max_thwarted;
    int max_successful;
    int max_agents_exposed;
    int number_of_ranks;
    int probability_of_getting_killed;
    int agent_sucessful_plant_percentage;
    int probability_of_agent_caught;
    int max_prep_time;
    int min_prep_time;
    int max_prep;
    int min_prep;
    int max_exec_time;
    int min_exec_time;
    int max_mem_prep_added_ps;
    int min_mem_prep_added_ps;
    int increase_ranking;
} Config;

extern Config config;
int load_config(const char* path);

// Member & Gang Types
typedef struct {
    pthread_t thread_id;
    int rank;
    int preparation;
    int max_mem_prep_required;
    bool is_agent;
    bool is_alive;
    int should_wake_up;
    int arrested;
    bool has_reported;
    char known_crime_info[8][30];
    char member_message[128];
    int time_in_gang;
} Member;

typedef struct {
    int gang_id;
    Member members[MAX_MEMBERS];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_barrier_t barrier;
    int start_prep;
    int start_exec;
    int agent_exposed;
    int leader_ready;
    int current_target;
    int time_to_execute;
    int required_prep_time;
    int all_required_prep;
    int current_prep;
    float rtrp_ratio;
    char crime_info[8][30];
    int can_recruit;
    int can_thwart;
    int is_arrested;
    int leader_num;
    int arrest_timer;
} Gang;

typedef struct {
    int suspicion_levels[MAX_GANGS];
    int report_confirmations[MAX_GANGS];
    mqd_t mq;
    char status_message[MAX_GANGS][128];
} Police;

typedef struct {
    long mtype;
    int gang_id;
    int member_id;
    int suspicion;
    int info_type;
    char crime_info[30];
} Report;

typedef struct {
    Gang gangs[MAX_GANGS];
    Police police;
    int agents_exposed;
    int agents_active;
    int thwarted;
    int successful;
    int failed;
    pid_t gang_pids[MAX_GANGS];
    int sem_id;
    int msqid;
} GangPoliceState;

// Function Prototypes
void run_gang(int gang_id);
void run_police();
void* agent_thread_func(void* arg);
void log_event(const char* format, ...);
GangPoliceState* init_shared_memory_gangs();
GangPoliceState* get_shared_memory_gangs();
void destroy_shared_memory_gangs();
#endif
