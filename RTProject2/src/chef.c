#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "config.h"
#include "sem.h"
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t check_role = 0;

void handle_signal(int sig) {
        check_role = 1;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (argc < 2) {
        printf("Number of arguments must be greater than 2\n");
        return -1;
    }
    BakeryState* state = get_shared_memory();
    int semid = init_semaphore_set(SEMKEY_STOCK, 5);
    int chef_num = atoi(argv[1]);
    while (1) {
        int my_role = state->chef_roles[chef_num];
        if (check_role == 1){
            check_role = 0;
            my_role = state->chef_roles[chef_num];
        }

        if (my_role == ROLE_PASTE) {
            while (!check_role) {
                if (state->ingredient_stock[ING_WHEAT]*state->ingredient_stock[ING_SALT] *
                    state->ingredient_stock[ING_SUGAR] * state->ingredient_stock[ING_MILK] *
                    state->ingredient_stock[ING_BUTTER] * state->ingredient_stock[ING_YEAST] != 0) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_WHEAT] -= 1;
                    state->ingredient_stock[ING_SALT] -= 1;
                    state->ingredient_stock[ING_SUGAR] -= 1;
                    state->ingredient_stock[ING_MILK] -=1;
                    state->ingredient_stock[ING_BUTTER]-=1;
                    state->ingredient_stock[ING_YEAST]-=1;
                    sem_post_index(semid, SEM_IDX_STOCK);

                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();
                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_PASTE] += 2;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    }
                else {
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_CAKE) {
            while (!check_role) {
                if (state->ingredient_stock[ING_WHEAT] *state->ingredient_stock[ING_SUGAR]
                    * state->ingredient_stock[ING_MILK] *state->ingredient_stock[ING_BUTTER] *
                    state->ingredient_stock[ING_YEAST] != 0) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_WHEAT] -= 1;
                    state->ingredient_stock[ING_SUGAR] -= 1;
                    state->ingredient_stock[ING_MILK] -=1;
                    state->ingredient_stock[ING_BUTTER]-=1;
                    state->ingredient_stock[ING_YEAST]-=1;
                    sem_post_index(semid, SEM_IDX_STOCK);

                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();
                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_CAKE] += 1;
                    sem_post_index(semid, SEM_IDX_CHEF);

                    }
                else {
                    sleep(1);
                }
            }
        }

        else if (my_role == ROLE_SANDWICH) {
            while (!check_role) {
                if (state->baked_items[ITEM_BREAD] * state->ingredient_stock[ING_SALAMI] *
                    state->ingredient_stock[ING_CHEESE] != 0 ) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_SALAMI] -= 1;
                    state->ingredient_stock[ING_CHEESE] -= 1;
                    sem_post_index(semid, SEM_IDX_STOCK);
                    sem_wait_index(semid, SEM_IDX_BAKE);
                    state->baked_items[ITEM_BREAD] -=1;
                    sem_post_index(semid, SEM_IDX_BAKE);

                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);
                    clock_t start_time = clock();
                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SANDWICH] += 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    }
                else {
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_SWEET) {
            while (!check_role) {
                if (state->ingredient_stock[ING_BUTTER] * state->ingredient_stock[ING_SUGAR]
                    * state->ingredient_stock[ING_MILK] * state->ingredient_stock[ING_SWEETITEMS] != 0) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_BUTTER] -= 1;
                    state->ingredient_stock[ING_SUGAR] -= 1;
                    state->ingredient_stock[ING_MILK] -=1;
                    state->ingredient_stock[ING_SWEETITEMS] -=1;
                    sem_post_index(semid, SEM_IDX_STOCK);

                    float random_seconds = 0.5 + ((rand() % 2500) / 1000.0);

                    clock_t start_time = clock();
                    while (1) {
                        clock_t current_time = clock();
                        double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
                        if (elapsed_time >= random_seconds) {
                            break;
                        }
                    }
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SWEET] += 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    }
                else {
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_SAVORY_PASTRY) {
            while (!check_role) {
                if (state->ingredient_stock[ING_BUTTER] * state->ingredient_stock[ING_SALAMI]
                    * state->ingredient_stock[ING_CHEESE] * state->prepared_items[ITEM_PASTE] != 0) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_BUTTER] -= 1;
                    state->ingredient_stock[ING_SALAMI] -= 1;
                    state->ingredient_stock[ING_CHEESE] -=1;
                    sem_post_index(semid, SEM_IDX_STOCK);
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_PASTE] -=1;
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
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SAVORY_PATISSERIE] += 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    }
                else {;
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_SWEET_PASTRY) {
            while (!check_role) {
                if (state->ingredient_stock[ING_BUTTER] * state->ingredient_stock[ING_SUGAR]
                    * state->ingredient_stock[ING_SWEETITEMS] * state->prepared_items[ITEM_PASTE] != 0) {
                    sem_wait_index(semid, SEM_IDX_STOCK);
                    state->ingredient_stock[ING_BUTTER] -= 1;
                    state->ingredient_stock[ING_SUGAR] -= 1;
                    state->ingredient_stock[ING_SWEETITEMS] -=1;
                    sem_post_index(semid, SEM_IDX_STOCK);
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_PASTE] -=1;
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
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[ITEM_SWEET_PATISSERIE] += 1;
                    sem_post_index(semid, SEM_IDX_CHEF);
                    }
                else {
                    sleep(1);
                }
            }
        }
        else if (my_role == ROLE_NOT_WORKING) {
            while (!check_role) {
            }
        }

    }

    return 0;
}

