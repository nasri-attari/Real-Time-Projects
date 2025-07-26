#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "config.h"
#include "sem.h"
#include <signal.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Number of arguments must be greater than 2\n");
        return -1;
    }
    BakeryState* state = get_shared_memory();
    int semid = init_semaphore_set(SEMKEY_STOCK, 5);
    int baker_num = atoi(argv[1]);
    while (1) {
        int my_role = state->baker_roles[baker_num];
        if (my_role == ROLE_BAKE_CAKE_SWEET) {
            while (1) {
                if (state->prepared_items[ITEM_CAKE] ) {
                        state->baker_roles[baker_num] = my_role;
                        sem_wait_index(semid, SEM_IDX_CHEF);
                        state->prepared_items[ITEM_CAKE]-=1;
                        sem_post_index(semid, SEM_IDX_CHEF);

                        float random_seconds = 0.5 + ((rand() % 2500) / 1000.0); // Random between 0.5 and 2.5
                        clock_t start_time = clock();

                        while (1) {
                            clock_t current_time = clock();
                            double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                            if (elapsed_time >= random_seconds) {
                                break;
                            }
                        }
                        sem_wait_index(semid, SEM_IDX_BAKE);
                        state->baked_items[ITEM_CAKE] += 1;
                        sem_post_index(semid, SEM_IDX_BAKE);
                }
                else {
                    state->baker_roles[baker_num] = ROLE_BAKE_NOT_WORKING;
                    sleep(1);
                }
                if (state->prepared_items[ITEM_SWEET] != 0) {
                    state->baker_roles[baker_num] = my_role;
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SWEET]-=1;
                    sem_post_index(semid, SEM_IDX_CHEF);

                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();

                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;

                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_BAKE);
                    state->baked_items[ITEM_SWEET] += 1;
                    sem_post_index(semid, SEM_IDX_BAKE);
                }
                else {
                    state->baker_roles[baker_num] = ROLE_BAKE_NOT_WORKING;
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_BAKE_PASTRY) {
            while (1) {
                if (state->prepared_items[ITEM_SAVORY_PATISSERIE] != 0 ) {
                    state->baker_roles[baker_num] = my_role;
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SAVORY_PATISSERIE] -= 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();

                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;

                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_BAKE);
                    state->baked_items[ITEM_SAVORY_PATISSERIE] += 1;
                    sem_post_index(semid, SEM_IDX_BAKE);
                    }
                else {
                    state->baker_roles[baker_num] = ROLE_BAKE_NOT_WORKING;
                    sleep(1);
                }
                if (state->prepared_items[ITEM_SWEET_PATISSERIE] != 0) {
                    state->baker_roles[baker_num] = my_role;
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SWEET_PATISSERIE] -= 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();

                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_BAKE);
                    state->baked_items[ITEM_SWEET_PATISSERIE] += 1;
                    sem_post_index(semid, SEM_IDX_BAKE);
                    }
                else {
                    state->baker_roles[baker_num] = ROLE_BAKE_NOT_WORKING;
                    sleep(1);
                }
            }
        }

        else if (my_role == ROLE_BAKE_BREAD) {
            while (1) {
                if (state->prepared_items[ITEM_PASTE] != 0) {
                    state->baker_roles[baker_num] = my_role;
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_PASTE] -= 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();
                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_BAKE);
                    state->baked_items[ITEM_BREAD] += 1;
                    sem_post_index(semid, SEM_IDX_BAKE);
                    }
                else {
                    state->baker_roles[baker_num] = ROLE_BAKE_NOT_WORKING;
                    sleep(1);
                }
            }
        }
    }
    return 0;
}
