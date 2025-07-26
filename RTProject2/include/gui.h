#ifndef GUI_H
#define GUI_H

#include "config.h"

void init_gui(int argc, char *argv[]);
void display_gui();
void update_gui(int value);
void draw_ingredients_info();
void draw_status_box();
void draw_suppliers();
void draw_chef_team_by_role(float x, float y, const char* label, int role);
void draw_prepared_items_near_chefs();
void draw_bakers_header_bar();
void draw_kitchen_divider();
void draw_storage_bar();
void draw_manager();
void draw_sellers();
void draw_counter_with_items();
void draw_customer_queues();
void draw_baker_team_by_role(float x, float y, const char* label, int role);
void draw_baked_items_near_bakers();

#endif