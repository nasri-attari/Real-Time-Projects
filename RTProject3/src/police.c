#include "../include/config.h"
#include "../include/sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>

GangPoliceState *shared;
Police *police;
volatile sig_atomic_t stop_requested = 0;

void print_shared_memory_state() {
    printf("\n==== SHARED MEMORY STATE ====\n");

    // Global counters
    printf("-> MISSION SUCCESS COUNT: %d\n", shared->successful);
    printf("-> MISSION FAILED COUNT:  %d\n", shared->thwarted);
    printf("-> AGENTS EXPOSED COUNT:  %d\n", shared->agents_exposed);
    printf("-> AGENTS ACTIVE COUNT:   %d\n", shared->agents_active);
    printf("-> SEMAPHORE ID:          %d\n", shared->sem_id);
    printf("-> MESSAGE QUEUE ID:      %d\n", shared->msqid);

    // Police Data
    printf("\n--- Police Suspicion Levels & Messages ---\n");
    for (int i = 0; i < config.gang_count; i++) {
        printf("Gang %d:\n", i);
        printf("  Suspicion Level : %d\n", shared->police.suspicion_levels[i]);
        printf("  Confirmations   : %d\n", shared->police.report_confirmations[i]);
        printf("  Message         : %s\n", shared->police.status_message[i]);
    }

    // Gang and Members Info
    printf("\n--- Gang States ---\n");
    for (int i = 0; i < config.gang_count; i++) {
        Gang *g = &shared->gangs[i];
        printf("\nGang %d:\n", i);
        printf("  PID            : %d\n", shared->gang_pids[i]);
        printf("  Can Recruit    : %s\n", g->can_recruit ? "YES" : "NO");
        printf("  Agent Exposed  : %d\n", g->agent_exposed);
        printf("  Current Target : %d\n", g->current_target);
        printf("  RTRP Ratio     : %.2f\n", g->rtrp_ratio);
        printf("  All Prep       : %d\n", g->all_required_prep);
        printf("  Current Prep   : %d\n", g->current_prep);
        printf("  Time to Exec   : %d\n", g->time_to_execute);
        printf("  Required Prep  : %d\n", g->required_prep_time);
        printf("  Leader Num     : %d\n", g->leader_num);

        printf(" Crime Info:\n");
        for (int j = 0; j < 8; j++) {
            printf("    [%d] %s\n", j, g->crime_info[j]);
        }

        for (int j = 0; j < config.members_per_gang; j++) {
            Member *m = &g->members[j];
            printf("    Member %d:\n", j);
            printf("      Rank             : %d\n", m->rank);
            printf("      Prep             : %d/%d\n", m->preparation, m->max_mem_prep_required);
            printf("      Is Agent         : %s\n", m->is_agent ? "YES" : "NO");
            printf("      Is Alive         : %s\n", m->is_alive ? "YES" : "NO");
            printf("      Arrested         : %s\n", m->arrested ? "YES" : "NO");
            printf("      Should Wake Up   : %d\n", m->should_wake_up);
            printf("      Has Reported     : %d\n", m->has_reported);
            printf("      Message          : %s\n", m->member_message);

            printf("      Known Crime Info:\n");
            for (int k = 0; k < 8; k++) {
                printf("        [%d] %s\n", k, m->known_crime_info[k]);
            }
        }
    }
    printf("==========================================\n\n");
}


void handle_sigterm(int sig) {
    stop_requested = 1;
}

const int critical_info_types[] = {
    ENTRY_POINT,
    ESCAPE_PLAN,
    EQUIPMENT_USED,
    POLICE_EXPECTED
};

const char *info_type_names[] = {
    "ESCAPE_PLAN",
    "EQUIPMENT_USED",
    "POLICE_EXPECTED",
    "ENTRY_POINT",
    "BACKUP_PLAN",
    "MEETING_POINT",
    "ROLES_DISTRIBUTION",
    "COMM_METHOD"
};
int confirmations[10][8] = {0};
const int num_critical_types = sizeof(critical_info_types) / sizeof(critical_info_types[0]);

int is_critical_info(int info_type) {
    for (int i = 0; i < num_critical_types; i++) {
        if (critical_info_types[i] == info_type)
            return 1;
    }
    return 0;
}

