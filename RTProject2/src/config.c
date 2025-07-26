#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "config.h"

Config global_config;

BakeryState *shared_state = NULL;
int shm_id = -1;
int msgq_id = -1;

char* trim(char* str) {
    while (isspace(*str)) str++;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) *end-- = '\0';
    return str;
}

// Load config.txt
void load_config_from_file(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "[Config] Failed to open %s\n", filename);
        exit(1);
    }

char line[256];
while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || strlen(trim(line)) == 0) continue;

    char key[64], value[64];
    if (sscanf(line, "%[^=]=%s", key, value) != 2) continue;

    strcpy(key, trim(key));
    strcpy(value, trim(value));

    if (strcmp(key, "simulation_duration_minutes") == 0)
        global_config.simulation_duration_minutes = atoi(value);
    else if (strcmp(key, "profit_threshold") == 0)
        global_config.profit_threshold = atoi(value);
    else if (strcmp(key, "supply_min_add") == 0)
        global_config.supply_min_add = atoi(value);
    else if (strcmp(key, "supply_max_add") == 0)
        global_config.supply_max_add = atoi(value);
    else if (strcmp(key, "complaint_threshold") == 0)
        global_config.complaint_threshold = atoi(value);
    else if (strcmp(key, "frustrated_threshold") == 0)
        global_config.frustrated_threshold = atoi(value);
    else if (strcmp(key, "missing_item_threshold") == 0)
        global_config.missing_item_threshold = atoi(value);
    else if (strcmp(key, "num_chefs") == 0)
        global_config.num_chefs = atoi(value);
    else if (strcmp(key, "num_bakers") == 0)
        global_config.num_bakers = atoi(value);
    else if (strcmp(key, "num_sellers") == 0)
        global_config.num_sellers = atoi(value);
    else if (strcmp(key, "num_customers") == 0)
        global_config.num_customers = atoi(value);
    else if (strcmp(key, "num_supply_chain_workers") == 0)
        global_config.num_supply_chain_workers = atoi(value);
    else if (strcmp(key, "wheat") == 0)
        global_config.ingredient_limits[ING_WHEAT] = atoi(value);
    else if (strcmp(key, "yeast") == 0)
        global_config.ingredient_limits[ING_YEAST] = atoi(value);
    else if (strcmp(key, "butter") == 0)
        global_config.ingredient_limits[ING_BUTTER] = atoi(value);
    else if (strcmp(key, "milk") == 0)
        global_config.ingredient_limits[ING_MILK] = atoi(value);
    else if (strcmp(key, "sugar") == 0)
        global_config.ingredient_limits[ING_SUGAR] = atoi(value);
    else if (strcmp(key, "salt") == 0)
        global_config.ingredient_limits[ING_SALT] = atoi(value);
    else if (strcmp(key, "cheese") == 0)
        global_config.ingredient_limits[ING_CHEESE] = atoi(value);
    else if (strcmp(key, "salami") == 0)
        global_config.ingredient_limits[ING_SALAMI] = atoi(value);
    else if (strcmp(key, "sweet_items") == 0)
        global_config.ingredient_limits[ING_SWEETITEMS] = atoi(value);
    else if (strcmp(key, "bread_price") == 0)
        global_config.item_prices[ITEM_BREAD] = atoi(value);
    else if (strcmp(key, "cake_price") == 0)
        global_config.item_prices[ITEM_CAKE] = atoi(value);
    else if (strcmp(key, "sweet_price") == 0)
        global_config.item_prices[ITEM_SWEET] = atoi(value);
    else if (strcmp(key, "sandwich_price") == 0)
        global_config.item_prices[ITEM_SANDWICH] = atoi(value);
    else if (strcmp(key, "sweet_patisserie_price") == 0)
        global_config.item_prices[ITEM_SWEET_PATISSERIE] = atoi(value);
    else if (strcmp(key, "savory_patisserie_price") == 0)
        global_config.item_prices[ITEM_SAVORY_PATISSERIE ] = atoi(value);
    else if (strcmp(key, "gui_enabled") == 0)
        global_config.gui_enabled = atoi(value);
}
    fclose(file);
}

// Shared Memory
BakeryState* init_shared_memory() {
    shm_id = shmget(SHM_KEY, sizeof(BakeryState), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    shared_state = (BakeryState *) shmat(shm_id, NULL, 0);
    if (shared_state == (void *) -1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    memset(shared_state, 0, sizeof(BakeryState));
    return shared_state;
}

BakeryState* get_shared_memory() {
    int id = shmget(SHM_KEY, sizeof(BakeryState), 0666);
    if (id == -1) {
        perror("shmget (get) failed");
        exit(EXIT_FAILURE);
    }
    BakeryState *ptr = (BakeryState *) shmat(id, NULL, 0);
    if (ptr == (void *) -1) {
        perror("shmat (get) failed");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void destroy_shared_memory() {
    if (shared_state != NULL)
        shmdt((void *)shared_state);
    if (shm_id != -1)
        shmctl(shm_id, IPC_RMID, NULL);
}

// Message Queue
int init_message_queue() {
    msgq_id = msgget(MSGQ_KEY, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    return msgq_id;
}

void destroy_message_queue(int id) {
    if (msgctl(id, IPC_RMID, NULL) == -1) {
        perror("msgctl destroy failed");
        exit(EXIT_FAILURE);
    }
}

bool is_already_requested(int item, int index, int* array) {
    for (int i = 0; i < index; i++) {
        if (array[i] == item) return true;
    }
    return false;
}


void print_shared_state(BakeryState *state) {
    printf("\n========== Shared Memory Debug ==========");
    printf("\n Ingredient Stock:\n");
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        printf("  Ingredient %d: %d\n", i, state->ingredient_stock[i]);
    }
    printf("\n Prepared Items:\n");
    for (int i = 0; i < NUM_ITEM_TYPES + 1; i++) {
        printf("  Item %d: %d\n", i, state->prepared_items[i]);
    }
    printf("\n Baked Items:\n");
    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        printf("  Item %d: %d\n", i, state->baked_items[i]);
    }
    printf("\n Stats:\n");
    printf("  Profit: %d\n", state->profit);
    printf("  Complaints: %d\n", state->complaining_customers);
    printf("  Frustrated: %d\n", state->frustrated_customers);
    printf("  Missing Items: %d\n", state->missing_item_customers);
    printf("  Total Customers: %d\n", state->total_customers);
    printf("  Elapsed Minutes: %d\n", state->simulation_minutes_elapsed);
    printf("\n=========================================\n\n");
}


