#ifndef GUI_H
#define GUI_H

#include <GL/glut.h>
#define SIMULATION_SCREEN 0
#define ROUND_END_SCREEN 1
#define GAME_END_SCREEN 2
#define GET_READY_SCREEN 3
#define MAX_PLAYERS_PER_TEAM 4
#define MAX_TEAMS 2
extern int should_exit;

typedef struct {
    int team_id;
    int player_id;
    int energy;
    int max_energy;
    int is_fallen;
    float recovery_time;
} GuiPlayer;

typedef struct {
    GuiPlayer players[MAX_PLAYERS_PER_TEAM];
    int score;
    int total_effort;
} GuiTeam;

typedef struct {
    GuiTeam teams[MAX_TEAMS];
    float rope_position;
    int current_round;
    float time_remaining;
    char round_result[100];
    char game_result[200];
    int game_state; // 0=Simulating, 1=RoundEnd, 2=GameEnd, 3=GetReady
} GuiMessage;

extern GuiTeam teams[2];
extern GuiMessage current_state;
extern float rope_position;
extern int game_state;
extern int current_round;
extern float time_remaining;
extern char round_result[100];
extern char game_result[200];

extern float rope_y_position;
extern float rope_transition_timer;

void drawPlayer(float x, float y, float r, float g, float b, float alpha, int player_id, int energy, int max_energy, int is_fallen, float recovery_time, int team_id);
void drawRope(float rope_position);
void drawReferee(float x, float y, float wave_angle, const char* message);
void drawScoreboard();
void drawNotification(const char* message);
void drawRoundResult();
void drawGameResult();
void drawGround();
void drawRefereePlatform(float x, float y);
void drawTree(float x, float y, float height_scale, float trunk_width, float foliage_color_offset);
void sort_players_by_effort(GuiTeam* team);
void* gui_fifo_listener(void* arg);


#endif