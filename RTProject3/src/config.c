#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/config.h"
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>

static GangPoliceState* shared_state_gangs = NULL;

GangPoliceState* init_shared_memory_gangs() {
    size_t size = sizeof(GangPoliceState);

    if (size == 0) {
        fprintf(stderr, "[ERROR] sizeof(GangPoliceState) returned 0!\n");
        exit(1);
    }

    key_t key = ftok(FTOK_PATH, FTOK_PROJ_ID);
    if (key == -1) {
        perror("ftok failed");
        exit(1);
    }

    int shmid = shmget(key, size, IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    shared_state_gangs = (GangPoliceState*)shmat(shmid, NULL, 0);
    if (shared_state_gangs == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    memset(shared_state_gangs, 0, size);
    for (int i = 0; i < 10; i++) {
        shared_state_gangs->gang_pids[i] = -1;
    }

    return shared_state_gangs;
}


GangPoliceState* get_shared_memory_gangs() {
    if (shared_state_gangs != NULL) return shared_state_gangs;

    key_t key = ftok(FTOK_PATH, FTOK_PROJ_ID);
    if (key == -1) {
        perror("ftok (get) failed");
        exit(1);
    }
    int shmid = shmget(key, sizeof(GangPoliceState), 0666);
    if (shmid == -1) {
        perror("shmget (get) failed");
        exit(1);
    }

    shared_state_gangs = (GangPoliceState*)shmat(shmid, NULL, 0);
    if (shared_state_gangs == (void*)-1) {
        perror("shmat (get) failed");
        exit(1);
    }

    return shared_state_gangs;
}

void destroy_shared_memory_gangs() {
    if (shared_state_gangs != NULL) {
        if (shmdt(shared_state_gangs) == -1) {
            perror("shmdt (destroy) failed");
        }
        shared_state_gangs = NULL;
    }

    key_t key = ftok(FTOK_PATH, FTOK_PROJ_ID);
    if (key == -1) {
        perror("ftok (destroy) failed");
        return;
    }

    int shmid = shmget(key, sizeof(GangPoliceState), 0666);
    if (shmid == -1) {
        perror("shmget (destroy) failed");
        return;
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    } else {
        printf("Shared memory segment removed.\n");
    }
}


Config config;

int load_config(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Failed to open config file");
        return -1;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char key[64];
        int value;
        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "gang_count") == 0) config.gang_count = value;
            else if (strcmp(key, "members_per_gang") == 0) config.members_per_gang = value;
            else if (strcmp(key, "agents_percentage") == 0) config.agents_percentage = value;
            else if (strcmp(key, "promotion_interval") == 0) config.promotion_interval = value;
            else if (strcmp(key, "max_preparation_level") == 0) config.max_preparation_level = value;
            else if (strcmp(key, "false_info_chance") == 0) config.false_info_chance = value;
            else if (strcmp(key, "success_rate_base") == 0) config.success_rate_base = value;
            else if (strcmp(key, "suspicion_threshold") == 0) config.suspicion_threshold = value;
            else if (strcmp(key, "arrest_duration") == 0) config.arrest_duration = value;
            else if (strcmp(key, "max_thwarted") == 0) config.max_thwarted = value;
            else if (strcmp(key, "max_successful") == 0) config.max_successful = value;
            else if (strcmp(key, "max_agents_exposed") == 0) config.max_agents_exposed = value;
            else if (strcmp(key, "number_of_ranks") == 0) config.number_of_ranks = value;
            else if (strcmp(key, "probability_of_getting_killed") == 0) config.probability_of_getting_killed = value;
            else if (strcmp(key, "agent_sucessful_plant_percentage") == 0) config.agent_sucessful_plant_percentage = value;
            else if (strcmp(key, "probability_of_agent_caught") == 0) config.probability_of_agent_caught = value;
            else if (strcmp(key, "max_prep_time") == 0) config.max_prep_time = value;
            else if (strcmp(key, "min_prep_time") == 0) config.min_prep_time = value;
            else if (strcmp(key, "max_prep") == 0) config.max_prep = value;
            else if (strcmp(key, "min_prep") == 0) config.min_prep = value;
            else if (strcmp(key, "max_exec_time") == 0) config.max_exec_time = value;
            else if (strcmp(key, "min_exec_time") == 0) config.min_exec_time = value;
            else if (strcmp(key, "max_mem_prep_added_ps") == 0) config.max_mem_prep_added_ps = value;
            else if (strcmp(key, "min_mem_prep_added_ps") == 0) config.min_mem_prep_added_ps = value;
            else if (strcmp(key, "increase_ranking") == 0) config.increase_ranking = value;
        }
    }

    fclose(file);
    return 0;
}