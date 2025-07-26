#include <stdio.h>
#include "../include/config.h"
#include "../include/sem.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>

int sem_id;
int msqid;
key_t key;
int detect_last_thread=0;
int leader_num;

const char *escape_plan_options[] = {
    "North Alley", "South Tunnel", "Rooftop Exit", "Old Sewer"
};

const char *equipment_options[] = {
    "Guns, masks, van", "Crowbars, rope, bike", "Hacking gear, drone", "Smoke bombs, motorbikes"
};

const char *police_expected_options[] = {
    "Low", "Medium", "High", "None"
};

const char *entry_point_options[] = {
    "Front Door", "Back Window", "Roof Hatch", "Side Entrance"
};

const char *backup_plan_options[] = {
    "Escape via Subway", "Hide in Safehouse", "Switch vehicles", "Blend with crowd"
};

const char *meeting_point_options[] = {
    "Warehouse 17", "Dock 3", "Old Gym", "Abandoned Church"
};

const char *roles_distribution_options[] = {
    "2 lookouts, 3 attackers", "1 hacker, 2 guards, 2 looters", "3 entry team, 2 drivers", "1 negotiator, 4 muscle"
};

const char *comm_method_options[] = {
    "Burner phones", "Walkie-talkies", "Hand signals", "Encrypted app"
};

pid_t POLICE_PID;

GangPoliceState *shared;
Gang *gang;
int members_per_gang;
int number_of_ranks;
int MEMBERS_GENERATED = 0;
__thread int time_in_gang = 0;
__thread int handleSignals = 1;
__thread int handleSignals2 = 1;
__thread sigjmp_buf jump_buffer;
__thread int memNum;
int update_rank(int current_rank, int time) {
    if (time % config.increase_ranking == 0) {
        if (current_rank < number_of_ranks)
            return current_rank + 1;
    }
    return current_rank;
}

void sigusr1_handler(int sig) {
    gang->is_arrested=1;
    for (int i = 0; i < members_per_gang; i++) {
        pthread_kill(gang->members[i].thread_id, SIGUSR3);
    }
}

void sigusr3_handler(int sig) {
    if (handleSignals) {
        gang->members[memNum].arrested = 1; // Flag for threads to detect
        siglongjmp(jump_buffer, 1); // Jump out of current logic, back to safe point
    }
}

int specify_leader() {
    int max_rank = -1;
    int max_count = 0;
    int max_index = -1;
    int equal_indices[members_per_gang]; // Store indices of eligible equal-rank members

    // First pass: find the maximum rank among non-agent members
    for (int i = 0; i < members_per_gang; i++) {
        if (!gang->members[i].is_agent && gang->members[i].rank > max_rank) {
            max_rank = gang->members[i].rank;
        }
    }

    // Second pass: find all non-agent members with max_rank
    for (int i = 0; i < members_per_gang; i++) {
        if (!gang->members[i].is_agent && gang->members[i].rank == max_rank) {
            equal_indices[max_count++] = i;
        }
    }

    // Choose a random one from the list
    if (max_count > 0) {
        max_index = equal_indices[rand() % max_count];
    }

    // If more than one member has the highest rank, boost the chosen leader
    if (max_count > 1) {
        gang->members[max_index].rank++;
    }

    return max_index;
}



void reset_round_variables() {
    // Clean up all shared gang values before starting a new round.
    gang->current_target = -1;
    gang->leader_ready = 0;
    gang->all_required_prep = 0;
    gang->current_prep = 0;
    gang->rtrp_ratio = 0;
    gang->time_to_execute = 0;
    gang->agent_exposed = 0;
    gang->start_exec = 0;
    gang->start_prep = 0;

    // Clear gang crime info
    for (int i = 0; i < 8; i++) {
        memset(gang->crime_info[i], 0, 30);
    }

    for (int i = 0; i < members_per_gang; i++) {
        gang->members[i].preparation = 0;
        gang->members[i].max_mem_prep_required = 0;

        // Clear each member's known crime info
        for (int j = 0; j < 8; j++) {
            memset(gang->members[i].known_crime_info[j], 0, 30);
        }
    }
}


