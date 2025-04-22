#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include "config.h"

int team, player;
int fd;
int energy = 100;
int decay_rate = 5;
int position_weight = 1;
int falling_duration = 0;
int tick_count = 0;
int can_send_effort = 0;

Config config;

void send_effort(int value) {
    TrackMessage msg = { PLAYER_ENERGY, team, player, value, (float)falling_duration };
    write(fd, &msg, sizeof(msg));
}

void handle_sigusr1(int sig) {
    can_send_effort = 1;
    printf("[Player %d-%d] Received start signal. Now sending efforts.\n", team, player);
}

void handle_sigalrm(int sig) {
    if (!can_send_effort) {
        alarm(1);
        return;
    }

    if (falling_duration > 0) {
        falling_duration--;
        printf("[Player %d-%d] Fell down! Remaining: %d sec.\n", team, player, falling_duration);
        send_effort(0);
    } else {
        int decay;
        if (tick_count % 2 == 0) {
            decay = decay_rate * (position_weight / 2);
            if (decay > energy / 2) decay = energy / 2;
            energy -= decay;
            if (energy < 0) energy = 0;
        }

        //energy += rand() % 3;
        if (energy > config.max_energy) energy = config.max_energy;

        if ((rand() % 100) < config.fall_chance_percent) {
            falling_duration = rand() % (config.max_fall_duration - config.min_fall_duration + 1) + config.min_fall_duration;
            printf("[Player %d-%d] Fell down! Out for %d sec.\n", team, player, falling_duration);
            send_effort(0);
        } else {
            printf("[Player %d-%d] Effort: energy=%d, decay=%d (rate=%d, weight=%d)\n",
                   team, player, energy, decay, decay_rate, position_weight);
            send_effort(energy);
        }
    }
    tick_count++;
    alarm(1);
}

void handle_sigusr2(int sig, siginfo_t *info, void *ctx) {
    int val = info->si_value.sival_int;

    if (val >= 1 && val <= 4) {
        // Position assignment
        position_weight = val;
        printf("[Player %d-%d] Assigned position weight: %d\n", team, player, val);
    } else {
        // Refuel
        int added = (val * config.max_energy) / 100;
        energy += added;
        if (energy > config.max_energy) energy = config.max_energy;

        if (val >= config.refuel_loss_min)
            printf("[Player %d-%d] Recovery (Loss): +%d%% ➜ energy = %d\n", team, player, val, energy);
        else if (val >= config.refuel_win_min)
            printf("[Player %d-%d] Recovery (Win): +%d%% ➜ energy = %d\n", team, player, val, energy);
        else
            printf("[Player %d-%d] Light Refuel: +%d%% ➜ energy = %d\n", team, player, val, energy);
    }

    fflush(stdout);
}

void handle_sigterm(int sig) {
    printf("[Player %d-%d] Terminating...\n", team, player);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    team = atoi(argv[1]);
    player = atoi(argv[2]);
    srand(getpid());

    if (load_config(CONFIG_FILE, &config) != 0) return 1;

    // Initial energy and decay rate setup
    energy = rand() % (config.max_initial_energy - config.min_initial_energy + 1) + config.min_initial_energy;
    decay_rate = rand() % (config.max_energy_decrease - config.min_energy_decrease + 1) + config.min_energy_decrease;

    // Open FIFO
    char fifo_path[64];
    snprintf(fifo_path, sizeof(fifo_path), FIFO_TRACK, team, player);

    int attempts = 0;
    while ((fd = open(fifo_path, O_WRONLY)) == -1 && attempts++ < 100) usleep(100000);

    printf("[Player %d-%d] Ready. Initial energy = %d, decay = %d\n",
           team, player, energy, decay_rate);

    struct sigaction sa1;
    sa1.sa_handler = handle_sigusr1;
    sa1.sa_flags = 0;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    sa2.sa_sigaction = handle_sigusr2;
    sa2.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &sa2, NULL);

    struct sigaction sa3 = { .sa_handler = handle_sigterm };
    sigaction(SIGTERM, &sa3, NULL);

    struct sigaction sa;
    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    alarm(1);

    while (1) pause();
}
