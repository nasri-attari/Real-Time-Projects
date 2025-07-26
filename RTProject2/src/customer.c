#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <stdbool.h>
#include "config.h"
#include "sem.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <customer_index>\n", argv[0]);
        exit(1);
    }
    BakeryState* state = get_shared_memory();
    int customer_index = atoi(argv[1]);
    srand(getpid());
    load_config_from_file("config.txt");

    int msgid = init_message_queue();

    state->customer_status[customer_index] = CUSTOMER_EXIST;
    sleep(10);

    CustomerRequest request;
    SellerResponse response;
    ManagerMessage mmsg;

    int semid = init_semaphore_set(SEMKEY_STOCK, 5);

    // Build a random order
    request.mtype = 1;
    request.sender_pid = getpid();
    request.customer_number = customer_index;
    mmsg.customer_number = request.customer_number;
    request.num_items = (rand() % 3) + 1;
    mmsg.num_items = request.num_items;

    for (int i = 0; i < request.num_items; ) {
    	int item = rand() % (NUM_ITEM_TYPES + 1);
    	if (!is_already_requested(item, i, request.item_codes)) {
        	request.item_codes[i] = item;
        	request.quantities[i] = (rand() % 3) + 1;
            mmsg.item_codes[i] = request.item_codes[i];
            mmsg.quantities[i] = request.quantities[i];
        	i++;
    	}
	}

    const char* item_names[] = {
    "B", "C", "SW", "SW-P", "SA-P", "SA"
	};

	char request_string[128] = "";
	for (int i = 0; i < request.num_items; i++) {
    	char temp[32];
    	snprintf(temp, sizeof(temp), "%d x %s", request.quantities[i], item_names[request.item_codes[i]]);
    	strcat(request_string, temp);
    	if (i < request.num_items - 1) strcat(request_string, ", ");
	}

    snprintf(state->customer_requests[customer_index], sizeof(state->customer_requests[customer_index]), "%s", request_string);
    state->customer_requests[customer_index][sizeof(state->customer_requests[customer_index]) - 1] = '\0';

    // Send order
    if (msgsnd(msgid, &request, sizeof(CustomerRequest) - sizeof(long), 0) == -1) {
        perror("[Customer] Failed to send order");
        exit(1);
    }

    // Wait for response
    int timeout_seconds = 2;
    int elapsed = 0, got_response = 0;

    while (elapsed < timeout_seconds) {
        if (msgrcv(msgid, &response, sizeof(SellerResponse) - sizeof(long), getpid(), IPC_NOWAIT) != -1) {
            got_response = 1;
            break;
        }
        sleep(1);
        elapsed++;
    }
    sleep(5);
    if (!got_response) {
        state->customer_status[customer_index] = CUSTOMER_FRUSTRATED;
        snprintf(state->customer_requests[customer_index], 128, "I'm leaving!");
        sleep(5);
        mmsg.mtype = 2;
        mmsg.sender_pid = getpid();
        mmsg.status_code = 2;
        sem_wait_index(semid, SEM_IDX_CUSTOMERS);
        state->frustrated_customers++;
        sem_post_index(semid, SEM_IDX_CUSTOMERS);
        snprintf(mmsg.message, sizeof(mmsg.message), "Customer %d frustrated (timeout).", getpid());
        state->customer_status[customer_index] = CUSTOMER_FINISHED;
        exit(0);
    }

    // Handle response
    if (response.success) {
        snprintf(state->customer_requests[customer_index], 128, "THE BEST BAKERY!");
        if (rand() % 10 == 0) {
            state->customer_status[customer_index] = CUSTOMER_COMPLAINING;
            snprintf(state->customer_requests[customer_index], 128, "Bad Items!");
            sleep(5);
            mmsg.mtype = 2;
            mmsg.sender_pid = getpid();
            mmsg.status_code = 1;
            sem_wait_index(semid, SEM_IDX_CUSTOMERS);
        	state->complaining_customers++;
        	sem_post_index(semid, SEM_IDX_CUSTOMERS);
            snprintf(mmsg.message, sizeof(mmsg.message), "Customer %d complained about quality.", getpid());
            msgsnd(msgid, &mmsg, sizeof(ManagerMessage) - sizeof(long), 0);
        }
        sleep(5);
    } else {
        state->customer_status[customer_index] = CUSTOMER_NO_ITEMS;
        snprintf(state->customer_requests[customer_index], 128, "Unbelievable! No items!");
        sleep(5);
        mmsg.mtype = 2;
        mmsg.sender_pid = getpid();
        mmsg.status_code = 3;
        sem_wait_index(semid, SEM_IDX_CUSTOMERS);
        state->missing_item_customers++;
        sem_post_index(semid, SEM_IDX_CUSTOMERS);

        snprintf(mmsg.message, sizeof(mmsg.message),
                 "Customer %d missing item %d.", getpid(), response.missing_item_index);
    }
    snprintf(state->customer_requests[customer_index], 128, "BYE!");
    sleep(5);
    state->customer_status[customer_index] = CUSTOMER_FINISHED;
    exit(0);
}
