#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <time.h>
#include "config.h"
#include "sem.h"

pid_t child_pids[MAX_CHILDREN];
pid_t chef_pid[MAX_CHEFS];
int num_children = 0;
int chefs_counter = 0;

int role_to_item_type[] = {
    -1,
    ITEM_CAKE,
    ITEM_SWEET,
    ITEM_SWEET_PATISSERIE,
    ITEM_SAVORY_PATISSERIE,
    ITEM_SANDWICH,
    ITEM_PASTE
};

int item_stock(int item, BakeryState* state) {
    if (item == ITEM_SANDWICH)
        return state->prepared_items[item];
    return state->baked_items[item];
}
int count_workers(int role, BakeryState *state) {
    int count = 0;
    for (int i = 0; i < MAX_CHEFS; i++) {
        if (state->chef_roles[i] == role) {
            count++;
        }
    }
    return count;
}
int get_worker_with_role(int role, BakeryState *state) {
    for (int i = 0; i < MAX_CHEFS; i++) {
        if (state->chef_roles[i] == role) {
            return i;
        }
    }
    return -1;
}
void manager_decision(BakeryState *state) {
    int shortage_roles[MAX_CHEFS];
    int shortage_count = 0;

    int surplus_roles[MAX_CHEFS];
    int surplus_count = 0;

    // Classify the roles have shortage in items
    for (int role = 1; role <= ROLE_PASTE; role++) {
        int item = role_to_item_type[role];
        if (item == -1) continue;

        int stock = item_stock(item, state);
        int workers = count_workers(role, state);

        if (stock <= ITEM_THRESHOLD) {
            shortage_roles[shortage_count++] = role;
        } else if (workers >= 1) {
            surplus_roles[surplus_count++] = role;
        }
    }

	int s = 0;
	for (int i = 0; i < shortage_count && s < surplus_count; i++) {
    	int target_role = shortage_roles[i];
    	int donor_role = surplus_roles[s];

    	if (target_role == donor_role || target_role == ROLE_NOT_WORKING) {
         s++;
       	 continue;
    	}

    	int chef_index = get_worker_with_role(donor_role, state);
    	if (chef_index == -1) {
        	s++;
        	continue;
    }
	if (count_workers(target_role, state) >= 4) {
    	continue;
	}
    // Reassign the chef
    state->chef_roles[chef_index] = target_role;
    kill(chef_pid[chef_index], SIGUSR1);
    s++;
   }

}

void cleanup(BakeryState* state, int semid, int msgid) {
    printf("\n[Manager] Cleaning resources...\n");
    for (int i = 0; i < num_children; i++) {
        kill(child_pids[i], SIGTERM);
    }
    destroy_shared_memory();
    destroy_semaphore_set(semid);
    destroy_message_queue(msgid);
    printf("[Manager] Cleanup complete.\n");
}