void run_police() {
    shared = get_shared_memory_gangs();
    police = &shared->police;
    int sem_id = shared->sem_id;

    for (int i = 0; i < config.gang_count; i++) {
        sem_wait_index(sem_id, SEM_POLICE);
        snprintf(police->status_message[i], sizeof(police->status_message[i]), "No reports yet");
        sem_post_index(sem_id, SEM_POLICE);
    }
    int msqid = shared->msqid;

    srand(time(NULL));
    Report r;

    while (!stop_requested) {
        ssize_t res = msgrcv(msqid, &r, sizeof(Report) - sizeof(long), 0, IPC_NOWAIT);
        if (res > 0) {
            // Suspicion Tracking
            if (r.suspicion >= config.suspicion_threshold && is_critical_info(r.info_type)) {
                confirmations[r.gang_id][r.info_type]++;
                if (confirmations[r.gang_id][r.info_type] >= 2) {
                    sem_wait_index(sem_id, SEM_POLICE);
                    police->suspicion_levels[r.gang_id] = r.suspicion;
                    snprintf(police->status_message[r.gang_id], 128,
                             "Confirmed critical info");
                    sem_post_index(sem_id, SEM_POLICE);
                    confirmations[r.gang_id][r.info_type] = 0;
                }
            } else {
                sem_wait_index(sem_id, SEM_POLICE);
                police->suspicion_levels[r.gang_id] = r.suspicion / 2;
                snprintf(police->status_message[r.gang_id], 128,
                     "Normal Report from member %d.",r.member_id + 1);

                sem_post_index(sem_id, SEM_POLICE);

            }
            // Arrest
            if (r.gang_id >= 0 && r.gang_id < config.gang_count) {
                sem_wait_index(sem_id, SEM_POLICE);
                int current_suspicion = police->suspicion_levels[r.gang_id];
                sem_post_index(sem_id, SEM_POLICE);

                if (current_suspicion >= config.suspicion_threshold) {
                    current_suspicion = 0;
                    sem_wait_index(sem_id, SEM_GLOBAL);
                    pid_t target_pid = shared->gang_pids[r.gang_id];
                    sem_post_index(sem_id, SEM_GLOBAL);
                    if (target_pid > 0 && shared->gangs[r.gang_id].can_thwart && !shared->gangs[r.gang_id].is_arrested) {
                        kill(target_pid, SIG_ARREST);
                        fflush(stdout);
                        sem_wait_index(sem_id, SEM_POLICE);
                        police->suspicion_levels[r.gang_id] = 0;
                        police->report_confirmations[r.gang_id] = 0;
                        sem_post_index(sem_id, SEM_POLICE);
                        snprintf(police->status_message[r.gang_id], 128,
                             "Attempt to Arrest Gang");
                        sem_wait_index(sem_id, SEM_GLOBAL);
                        shared->thwarted++;
                        sem_post_index(sem_id, SEM_GLOBAL);
                    }
                }
            }


            if (strstr(r.crime_info, "MISSION SUCCESS")) {
                sem_wait_index(sem_id, SEM_GLOBAL);
                shared->successful++;
                sem_post_index(sem_id, SEM_GLOBAL);
                snprintf(police->status_message[r.gang_id], 128,
                             "Gang Mission Successed");
            }
            if (strstr(r.crime_info, "MISSION FAILED")) {
                sem_wait_index(sem_id, SEM_GLOBAL);
                shared->failed++;
                sem_post_index(sem_id, SEM_GLOBAL);
                snprintf(police->status_message[r.gang_id], 128,
                             "Gang Mission Failed");
            }
        }
        // Attempt Agent Planting
        if (rand() % 100 < config.agents_percentage) {
            int target_gang = rand() % config.gang_count;
            sem_wait_index(sem_id, SEM_GLOBAL);
            int recruitable = shared->gangs[target_gang].can_recruit;
            pid_t pid = shared->gang_pids[target_gang];
            sem_post_index(sem_id, SEM_GLOBAL);
            if (recruitable && pid != -1) {
                kill(pid, SIG_PLANT_AGENT);
                snprintf(police->status_message[r.gang_id], 128,
                             "Planting Agent");
            }
        }
        print_shared_memory_state();
        fflush(stdout);
        usleep(500000);
    }
    shmdt(shared);
}

int main() {
    if (load_config("config.txt") != 0) {
        fprintf(stderr, "[POLICE] Failed to load config.\n");
        exit(1);
    }
    signal(SIGTERM, handle_sigterm);
    run_police();
    return 0;
}
