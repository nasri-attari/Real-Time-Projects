#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE "config.txt"
#define FIFO_TRACK "/tmp/rope_pulling_track_fifo_team%d_player%d"
#define FIFO_GUI "/tmp/rope_pulling_gui_fifo"

// Message types
#define PLAYER_ENERGY 1
#define GUI_EFFORT_UPDATE 1
#define GUI_ROUND_RESULT 2
#define GUI_GAME_RESULT 3

typedef struct {
    int type;
    int team;
    int player;
    int energy;
    float fall_remaining;
} TrackMessage;

typedef struct {
    int max_time;
    int round_interval;
    int effort_threshold;
    int consecutive_win_limit;
    int max_score;

    int min_initial_energy;
    int max_initial_energy;

    int min_energy_decrease;
    int max_energy_decrease;

    int fall_chance_percent;
    int min_fall_duration;
    int max_fall_duration;

    int position_multiplier[4];
    int gui_enabled;
    int max_round_duration;
    int hold_threshold;
    int threshold_decay_step;
    int threshold_decay_interval;

    int refuel_win_min, refuel_win_max;
    int refuel_loss_min, refuel_loss_max;
    int refuel_tie_min, refuel_tie_max;
    int max_energy;
} Config;


int load_config(const char *filename, Config *config);

#endif