void *gang_manager(void *arg) {
    while (1) {
        sleep(1); // avoid busy waiting

        pthread_mutex_lock(&gang->mutex);

        int all_reported = 1;
        for (int i = 0; i < members_per_gang; i++) {
            if (!gang->members[i].has_reported) {
                all_reported = 0;
                break;
            }
        }

        if (!all_reported) {
            pthread_mutex_unlock(&gang->mutex);
            continue; // wait for all to finish execution
        }

        // COUNT ALIVE
        int alive_count = 0;
        for (int i = 0; i < members_per_gang; i++) {
            if (gang->members[i].is_alive) {
                alive_count++;
            }
        }

        // Revive only dead members and reset "has_reported"
        for (int i = 0; i < members_per_gang; i++) {
            if (!gang->members[i].is_alive) {
                gang->members[i].is_alive = true;
                gang->members[i].rank = 1;
                gang->members[i].preparation = 0;
                gang->members[i].max_mem_prep_required = 0;
                gang->members[i].time_in_gang = 0;
            }

            // Wake up all members (alive or dead) if they’re blocked
            gang->members[i].should_wake_up = 1;
            gang->members[i].has_reported = 0;
        }

        // Decision logic
        if (alive_count > 0) {
            float chance = rand() / (float) RAND_MAX;
            Report report;
            report.mtype = 1;
            report.gang_id = gang->gang_id;
            report.info_type=0;
            report.member_id=0;
            report.suspicion=0;

            if (chance < gang->rtrp_ratio) {
                snprintf(gang->members[leader_num].member_message,30, "GOOD JOB TEAM!. WE WON");
                strncpy(report.crime_info, "MISSION SUCCESS", sizeof(report.crime_info));
                if (msgsnd(msqid, &report, sizeof(Report) - sizeof(long), 0) == -1) {
                    perror("Initial msgsnd failed");
                }

            } else {
                snprintf(gang->members[leader_num].member_message,30, "WE LOST.PREPARE BETTER");
                strncpy(report.crime_info, "MISSION FAILED", sizeof(report.crime_info));
                if (msgsnd(msqid, &report, sizeof(Report) - sizeof(long), 0) == -1) {
                    perror("Initial msgsnd failed");
                }

            }
        }

        pthread_cond_broadcast(&gang->cond);
        pthread_mutex_unlock(&gang->mutex);
    }

    return NULL;
}

