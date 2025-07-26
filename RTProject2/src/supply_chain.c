#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "config.h"
#include "sem.h"
const char* ingredient_names[NUM_INGREDIENTS] = {
    "Wheat",      // 0
    "Yeast",      // 1
    "Butter",     // 2
    "Milk",       // 3
    "Sugar",      // 4
    "Salt",       // 5
    "Sweet Items", // 6
    "Cheese",     // 7
    "Salami",     // 8
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <customer_index>\n", argv[0]);
        exit(1);
    }
    srand(getpid());
    load_config_from_file("config.txt");

    BakeryState* state = get_shared_memory();

    int semid = init_semaphore_set(SEMKEY_STOCK, 5);
    int supply_index = atoi(argv[1]);

    sem_wait_index(semid, SEM_IDX_STOCK);
    state->supply_worker_active[supply_index] = WORKING;
    sem_post_index(semid, SEM_IDX_STOCK);

    while (1) {
        int all_full = 1;
        sem_wait_index(semid, SEM_IDX_STOCK);
        for (int i = 0; i < NUM_INGREDIENTS; i++) {
            int current = state->ingredient_stock[i];
            int max = global_config.ingredient_limits[i];

            if (current < max) {
                all_full = 0;
                int add = (rand() % (global_config.supply_max_add - global_config.supply_min_add + 1)) + global_config.supply_min_add;
                if (current + add > max) add = max - current;
                if (add > 0) {
                    state->ingredient_stock[i] += add;
                }
            }
        }
        state->supply_worker_active[supply_index] = all_full ? NOT_WORKING : WORKING;
        sem_post_index(semid, SEM_IDX_STOCK);
        sleep(5);
    }
    return 0;
}