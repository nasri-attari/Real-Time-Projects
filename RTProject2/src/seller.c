#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>
#include "config.h"
#include "sem.h"

int main(int argc, char* argv[]) {
     if (argc < 2) {
       fprintf(stderr, "Usage: %s <customer_index>\n", argv[0]);
       exit(1);
    }
    load_config_from_file("config.txt");
    BakeryState* state = get_shared_memory();
    int semid = init_semaphore_set(SEMKEY_STOCK, 5);
    int msgid = init_message_queue();

    int seller_index = atoi(argv[1]);
    srand(time(NULL));
    sem_wait_index(semid, SEM_IDX_CUSTOMERS);
    state->seller_active[seller_index] = NOT_WORKING;
    sem_post_index(semid, SEM_IDX_CUSTOMERS);
    while (1) {
        CustomerRequest request;
        SellerResponse response;

        if (msgrcv(msgid, &request, sizeof(CustomerRequest) - sizeof(long), 1, 0) == -1) {
            perror("[Seller] Failed to receive customer request");
            continue;
        }

        int can_fulfill = 1;
        int missing_index = -1;

        // Check stock
        state->seller_active[seller_index] = WORKING;
        int customer_num = request.customer_number;
        for (int i = 0; i < request.num_items; i++) {
            int item_code = request.item_codes[i];
            int quantity = request.quantities[i];

            if (item_code == ITEM_SANDWICH) {
                sem_wait_index(semid, SEM_IDX_CHEF);
                if (state->prepared_items[item_code] < quantity) {
                 	can_fulfill = 0;
                	missing_index = item_code;
                }
                sem_post_index(semid, SEM_IDX_CHEF);
            } else {
              sem_wait_index(semid, SEM_IDX_BAKE);
              if (state->baked_items[item_code] < quantity) {
              	can_fulfill = 0;
              	missing_index = item_code;
              }
              sem_post_index(semid, SEM_IDX_BAKE);
            }
            if (!can_fulfill) break;
        }

        if (can_fulfill) {
            for (int i = 0; i < request.num_items; i++) {
                int item_code = request.item_codes[i];
                int quantity = request.quantities[i];
                if (item_code != ITEM_SANDWICH) {
                    sem_wait_index(semid, SEM_IDX_BAKE);
                	state->baked_items[item_code] -= quantity;
                    sem_post_index(semid, SEM_IDX_BAKE);
                }
                else{
                    sem_wait_index(semid, SEM_IDX_CHEF);
                    state->prepared_items[item_code] -= quantity;
                    sem_post_index(semid, SEM_IDX_CHEF);
                }
                int price = global_config.item_prices[item_code];
                sem_wait_index(semid, SEM_IDX_PROFIT);
                state->profit += price * quantity;
                sem_post_index(semid, SEM_IDX_PROFIT);
            }

            response.mtype = request.sender_pid;
            response.success = 1;
            response.missing_item_index = -1;
            snprintf(response.message, sizeof(response.message), "Order completed successfully!");
            snprintf(state->seller_responds[seller_index], 128, "Served customer%d ", customer_num);
        } else {
            response.mtype = request.sender_pid;
            response.success = 0;
            response.missing_item_index = missing_index;
            snprintf(response.message, sizeof(response.message),
                     "Not enough stock for item %d!", missing_index);
            snprintf(state->seller_responds[seller_index], 128, "No items for C%d", customer_num);
        }
        srand(time(NULL) ^ getpid() ^ rand());
        // Send response back to customer
        if (rand() % 100 < 15) {
           snprintf(state->seller_responds[seller_index], 128, "So Tired Can't Work...");
           sleep(2);
		}
        if (msgsnd(msgid, &response, sizeof(SellerResponse) - sizeof(long), 0) == -1) {
            perror("[Seller] Failed to send response to customer");
        }
        sleep(1);
    }

    return 0;
}
