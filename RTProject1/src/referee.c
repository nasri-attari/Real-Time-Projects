#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "config.h"
#include "gui.h"
#define NUM_TEAMS 2
#define PLAYERS_PER_TEAM 4

int gui_fd = -1;
int player_fifos[NUM_TEAMS][PLAYERS_PER_TEAM];
pid_t player_pids[NUM_TEAMS][PLAYERS_PER_TEAM];
int efforts[NUM_TEAMS][PLAYERS_PER_TEAM];
int sorted_indices[NUM_TEAMS][PLAYERS_PER_TEAM];
int scores[NUM_TEAMS] = {0};
int consecutive_wins[NUM_TEAMS] = {0};
float fall_remaining[NUM_TEAMS][PLAYERS_PER_TEAM];
float rope_position = 0.0f;
int current_round = 1;
char round_result[100] = "";
char game_result[200] = "";
static float prev_scaled_diff = 0.0f;
int round_state = 0;

Config config;
time_t game_start_time;
GuiMessage final_msg;

void sort_players_by_efforts(int team, int sorted[PLAYERS_PER_TEAM]) {
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) sorted[i] = i;
    for (int i = 0; i < PLAYERS_PER_TEAM - 1; i++) {
        for (int j = 0; j < PLAYERS_PER_TEAM - i - 1; j++) {
            if (efforts[team][sorted[j]] > efforts[team][sorted[j + 1]]) {
                int temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
}

void cleanup_players() {
    for (int team = 0; team < NUM_TEAMS; team++)
        for (int player = 0; player < PLAYERS_PER_TEAM; player++)
            kill(player_pids[team][player], SIGTERM);
    sleep(1);
    printf("[Referee] All players terminated. Game over.\n");
}

void send_gui_message(int gui_fd) {
    if (gui_fd == -1) return;

    GuiMessage msg;
    for (int t = 0; t < NUM_TEAMS; t++) {
        msg.teams[t].score = scores[t];
        int total_effort = 0;
        for (int p = 0; p < PLAYERS_PER_TEAM; p++) {
            int weight = (p < 2) ? (p + 1) : (5 - p);
            total_effort += efforts[t][p] * weight;
            msg.teams[t].players[p].team_id = t;
            msg.teams[t].players[p].player_id = p;
            msg.teams[t].players[p].energy = efforts[t][p];
            msg.teams[t].players[p].max_energy = config.max_energy;
            msg.teams[t].players[p].is_fallen = (fall_remaining[t][p] > 0);
            msg.teams[t].players[p].recovery_time = fall_remaining[t][p];
        }
        msg.teams[t].total_effort = total_effort;
    }
    msg.rope_position = rope_position;
    msg.current_round = current_round;
    msg.time_remaining = config.max_time - difftime(time(NULL), game_start_time);
    strncpy(msg.round_result, round_result, sizeof(msg.round_result));

    if (consecutive_wins[0] >= config.consecutive_win_limit || consecutive_wins[1] >= config.consecutive_win_limit) {
        msg.game_state = GAME_END_SCREEN;
        snprintf(game_result, sizeof(game_result),
                 "Game Over!\n"
                 "Final Winner: Team %s!\n"
                 "Team 1 Score: %d | Team 2 Score: %d\n"
                 "Rounds Played: %d\n"
                 "Reason: Consecutive Win Limit Exceeded!\n",
                 (scores[0] > scores[1]) ? "1" :
                 (scores[1] > scores[0]) ? "2" : "None (Tie)",
                 scores[0], scores[1], current_round - 1);
    }
    else if (scores[0] >= config.max_score || scores[1] >= config.max_score) {
        msg.game_state = GAME_END_SCREEN;
        snprintf(game_result, sizeof(game_result),
                 "Game Over!\n"
                 "Final Winner: Team %s!\n"
                 "Team 1 Score: %d | Team 2 Score: %d\n"
                 "Rounds Played: %d\n"
                 "Reason: Max Score Reached!\n",
                 (scores[0] > scores[1]) ? "1" :
                 (scores[1] > scores[0]) ? "2" : "None (Tie)",
                 scores[0], scores[1], current_round - 1);
    }else if (strcmp(round_result, "") == 0) {
        msg.game_state = (round_state == 0) ? GET_READY_SCREEN : SIMULATION_SCREEN;
        }else {
        msg.game_state = SIMULATION_SCREEN;
    }
    strncpy(msg.game_result, game_result, sizeof(msg.game_result));
    final_msg = msg;
    write(gui_fd, &msg, sizeof(GuiMessage));
}

int main() {
    if (load_config(CONFIG_FILE, &config) != 0) {
        fprintf(stderr, "[Referee] Failed to load config.\n");
        return 1;
    }

    if (config.gui_enabled) {
        gui_fd = open(FIFO_GUI, O_WRONLY);
        if (gui_fd == -1) {
            perror("[Referee] Failed to open GUI FIFO");
        }

    }
    game_start_time = time(NULL);

    char fifo_path[64];
    for (int team = 0; team < NUM_TEAMS; team++)
        for (int player = 0; player < PLAYERS_PER_TEAM; player++) {
            snprintf(fifo_path, sizeof(fifo_path), FIFO_TRACK, team, player);
            mkfifo(fifo_path, 0666);
        }

    for (int team = 0; team < NUM_TEAMS; team++)
        for (int player = 0; player < PLAYERS_PER_TEAM; player++) {
            pid_t pid = fork();
            if (pid == 0) {
                char t[10], p[10];
                sprintf(t, "%d", team);
                sprintf(p, "%d", player);
                execlp("./bin/player", "player", t, p, NULL);
                perror("execlp failed");
                exit(1);
            }
            player_pids[team][player] = pid;
        }

    for (int team = 0; team < NUM_TEAMS; team++){
        for (int player = 0; player < PLAYERS_PER_TEAM; player++) {
            snprintf(fifo_path, sizeof(fifo_path), FIFO_TRACK, team, player);
            player_fifos[team][player] = open(fifo_path, O_RDONLY | O_NONBLOCK);
        }
        usleep(100000);
        }


    printf("[Referee] Game started.\n");

    for (int round = 1;; round++) {
        current_round = round;
        round_state = 0;
        rope_position = 0.0f;
        prev_scaled_diff = 0.0f;

        printf("\n[Referee] === Round %d ===\n", round);

        strncpy(round_result, "", sizeof(round_result));
        send_gui_message(gui_fd);
        sleep(3);

        round_state = 1;
        for (int t = 0; t < NUM_TEAMS; t++)
            for (int p = 0; p < PLAYERS_PER_TEAM; p++)
                kill(player_pids[t][p], SIGUSR1);

        TrackMessage msg;
        int received = 0;
        time_t start = time(NULL);
        while (received < 8 && time(NULL) - start < 3) {
            for (int t = 0; t < NUM_TEAMS; t++)
                for (int p = 0; p < PLAYERS_PER_TEAM; p++) {
                    ssize_t n = read(player_fifos[t][p], &msg, sizeof(msg));
                    if (n > 0) {
                        efforts[msg.team][msg.player] = msg.energy;
                        fall_remaining[msg.team][msg.player] = msg.fall_remaining;
                        //printf("\n\n\n=======> team %d ,, player %d ,, energy %d\n\n\n", t,p, msg.energy);
                        received++;
                    }
                }
            usleep(100000);
        }

        for (int t = 0; t < NUM_TEAMS; t++)
            sort_players_by_efforts(t, sorted_indices[t]);

        for (int t = 0; t < NUM_TEAMS; t++)
            for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
                union sigval v;
                v.sival_int = i + 1;
                sigqueue(player_pids[t][sorted_indices[t][i]], SIGUSR2, v);
            }

        sleep(1);

        int threshold = config.effort_threshold;
        int round_winner = -1;
        int seconds_passed = 0;
        int hold_duration = 0;
        float effort_diff = 0;
        float scaled_diff = 0;
        while (1) {
            if (difftime(time(NULL), game_start_time) >= config.max_time) {
                printf("Game time exceeded (mid-round). Ending.\n");

                cleanup_players();

                const char* reason_str = "Time Exceeded";
                snprintf(game_result, sizeof(game_result),
                         "Game Over!\n"
                         "Final Winner: Team %s!\n"
                         "Team 1 Score: %d | Team 2 Score: %d\n"
                         "Rounds Played: %d\n"
                         "Reason: %s",
                         (scores[0] > scores[1]) ? "1" :
                         (scores[1] > scores[0]) ? "2" : "None (Tie)",
                         scores[0], scores[1], current_round - 1, reason_str);

                strncpy(round_result, "", sizeof(round_result));

                send_gui_message(gui_fd);
                final_msg.game_state = GAME_END_SCREEN;
                write(gui_fd, &final_msg, sizeof(GuiMessage));

                sleep(1);
                close(gui_fd);
                return 0;
            }
            int team_force[2] = {0};
            received = 0;

            // Read all available messages from FIFOs
            for (int t = 0; t < NUM_TEAMS; t++) {
                for (int p = 0; p < PLAYERS_PER_TEAM; p++) {
                    ssize_t n;
                    TrackMessage msg;
                    while ((n = read(player_fifos[t][p], &msg, sizeof(msg))) > 0) {
                        efforts[msg.team][msg.player] = msg.energy;
                        fall_remaining[msg.team][msg.player] = msg.fall_remaining;
                        team_force[msg.team] += msg.energy;
                        //printf("\n\n\n=======> team %d ,, player %d ,, energy %d\n\n\n", t,p, msg.energy);
                        received++;
                    }
                }
            }

            // Compute effort difference and scaled values
            effort_diff = (float)(team_force[0] - team_force[1]);
            scaled_diff = (effort_diff / 4800.0f) * 30.0f;

            // Update rope position
            if (scaled_diff != prev_scaled_diff) {
                rope_position = scaled_diff * 0.075f; // 0.6 / 8 = 0.075 per unit
                if (rope_position > 0.20f) rope_position = 0.20f;
                if (rope_position < -0.20f) rope_position = -0.20f;
                prev_scaled_diff = scaled_diff;
            }

            send_gui_message(gui_fd);

            printf("Round %d Status ➜ T0=%d | T1=%d | Δ=%.0f | Scaled Δ=%.1f | Rope Pos=%.2f\n",
                   current_round, team_force[0], team_force[1], effort_diff, scaled_diff, rope_position);

            // Break if both teams are exhausted
            if (team_force[0] == 0 && team_force[1] == 0) break;

            // Check for round winner based on threshold hold duration
            if (effort_diff >= threshold) {
                hold_duration++;
                if (hold_duration >= 3) {
                    round_winner = 0;
                    break;
                }
            } else if (effort_diff <= -threshold) {
                hold_duration++;
                if (hold_duration >= 3) {
                    round_winner = 1;
                    break;
                }
            } else {
                hold_duration = 0;
            }

            // Wait for round interval
            sleep(config.round_interval);
            seconds_passed++;

            // Decay the threshold
            if (seconds_passed % config.threshold_decay_interval == 0 &&
                threshold > config.max_round_duration)
            {
                threshold -= config.threshold_decay_step;
            }

            // Stop the round if duration exceeded
            if (seconds_passed >= config.max_round_duration) break;
        }

        if (round_winner != -1) {
            snprintf(round_result, sizeof(round_result), "Round %d Winner: Team %d", current_round, round_winner + 1 );
            scores[round_winner]++;
            consecutive_wins[round_winner]++;
            consecutive_wins[1 - round_winner] = 0;
            printf("Team %d wins the round!\n", round_winner + 1);
            printf("Elapsed Game Time: %lds\n", time(NULL) - game_start_time);

            for (int t = 0; t < NUM_TEAMS; t++)
                for (int p = 0; p < PLAYERS_PER_TEAM; p++) {
                    union sigval v;
                    int bonus = (t == round_winner)
                                ? rand() % (config.refuel_win_max - config.refuel_win_min + 1) + config.refuel_win_min
                                : rand() % (config.refuel_loss_max - config.refuel_loss_min + 1) + config.refuel_loss_min;
                    v.sival_int = bonus;
                    sigqueue(player_pids[t][p], SIGUSR2, v);
                }
        } else {
            snprintf(round_result, sizeof(round_result), "Round %d: No Winner", current_round);
            consecutive_wins[0] = 0;
            consecutive_wins[1] = 0;
            printf("Round undecided, applying light refuel.\n");

            for (int t = 0; t < NUM_TEAMS; t++)
                for (int p = 0; p < PLAYERS_PER_TEAM; p++) {
                    union sigval v;
                    int bonus = rand() % (config.refuel_tie_max - config.refuel_tie_min + 1) + config.refuel_tie_min;
                    v.sival_int = bonus;
                    sigqueue(player_pids[t][p], SIGUSR2, v);
                }
        }

        // Send round result to GUI
        //send_gui_message(gui_fd);
        send_gui_message(gui_fd);
        final_msg.game_state = ROUND_END_SCREEN;
        write(gui_fd, &final_msg, sizeof(GuiMessage));
        sleep(3);


        printf("Score — T1: %d | T2: %d\n", scores[0], scores[1]);
        if (scores[0] >= config.max_score || scores[1] >= config.max_score ||
            consecutive_wins[0] >= config.consecutive_win_limit ||
            consecutive_wins[1] >= config.consecutive_win_limit)
            break;
    }

    printf("\n[Final Result] T0 = %d | T1 = %d\n", scores[0], scores[1]);
    if (scores[0] > scores[1])
        snprintf(game_result, sizeof(game_result), "Team 1 Wins!");
    else if (scores[1] > scores[0])
        snprintf(game_result, sizeof(game_result), "Team 2 Wins!");
    else
        snprintf(game_result, sizeof(game_result), "Match Draw!");

    if (scores[0] > scores[1]) printf("Team 1 wins!\n");
    else if (scores[1] > scores[0]) printf("Team 2 wins!\n");
    else printf("Match is a draw!\n");

    cleanup_players();
    //GuiMessage final_msg;
    sleep(2);
    close(gui_fd);
    return 0;
}