void shuffle(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);  // random index from 0 to i
        // Swap array[i] and array[j]
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}
void *gang_member(void *arg) {
    memNum = *((int *) arg);
    //sigset 1
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR3);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL); // UNBlock SIGUSR2 by default

    struct sigaction sa;
    sa.sa_handler = sigusr3_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR3, &sa, NULL);

    signal(SIGUSR3, sigusr3_handler);
    gang->members[memNum].rank = 1;

    while (1) {
        if (sigsetjmp(jump_buffer, 1) != 0) {
            // Jumped here from signal
            if (!gang->members[memNum].is_agent) {
                if(memNum==leader_num)
                    gang->arrest_timer=config.arrest_duration;
                for (int y=0; y<config.arrest_duration; y++) {
                    sleep(1);
                    if(memNum==leader_num)
                        gang->arrest_timer--;
                }
            }
            else if (gang->members[memNum].is_agent) {
                snprintf(gang->members[memNum].member_message,30, "HAHA, we got them weak ones!");
                sleep(config.arrest_duration);
                gang->members[memNum].member_message[0] = '\0';
            }
            gang->members[memNum].arrested = 0;
            int leader = specify_leader();
            if (memNum == leader) { // Leader
                for (int i = 0; i < members_per_gang; i++) {
                    if (!gang->members[i].is_agent)
                        continue;

                    // Count correct crime info entries
                    int correct_info = 0;
                    for (int j = 0; j < 8; j++) {
                        if (strcmp(gang->members[i].known_crime_info[j], gang->crime_info[j]) == 0) {
                            correct_info++;
                        }
                    }

                    // Threshold based on rank
                    int allowed = (float) gang->members[i].rank / (float) (float) gang->members[leader].rank;

                    // 30% chance to catch if suspicious
                    if (correct_info > allowed && (rand() % 100) < config.probability_of_agent_caught) {
                        // Execute the agent (reset)

                        gang->members[i].is_alive = 0;
                        snprintf(gang->members[leader].member_message,30, "member %d is a TRAITOR", i);
                        gang->agent_exposed = 1; // for gui
                        sleep(2);
                        for (int h=0;h<members_per_gang;h++) {
                            if (h!=i) {
                                snprintf(gang->members[h].member_message,30, "UNBELIEVABLE!");
                            }
                        }
                        snprintf(gang->members[leader].member_message,30, "EXECUTE HIM!");
                        sleep(2);
                        snprintf(gang->members[leader].member_message,30, "WELCOME TO THE NEW MEMBER");
                        sleep(2);
                        // Reset round variables
                        reset_round_variables();
                        //sem_wait
                        shared->agents_exposed++;
                        shared->agents_active--;
                        //sem_lock
                        gang->agent_exposed = 0; // for gui
                        //thread values reset
                        gang->members[i].is_agent = 0;
                        gang->members[i].preparation = 0;
                        gang->members[i].arrested = 0;
                        gang->members[i].is_alive = 1;
                        gang->members[i].rank = 0; // or re-assign dynamically
                        gang->members[i].max_mem_prep_required = 0;
                        for (int j = 0; j < 8; j++) {
                            strcpy(gang->members[i].known_crime_info[j], "");
                        }
                        break; // stop checking others, restart now
                    }
                }

            }


        }
        detect_last_thread=0;
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_barrier_wait(&gang->barrier);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        gang->is_arrested = 0;
        reset_round_variables();
        gang->members[memNum].is_alive = true;
        gang->members[memNum].time_in_gang++;
        gang->members[memNum].rank = update_rank(gang->members[memNum].rank, gang->members[memNum].time_in_gang);
        pthread_mutex_lock(&gang->mutex);
        detect_last_thread++;
        pthread_mutex_unlock(&gang->mutex);
        if (detect_last_thread == members_per_gang) {
            // let the last thread detect leader_num
            leader_num = specify_leader();
            gang->leader_num=leader_num;
        }
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_barrier_wait(&gang->barrier);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        if (memNum==leader_num) {
            snprintf(gang->members[leader_num].member_message,128, "GET READY TEAM!");
            gang->can_recruit=1;
        }
        sleep(3);
        if (memNum==leader_num)
            gang->members[memNum].member_message[0] = '\0';
        if (memNum==leader_num)
            gang->can_recruit=0;
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_barrier_wait(&gang->barrier); // to ensure all ranks are updated
        //LEADER INITIALIZES CRIME ROUND
        if (gang->current_target == -1 && memNum == leader_num) {
            snprintf(gang->members[leader_num].member_message,30, "I AM THE LEADER");
            sleep(1);
            snprintf(gang->members[leader_num].member_message,30, "THE PLAN IS READY");
            // Randomly select one option for each field
            strcpy(gang->crime_info[ESCAPE_PLAN], escape_plan_options[rand() % OPTION_COUNT(escape_plan_options)]);
            strcpy(gang->crime_info[EQUIPMENT_USED], equipment_options[rand() % OPTION_COUNT(equipment_options)]);
            strcpy(gang->crime_info[POLICE_EXPECTED],
                   police_expected_options[rand() % OPTION_COUNT(police_expected_options)]);
            strcpy(gang->crime_info[ENTRY_POINT], entry_point_options[rand() % OPTION_COUNT(entry_point_options)]);
            strcpy(gang->crime_info[BACKUP_PLAN], backup_plan_options[rand() % OPTION_COUNT(backup_plan_options)]);
            strcpy(gang->crime_info[MEETING_POINT],
                   meeting_point_options[rand() % OPTION_COUNT(meeting_point_options)]);
            strcpy(gang->crime_info[ROLES_DISTRIBUTION],
                   roles_distribution_options[rand() % OPTION_COUNT(roles_distribution_options)]);
            strcpy(gang->crime_info[COMM_METHOD], comm_method_options[rand() % OPTION_COUNT(comm_method_options)]);

            gang->current_target = rand() % 7;
            gang->required_prep_time = rand() % (config.max_prep_time - config.min_prep_time +1) + config.min_prep_time;

            for (int i = 0; i < members_per_gang; i++) {
                gang->members[i].max_mem_prep_required = rand() % (config.max_prep - config.min_prep +1) + config.min_prep;
                gang->all_required_prep += gang->members[i].max_mem_prep_required;

                // Assign known crime info based on truth chance
                int num_truths = (int)((float)gang->members[i].rank / gang->members[leader_num].rank * 8);
                int indices[8] = {0,1,2,3,4,5,6,7};
                shuffle(indices,8); // randomize which crime infos will be truth

                for (int j = 0; j < 8; j++) {
                    int info_idx = indices[j];
                    if (j < num_truths)
                        strcpy(gang->members[i].known_crime_info[info_idx], gang->crime_info[info_idx]); // truth
                    else
                        snprintf(gang->members[i].known_crime_info[info_idx], 30, "FALSE_INFO_%d", rand() % 100); // fake
                }
                if (i!=leader_num)
                snprintf(gang->members[i].member_message,30, "INFORMED");
            }

            gang->time_to_execute = rand() % (config.max_exec_time - config.min_exec_time +1) + config.min_exec_time;

        }

        sleep(1);
        pthread_barrier_wait(&gang->barrier);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);

        if (gang->members[memNum].is_agent) {

            int correct_info = 0;
            for (int i = 0; i < 8; i++) {
                if (strcmp(gang->members[memNum].known_crime_info[i], gang->crime_info[i]) == 0) {
                    correct_info++;
                }
            }

            int suspicion = (int)(((float)correct_info / 8.0f) * 100);

            for (int i = 0; i < 8; i++) {
                Report report;
                report.mtype = 1;
                report.gang_id = gang->gang_id;
                report.member_id = memNum;
                report.suspicion = suspicion;
                report.info_type = i;

                snprintf(report.crime_info, sizeof(report.crime_info), "%s", gang->members[memNum].known_crime_info[i]);
                if (!gang->members[memNum].arrested) {
                    if (msgsnd(msqid, &report, sizeof(Report) - sizeof(long), 0) == -1) {
                        perror("Initial msgsnd failed");
                    }
                }
            }
        }
        sleep(2);
        gang->members[memNum].member_message[0] = '\0';

        for (int i = 0; i < gang->required_prep_time; i++) {
            int TARGET=0;
            if (memNum==leader_num)
                snprintf(gang->members[leader_num].member_message,30, "START PREPARING");
            gang->start_prep=1;
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_barrier_wait(&gang->barrier);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    // Normal member does preparation
    if (!gang->members[memNum].is_agent) {
        gang->members[memNum].preparation += rand() % (config.max_mem_prep_added_ps - config.min_mem_prep_added_ps +1) + config.min_mem_prep_added_ps;
        if (gang->members[memNum].preparation >= gang->members[memNum].max_mem_prep_required)
            gang->members[memNum].preparation = gang->members[memNum].max_mem_prep_required;
    } else {
        // Agent: no preparation
        gang->members[memNum].preparation = 0;

        // Collect list of valid targets (lower or equal rank)
        int valid_targets[members_per_gang];
        int valid_count = 0;

        for (int k = 0; k < members_per_gang; k++) {
            if (k != memNum && gang->members[k].rank <= gang->members[memNum].rank) {
                valid_targets[valid_count++] = k;
            }
        }

        // If there’s anyone valid to communicate with
        if (valid_count > 0) {
            // Pick one at random
            int target = valid_targets[rand() % valid_count];
            TARGET = target;
            int info_index = rand() % 8;
            snprintf(gang->members[memNum].member_message,30, "%d, CAN WE TALK?",target);
            snprintf(gang->members[target].member_message,30, "OKAY");

            // If the agent has wrong or empty info, update it
            if (strcmp(gang->members[memNum].known_crime_info[info_index], gang->crime_info[info_index]) != 0 ||
                strcmp(gang->members[memNum].known_crime_info[info_index], "") == 0) {
                strcpy(gang->members[memNum].known_crime_info[info_index],
                       gang->members[target].known_crime_info[info_index]);
            }

            // Calculate suspicion
            int correct_info = 0;
            for (int k = 0; k < 8; k++) {
                if (strcmp(gang->members[memNum].known_crime_info[k], gang->crime_info[k]) == 0) {
                    correct_info++;
                }
            }

            int suspicion = (int)(((float)correct_info / 8.0f) * 100);

            // Prepare report
            Report report;
            report.mtype = 1;
            report.gang_id = gang->gang_id;
            report.member_id = memNum;
            report.suspicion = suspicion;
            report.info_type = info_index;
            snprintf(report.crime_info, sizeof(report.crime_info), "%s", gang->members[memNum].known_crime_info[i]);
            // Send report
            if (!gang->members[memNum].arrested) {
                if (msgsnd(msqid, &report, sizeof(Report) - sizeof(long), 0) == -1) {
                    perror("msgsnd failed");
                }
            }
        }
    }

    sleep(1);
            gang->members[memNum].member_message[0] = '\0';
            gang->members[TARGET].member_message[0] = '\0';
}
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_barrier_wait(&gang->barrier);
        gang->start_prep = 0; // when all members finish preparing, start_prep=0
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_mutex_lock(&gang->mutex);
        gang->current_prep += gang->members[memNum].preparation;
        gang->rtrp_ratio = (float) gang->current_prep / (float) gang->all_required_prep;
        pthread_mutex_unlock(&gang->mutex);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        pthread_barrier_wait(&gang->barrier);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        unsigned int seed = time(NULL) ^ (memNum + pthread_self()); // Unique seed per thread
        pthread_sigmask(SIG_BLOCK, &set, NULL); // When the gang is executing the crime, dont handle
        if (!gang->members[memNum].is_agent) {
            snprintf(gang->members[leader_num].member_message,30, "START EXECUTING");
            gang->start_exec=1;
            gang->can_thwart = 0;
            for (int i = 0; i < gang->time_to_execute; i++) {
                float r = (float) rand_r(&seed) / (float) RAND_MAX;
                if (r < (float) config.probability_of_getting_killed / 100.0) {
                    pthread_mutex_lock(&gang->mutex);
                    gang->members[memNum].should_wake_up = 0;
                    pthread_mutex_unlock(&gang->mutex);
                    snprintf(gang->members[memNum].member_message,30, "HELP,They Will Kill Me");
                    sleep(1);
                    gang->members[memNum].is_alive = false;
                    gang->members[memNum].member_message[0] = '\0';
                    break;
                }
                sleep(1);

            }
        }

        pthread_barrier_wait(&gang->barrier);
        gang->start_exec = 0;
        gang->members[memNum].has_reported = 1;
        pthread_mutex_lock(&gang->mutex);
        while (!gang->members[memNum].should_wake_up) {
            pthread_cond_wait(&gang->cond, &gang->mutex);
        }
        pthread_mutex_unlock(&gang->mutex);
        sleep(2);
        handleSignals = 0;
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        handleSignals = 1; //reset flag
        gang->members[memNum].member_message[0] = '\0';
        gang->can_thwart = 1;
    }

    return NULL;
}

