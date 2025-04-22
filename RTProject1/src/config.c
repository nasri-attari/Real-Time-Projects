#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

int load_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("[Config] Cannot open config file");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char key[64];
        int value;
        if (sscanf(line, "%[^=]=%d", key, &value) != 2) continue;

        if (strcmp(key, "max_time") == 0)
            config->max_time = value;
        else if (strcmp(key, "round_interval") == 0)
            config->round_interval = value;
        else if (strcmp(key, "effort_threshold") == 0)
            config->effort_threshold = value;
        else if (strcmp(key, "consecutive_win_limit") == 0)
            config->consecutive_win_limit = value;
        else if (strcmp(key, "max_score") == 0)
            config->max_score = value;
        else if (strcmp(key, "min_initial_energy") == 0)
            config->min_initial_energy = value;
        else if (strcmp(key, "max_initial_energy") == 0)
            config->max_initial_energy = value;
        else if (strcmp(key, "min_energy_decrease") == 0)
            config->min_energy_decrease = value;
        else if (strcmp(key, "max_energy_decrease") == 0)
            config->max_energy_decrease = value;
        else if (strcmp(key, "fall_chance_percent") == 0)
            config->fall_chance_percent = value;
        else if (strcmp(key, "min_fall_duration") == 0)
            config->min_fall_duration = value;
        else if (strcmp(key, "max_fall_duration") == 0)
            config->max_fall_duration = value;
        else if (strncmp(key, "position_multiplier_", 20) == 0) {
            int index = atoi(&key[20]);
            if (index >= 0 && index < 4) {
                config->position_multiplier[index] = value;
            }
        } else if (strcmp(key, "gui_enabled") == 0)
            config->gui_enabled = value;
        else if (strcmp(key, "max_round_duration") == 0)
            config->max_round_duration = value;
        else if (strcmp(key, "hold_threshold") == 0)
            config->hold_threshold = value;
        else if (strcmp(key, "threshold_decay_step") == 0)
            config->threshold_decay_step = value;
        else if (strcmp(key, "threshold_decay_interval") == 0)
            config->threshold_decay_interval = value;
        else if (strcmp(key, "refuel_win_min") == 0)
            config->refuel_win_min = value;
        else if (strcmp(key, "refuel_win_max") == 0)
            config->refuel_win_max = value;
        else if (strcmp(key, "refuel_loss_min") == 0)
            config->refuel_loss_min = value;
        else if (strcmp(key, "refuel_loss_max") == 0)
            config->refuel_loss_max = value;
        else if (strcmp(key, "refuel_tie_min") == 0)
            config->refuel_tie_min = value;
        else if (strcmp(key, "refuel_tie_max") == 0)
            config->refuel_tie_max = value;
        else if (strcmp(key, "max_energy") == 0)
            config->max_energy = value;
    }

    fclose(file);
    return 0;
}