int main() {

    srand(getpid());
    printf("[Manager] Loading config.txt...\n");
    load_config_from_file("config.txt");

    printf("[Manager] Initializing shared memory...\n");
    BakeryState* state = init_shared_memory();
    snprintf(state->manager_command, 128, "INITIALIZING...");
	time_t init_msg_start = time(NULL);
	time_t boost_msg_start = 0;
	int boost_active = 0;
    time_t feedback_msg_start = 0;
	int feedback_active = 0;
    int refund = 0;

    int chef_roles[] = { ROLE_PASTE, ROLE_CAKE, ROLE_SANDWICH, ROLE_SWEET, ROLE_SWEET_PASTRY, ROLE_SAVORY_PASTRY };
    for (int i = 0; i < global_config.num_chefs; i++) {
        state->chef_roles[i] = chef_roles[i % 6];
    }

    int baker_roles[] = { ROLE_BAKE_CAKE_SWEET, ROLE_BAKE_PASTRY, ROLE_BAKE_BREAD };
    for (int i = 0; i < global_config.num_bakers; i++) {
        state->baker_roles[i] = baker_roles[i % 3];
    }

    printf("[Manager] Initializing semaphore set...\n");
    int semid = init_semaphore_set(SEMKEY_STOCK, 5);
    set_semaphore_value(semid, SEM_IDX_STOCK, 1);
    set_semaphore_value(semid, SEM_IDX_BAKE, 1);
    set_semaphore_value(semid, SEM_IDX_PROFIT, 1);
    set_semaphore_value(semid, SEM_IDX_CUSTOMERS, 1);
    set_semaphore_value(semid, SEM_IDX_CHEF, 1);

    printf("[Manager] Initializing message queue...\n");
    int msgid = init_message_queue();

    if (global_config.gui_enabled) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("./bin/gui", "gui", NULL);
            perror("execl gui failed");
            exit(1);
        }
        child_pids[num_children++] = pid;
        printf("[Manager] Launched GUI process (PID: %d)\n", pid);
    }

    for (int i = 0; i < global_config.num_supply_chain_workers; i++) {
        char index_str[16];
		snprintf(index_str, sizeof(index_str), "%d", i);

		pid_t pid = fork();
		if (pid == 0) {
    		execl("./bin/supply_chain", "supply_chain", index_str, NULL);
    		perror("execl supply_chain failed");
    		exit(1);
		}
        child_pids[num_children++] = pid;
    }

    for (int i = 0; i < global_config.num_chefs; i++) {
        char arg[16]; snprintf(arg, sizeof(arg), "%d", i);
        pid_t pid = fork();
        if (pid == 0) execl("./bin/chef", "chef", arg, NULL);
        child_pids[num_children++] = pid;
        chef_pid[chefs_counter++] = pid;
    }

    for (int i = 0; i < global_config.num_bakers; i++) {
        char arg[16]; snprintf(arg, sizeof(arg), "%d", i);
        pid_t pid = fork();
        if (pid == 0) execl("./bin/baker", "baker", arg, NULL);
        child_pids[num_children++] = pid;
    }

    for (int i = 0; i < global_config.num_sellers; i++) {
    	state->seller_active[i] = 2;

    	pid_t pid = fork();
    	if (pid == 0) {
        	char index_str[16];
        	snprintf(index_str, sizeof(index_str), "%d", i);
        	execl("./bin/seller", "seller", index_str, NULL);
        	perror("execl seller failed");
        	exit(1);
    	}
    	child_pids[num_children++] = pid;
	}

    int customers_spawned = 0;
    time_t last_customer_time = time(NULL);
    time_t simulation_start_time = time(NULL);
    int prev_missing = state->missing_item_customers;
    sleep(5);
    while (1) {
        state->simulation_minutes_elapsed = difftime(time(NULL), simulation_start_time);
        time_t now = time(NULL);
      	if (prev_missing != state->missing_item_customers) {
    		boost_msg_start = time(NULL);
    		boost_active = 1;
    		manager_decision(state);
    		prev_missing = state->missing_item_customers;
		}

        // Launch new customer every 2 seconds
        if (customers_spawned < global_config.num_customers && difftime(now, last_customer_time) >= 2.0) {
            pid_t pid = fork();
            if (pid == 0) {
                char index_str[16];
                snprintf(index_str, sizeof(index_str), "%d", customers_spawned);
                execl("./bin/customer", "customer", index_str, NULL);
                perror("execl failed");
                exit(1);
            }
            child_pids[num_children++] = pid;
            customers_spawned++;
            last_customer_time = now;
        }

        ManagerMessage msg;
        if (msgrcv(msgid, &msg, sizeof(ManagerMessage) - sizeof(long), 2, IPC_NOWAIT) != -1) {
    		feedback_msg_start = time(NULL);
    		feedback_active = 1;
            refund = 0;
            for (int i = 0; i < msg.num_items; i++) {
                int item_code = msg.item_codes[i];
                int quantity = msg.quantities[i];
                int price = global_config.item_prices[item_code];
                sem_wait_index(semid, SEM_IDX_PROFIT);
                state->profit -= price * quantity;
                refund += price * quantity;
                sem_post_index(semid, SEM_IDX_PROFIT);
            }
        }
        print_shared_state(state);
    	if (state->complaining_customers >= global_config.complaint_threshold ||
    		state->frustrated_customers >= global_config.frustrated_threshold ||
    		state->missing_item_customers >= global_config.missing_item_threshold ||
    		state->profit >= global_config.profit_threshold ||
    		difftime(time(NULL), simulation_start_time) >= global_config.simulation_duration_minutes * 60) {
    		printf("[Manager] Termination condition met. Ending simulation...\n");
    		break;
		}

		if (boost_active) {
    		if (difftime(now, boost_msg_start) >= 3) {
        		boost_active = 0;
    		} else {
        		snprintf(state->manager_command, 128, "Boosting preparation of low-prepared items");
    		}
		}

		if (!boost_active && feedback_active) {
    		if (difftime(now, feedback_msg_start) >= 4) {
        		feedback_active = 0;
    		} else {
        		snprintf(state->manager_command, 128, "feedback from Customer %d — refund issued: $%d", msg.customer_number, refund);
    		}
		}

		if (!boost_active && !feedback_active && difftime(now, init_msg_start) >= 5) {
    		snprintf(state->manager_command, 128, "Welcome! Your feedback means a lot to us!");
		}

        sleep(1);
    }
    cleanup(state, semid, msgid);
    return 0;
}