void sigusr2_handler(int sig) {
    if (gang->can_recruit){
        shared->agents_active++;
        int eligible[MEMBERS_GENERATED];
    int count = 0;

    for (int i = 0; i < MEMBERS_GENERATED; i++) {
        if (gang->members[i].is_alive && !gang->members[i].is_agent &&
            gang->members[i].rank != gang->members[specify_leader()].rank) {
            eligible[count++] = i;
            }
    }
    if (count == 0) {
        return;
    }

    int random_index = rand() % count;
    int selected_id = eligible[random_index];

    gang->members[selected_id].is_agent = true;
}
}


void run_gang(int gang_id) {
    sigset_t set2;
    sigemptyset(&set2);
    sigaddset(&set2, SIGUSR2);
    pthread_sigmask(SIG_UNBLOCK, &set2, NULL); // UNBlock SIGUSR2 by default

    struct sigaction sa;
    sa.sa_handler = sigusr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
    signal(SIGUSR2, sigusr2_handler);

    pthread_t manager_thread; // NEW

    pthread_mutex_init(&gang->mutex, NULL);
    pthread_cond_init(&gang->cond, NULL);
    pthread_barrier_init(&gang->barrier, NULL, members_per_gang);
    // Start member threads
    int ids[members_per_gang];
    for (MEMBERS_GENERATED = 0; MEMBERS_GENERATED < members_per_gang; MEMBERS_GENERATED++) {
        pthread_sigmask(SIG_BLOCK, &set2, NULL);
        ids[MEMBERS_GENERATED] = MEMBERS_GENERATED;
        if (MEMBERS_GENERATED < members_per_gang && pthread_create(&gang->members[MEMBERS_GENERATED].thread_id, NULL,
                                                                   gang_member,
                                                                   &ids[MEMBERS_GENERATED]) != 0) {
            perror("Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
        pthread_detach(gang->members[MEMBERS_GENERATED].thread_id);
        sleep(1);
        pthread_sigmask(SIG_UNBLOCK, &set2, NULL);
    }

    // Start manager thread
    if (pthread_create(&manager_thread, NULL, gang_manager, NULL) != 0) {
        perror("Failed to create manager thread\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(manager_thread);

    while (1) {
        // Gang loop continues indefinitely
        sleep(10); // Optional
    }
}

int main(int argc, char *argv[]) {

    signal(SIGUSR1, sigusr1_handler); // Register handler

    if (load_config("config.txt") != 0) {
        return 1;
    }

    srand(time(NULL) ^ getpid() ^ rand());

    shared = get_shared_memory_gangs();
    sem_id = shared->sem_id;
    msqid = shared->msqid;
    if (msqid <= 0) {
        exit(1);
    }
    if (shared == NULL) {
        exit(EXIT_FAILURE);
    }

    gang = &shared->gangs[atoi(argv[1])];
    members_per_gang = config.members_per_gang;
    number_of_ranks = config.number_of_ranks;
    gang->can_recruit = 0;
    gang->can_thwart = 1;
    gang->is_arrested=0;

    if (gang == NULL) {
        perror("Failed to allocate memory for gang");
        exit(EXIT_FAILURE);
    }

    gang->gang_id = atoi(argv[1]) ;
    reset_round_variables();

    for (int i = 0; i < members_per_gang; i++) {
        gang->members[i].is_alive = true;
        gang->members[i].is_agent = false;
        gang->members[i].rank = 0;
        gang->members[i].should_wake_up = 1;
        gang->members[i].arrested=0;
        gang->members[i].member_message[0] = '\0';
    }

    run_gang(gang->gang_id);

    pthread_mutex_destroy(&gang->mutex);
    pthread_cond_destroy(&gang->cond);
    pthread_barrier_destroy(&gang->barrier);

    return 0;
}
