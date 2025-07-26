#ifndef CONFIG_H
#define CONFIG_H

// Constants
#define NUM_INGREDIENTS 9
#define NUM_ITEM_TYPES  5

#define MAX_CHEFS          20
#define MAX_BAKERS         20
#define MAX_SELLERS        6
#define MAX_SUPPLY_WORKERS 20
#define MAX_CUSTOMERS      100

// Item Types
#define ITEM_BREAD              0
#define ITEM_PASTE              0
#define ITEM_CAKE               1
#define ITEM_SWEET              2
#define ITEM_SWEET_PATISSERIE   3
#define ITEM_SAVORY_PATISSERIE  4
#define ITEM_SANDWICH           5

// Ingredient Indexes
#define ING_WHEAT       0
#define ING_YEAST       1
#define ING_BUTTER      2
#define ING_MILK        3
#define ING_SUGAR       4
#define ING_SALT        5
#define ING_SWEETITEMS  6
#define ING_CHEESE      7
#define ING_SALAMI      8

// Chef Roles
#define ROLE_NOT_WORKING     0
#define ROLE_CAKE            1
#define ROLE_SWEET           2
#define ROLE_SWEET_PASTRY    3
#define ROLE_SAVORY_PASTRY   4
#define ROLE_SANDWICH        5
#define ROLE_PASTE           6

// Seller & Suppliers Roles
#define NOT_EXIST     0
#define WORKING       1
#define NOT_WORKING    2

// Baker Roles
#define ROLE_BAKE_CAKE_SWEET 10
#define ROLE_BAKE_PASTRY     11
#define ROLE_BAKE_BREAD      12
#define ROLE_BAKE_NOT_WORKING 13

// Semaphore Indexes
#define SEM_IDX_STOCK     0
#define SEM_IDX_BAKE      1
#define SEM_IDX_PROFIT    2
#define SEM_IDX_CUSTOMERS 3
#define SEM_IDX_CHEF      4

// customer_status
#define CUSTOMER_NOT_EXIST    0
#define CUSTOMER_EXIST        1
#define CUSTOMER_FRUSTRATED   2
#define CUSTOMER_COMPLAINING  3
#define CUSTOMER_NO_ITEMS     4
#define CUSTOMER_FINISHED     5

#define MAX_CHILDREN 100
#define ITEM_THRESHOLD 5

// Config Struct
typedef struct {
    int num_chefs;
    int num_bakers;
    int num_sellers;
    int num_customers;
    int num_supply_chain_workers;

    int ingredient_limits[NUM_INGREDIENTS];
    int item_prices[NUM_ITEM_TYPES + 1];

    int simulation_duration_minutes;
    int profit_threshold;
    int complaint_threshold;
    int frustrated_threshold;
    int missing_item_threshold;
    int supply_min_add;
    int supply_max_add;
    int gui_enabled;
} Config;

extern Config global_config;

// Shared Memory Struct
typedef struct {
    int ingredient_stock[NUM_INGREDIENTS];
    int prepared_items[NUM_ITEM_TYPES + 1];
    int baked_items[NUM_ITEM_TYPES];

    int profit;
    int frustrated_customers;
    int complaining_customers;
    int missing_item_customers;
    int total_customers;
    int simulation_minutes_elapsed;

    int chef_roles[MAX_CHEFS];
    int baker_roles[MAX_BAKERS];
    int seller_active[MAX_SELLERS];
    int supply_worker_active[MAX_SUPPLY_WORKERS];
    int customer_status[MAX_CUSTOMERS];
    char customer_requests[MAX_CUSTOMERS][128];
    char seller_responds[MAX_SELLERS][128];
    char manager_command[128];
} BakeryState;

// IPC Constants
#define SHM_KEY          0x1234
#define MSGQ_KEY         0x5678
#define SEMKEY_STOCK     2341

// Message Queue
typedef struct {
    long mtype;
    int sender_pid;
    int status_code;
    int customer_number;
    char message[128];
    int num_items;
    int item_codes[6];
    int quantities[6];
} ManagerMessage;

typedef struct {
    long mtype;
    int sender_pid;
    int customer_number;
    int num_items;
    int item_codes[6];
    int quantities[6];
} CustomerRequest;

typedef struct {
    long mtype;
    int success;
    int missing_item_index;
    char message[128];
} SellerResponse;

#include <stdbool.h>
// Function Declarations
BakeryState* init_shared_memory();
BakeryState* get_shared_memory();
void destroy_shared_memory();
int init_message_queue();
void destroy_message_queue(int id);
void load_config_from_file(const char* filename);
bool is_already_requested(int item, int index, int* array);
void print_shared_state(BakeryState *state);
#endif
