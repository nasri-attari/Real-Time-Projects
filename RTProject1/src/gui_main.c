#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <GL/glut.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include "config.h"
#include "gui.h"
#include <signal.h>

Config game_config;
#define game_state current_state.game_state
#define current_round current_state.current_round
#define time_remaining current_state.time_remaining
#define game_result current_state.game_result
#define round_result current_state.round_result
#define teams current_state.teams
#define rope_position current_state.rope_position


int fifo_fd;
char notification[100] = "";
float notification_timer = 0.0f;
int consecutive_wins[2] = {0, 0};
float round_end_timer = 0.0f;
float pull_message_timer = 0.0f;
int has_initial_transition = 0;
int fd;

void handle_alarm(int sig) {
    should_exit = 2;
}

void timer() {
    static float last_time = 0.0f;
    float current_time = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float delta_time = current_time - last_time;
    last_time = current_time;

    if (should_exit == 1) {
        glutPostRedisplay();
        return;
    }else if (should_exit == 2) {
      exit(0);
    }

    GuiMessage msg;
    ssize_t n = read(fd, &msg, sizeof(GuiMessage));
    if (n == sizeof(GuiMessage)) {
        current_state = msg;

        for (int t = 0; t < 2; t++){
          for (int p = 0; p < MAX_PLAYERS_PER_TEAM; p++){
            //printf("\n\n=======->team = %d, player = %d energy =  %d\n\n",t,p, teams[t].players[p].energy);
          }
          }
        glutPostRedisplay();
        if (game_state == GAME_END_SCREEN) {
            signal(SIGALRM, handle_alarm);
            alarm(5);
            should_exit=1;
            //printf("[GUI] Game over. Exiting OpenGL window.\n");
            return;
        }
    }
    if (game_state == 3) {
        if (!has_initial_transition && rope_transition_timer > 0) {
            rope_transition_timer -= delta_time;

            if (rope_transition_timer <= 0) {
                rope_transition_timer = 0.0f;
                rope_y_position = 0.03f;
                has_initial_transition = 1;
            } else {
                float t = 1.0f - (rope_transition_timer / 5.0f);
                rope_y_position = -0.025f + t * (0.03f - (-0.025f));
            }
        }
    }

    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (game_state == 2) {
        drawGameResult();
    } else {
        drawTree(-0.9f, -0.025f, 1.0f, 1.0f, 0.0f);
        drawTree(-0.8f, -0.025f, 1.2f, 0.8f, 0.1f);
        drawTree(-0.7f, -0.025f, 0.9f, 1.2f, -0.1f);
        drawTree(0.7f, -0.025f, 1.1f, 0.9f, 0.05f);
        drawTree(0.8f, -0.025f, 0.8f, 1.1f, -0.05f);
        drawTree(0.9f, -0.025f, 1.0f, 1.0f, 0.0f);

        drawGround();

        drawRope(rope_position);

        for (int t = 0; t < 2; t++) {
            sort_players_by_effort(&teams[t]);

            float r = (t == 0) ? 0.0f : 1.0f;
            float g = 0.0f;
            float b = (t == 0) ? 1.0f : 0.0f;
            float base_x = (t == 0) ? -0.4f : 0.4f;

            for (int i = 0; i < 4; i++) {
                float x_offset = (1.5f - i) * 0.1f;
                if (t == 1)
                    x_offset = (i - 1.5f) * 0.1f;

                GuiPlayer* p = &teams[t].players[i];
                drawPlayer(base_x + x_offset, 0.0f, r, g, b, 1.0f,
                           p->player_id, p->energy, p->max_energy,
                           p->is_fallen, p->recovery_time, p->team_id);
            }
        }

        drawRefereePlatform(-0.1f, 0.25f);

        float wave_angle = 10 * sin(glutGet(GLUT_ELAPSED_TIME) / 100.0);
        const char* referee_message = (game_state == 0) ? "Pull!" : (game_state == 3) ? "Get Ready!" : "";
        drawReferee(-0.1f, 0.3f, wave_angle, referee_message);

        drawScoreboard();

        drawNotification(notification);

        if (game_state == 1) {
            drawRoundResult();
        }
    }

    glutSwapBuffers();
}

void init() {
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
    srand(time(NULL));

    rope_y_position = -0.025f;
    rope_transition_timer = 1.0f;
    has_initial_transition = 0;
}

int main(int argc, char** argv) {
    fd = open(FIFO_GUI, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("[GUI] Failed to open FIFO_GUI for reading");
        return;
    }
    if (load_config(CONFIG_FILE, &game_config) != 0) {
        fprintf(stderr, "[Main] Failed to load configuration file.\n");
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Rope Pulling Game Simulation");


    time_remaining = game_config.max_time;

    init();
    game_state = 3;
    round_end_timer = 5.0f;
    glutDisplayFunc(display);
    glutIdleFunc(timer);
    glutMainLoop();

    return EXIT_SUCCESS;
}