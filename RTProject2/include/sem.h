#ifndef SEM_H
#define SEM_H

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SEM_FLAGS 0  // blocking like FIFO

// Create a set of semaphores
int init_semaphore_set(key_t key, int count) {
    int semid = semget(key, count, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget (set) failed");
        exit(1);
    }
    return semid;
}

// Set value of a specific semaphore in the set
void set_semaphore_value(int semid, int index, int value) {
    if (semctl(semid, index, SETVAL, value) == -1) {
        perror("semctl SETVAL failed");
        exit(1);
    }
}

// Wait (P operation) on a specific semaphore index (FIFO-like with blocking)
void sem_wait_index(int semid, int index) {
    struct sembuf op;
    op.sem_num = index;
    op.sem_op = -1;
    op.sem_flg = SEM_FLAGS;

    while (semop(semid, &op, 1) == -1) {
        if (errno != EINTR) {
            perror("semop wait failed");
            exit(1);
        }
    }
}

// Signal (V operation) on a specific semaphore index
void sem_post_index(int semid, int index) {
    struct sembuf op;
    op.sem_num = index;
    op.sem_op = 1;
    op.sem_flg = SEM_FLAGS;

    if (semop(semid, &op, 1) == -1) {
        perror("semop post failed");
        exit(1);
    }
}

// Destroy the entire semaphore set
void destroy_semaphore_set(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
        exit(1);
    }
}

#endif
