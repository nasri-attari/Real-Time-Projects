#ifndef GUI_H
#define GUI_H
#include <stdlib.h>
#include <time.h>
#include "config.h"
void init_gui(int argc, char *argv[]);
void display_gui();
void draw_bankers();
void render_text(float x, float y, const char *text);
void draw_stick_figure(float x, float y);
void draw_jewelry_and_gold();
void draw_sellers(float x, float y);
void draw_counter(float x, float y, float w, float h);
void draw_jewelry_items(float x, float y);
void draw_artwork_section();
void draw_art_pieces(float x, float y);
void draw_counter_number_per_person(float x, float y);
void draw_arm_trafficking();
void draw_arms_sellers(float x, float y);
void draw_weapon_labels(float x, float y);
void draw_drug_trafficking();
void draw_drug_dealers(float x, float y);
void draw_wealthy_people();
void draw_wealthy_figures(float x, float y);
void draw_police_station();
void draw_police_figures(float x, float y);
void draw_prison_section();
void draw_prison_bars(float x, float y, float width, float height);
void draw_prison_dividers(float x, float y, float width, float height);
void draw_status_panel();
void draw_gang_section(int gang_index, float x, float y);
Gang* get_gang_by_index(int index);
void draw_gang_grid_layout();
void draw_gang_box(int gang_index, float x, float y);
void update_gui();
void mock_shared_memory_data();
void draw_arrested_members_in_prison(float x, float y, float width, float height);
void draw_arrested_gangs_in_prison(int display_index, int gang_index);
void draw_police_gang_messages();
void update_gui(int value);
bool any_gang_has_target(int crime_type);
#endif
