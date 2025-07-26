#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include "../include/sem.h"
#include "../include/config.h"

#define POLICE_EXEC "./police"
#define GANG_EXEC "./gang"
#define GUI_EXEC "./gui"

int main() {
    if (load_config("config.txt") != 0) {
        fprintf(stderr, "Failed to load config.\n");
        exit(EXIT_FAILURE);
    }

    // === Init shared memory
    GangPoliceState* shared = init_shared_memory_gangs();
    key_t sem_key = ftok(FTOK_PATH, FTOK_PROJ_ID + 1);
    int sem_id = init_semaphore_set(sem_key, SEM_COUNT);

    // Initialize each gang semaphore
    for (int i = 0; i < config.gang_count; i++) {
        set_semaphore_value(sem_id, SEM_GANG_BASE + i, 1);
    }
    // Police and global counter locks
    set_semaphore_value(sem_id, SEM_POLICE, 1);
    set_semaphore_value(sem_id, SEM_GLOBAL, 1);

    shared->sem_id = sem_id;

    key_t key = ftok("/tmp", 65);
    if (key == -1) {
        perror("[MAIN] ftok failed");
        exit(EXIT_FAILURE);
    }

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[MAIN] msgget creation failed");
        exit(EXIT_FAILURE);
    }
    shared->msqid = msqid;
    // === Fork gang processes
    for (int i = 0; i < config.gang_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork gang");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child - run gang
            char gang_id_str[8];
            snprintf(gang_id_str, sizeof(gang_id_str), "%d", i);
            execl(GANG_EXEC, GANG_EXEC, gang_id_str, NULL);
            perror("execl gang");
            exit(EXIT_FAILURE);
        } else {
            // Parent - save PID
            shared->gang_pids[i] = pid;
        }
    }

    // === Fork police process
    pid_t police_pid = fork();
    if (police_pid < 0) {
        perror("fork police");
        exit(EXIT_FAILURE);
    } else if (police_pid == 0) {
        execl(POLICE_EXEC, POLICE_EXEC, NULL);
        perror("execl police");
        exit(EXIT_FAILURE);
    }


    // === Fork GUI process
    pid_t gui_pid = fork();
    if (gui_pid < 0) {
        perror("fork gui");
        exit(EXIT_FAILURE);
    } else if (gui_pid == 0) {
        execl(GUI_EXEC, GUI_EXEC, NULL);
        perror("execl gui");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sleep(1); // Sleep 1 second between checks

        if (shared->thwarted >= config.max_thwarted) {
            printf("[MAIN] Ending simulation: Max thwarted plans reached (%d)\n", shared->thwarted);
            break;
        }
        if (shared->successful >= config.max_successful) {
            printf("[MAIN] Ending simulation: Max successful plans reached (%d)\n", shared->successful);
            break;
        }
        if (shared->agents_exposed >= config.max_agents_exposed) {
            printf("[MAIN] Ending simulation: Max agents exposed reached (%d)\n", shared->agents_exposed);
            break;
        }
    }
    // Kill police
    kill(police_pid, SIGTERM);
    waitpid(police_pid, NULL, 0);

    kill(gui_pid, SIGTERM);
    waitpid(gui_pid, NULL, 0);

    // Kill gangs
    for (int i = 0; i < config.gang_count; i++) {
        kill(shared->gang_pids[i], SIGTERM);
        waitpid(shared->gang_pids[i], NULL, 0);
    }
    if (shmdt(shared) == -1) {
        perror("[MAIN] shmdt failed");
    }

    destroy_shared_memory_gangs();
    printf("[MAIN] Simulation complete. All processes terminated.\n");
    return 0;
}
