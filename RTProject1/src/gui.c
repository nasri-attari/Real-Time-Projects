#include "gui.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"
#define teams current_state.teams
#define rope_position current_state.rope_position
#define game_state current_state.game_state
#define current_round current_state.current_round
#define time_remaining current_state.time_remaining
#define round_result current_state.round_result
#define game_result current_state.game_result

float rope_marker_pos = 0.0f;
float rope_y_position = -0.025f;
float rope_transition_timer = 0.0f;
int gui_fifo_fd = -1;
GuiMessage current_state;
int should_exit = 0;


void sort_players_by_effort(GuiTeam* team) {
    GuiPlayer temp;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (team->players[i].energy > team->players[j].energy) {
                temp = team->players[i];
                team->players[i] = team->players[j];
                team->players[j] = temp;
            }
        }
    }
}
void drawPlayer(float x, float y, float r, float g, float b, float alpha, int player_id, int energy, int max_energy, int is_fallen, float recovery_time, int team_id) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float effective_alpha = is_fallen ? alpha * 0.3f : alpha;

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(0.5f, 0.5f, 0.5f);

    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    glColor4f(r, g, b, effective_alpha);
    glutSolidSphere(0.03, 20, 20);

    glColor4f(0.0f, 0.0f, 0.0f, effective_alpha);

    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex2f(-0.01f, 0.01f);
    glVertex2f(0.01f, 0.01f);
    glEnd();
    glPointSize(1.0f);

    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 10; i++) {
        float angle = 3.14159f * i / 10.0f;
        float mx = 0.015f * cos(angle);
        float my = -0.015f * sin(angle) - 0.005f;
        glVertex2f(mx, my);
    }
    glEnd();
    glPopMatrix();

    glColor4f(0.0f, 0.0f, 0.0f, effective_alpha);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.12f);
    glVertex2f(0.0f, 0.0f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(-0.05f, 0.08f);
    glVertex2f(0.0f, 0.10f);
    glVertex2f(0.0f, 0.10f);
    glVertex2f(0.05f, 0.08f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(-0.05f, -0.05f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.05f, -0.05f);
    glEnd();
    glLineWidth(1.0f);

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glRasterPos2f(-0.04f, 0.30f);
    char team_player_str[6];
    sprintf(team_player_str, "T%dP%d", team_id + 1, player_id);
    for (char* c = team_player_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    }

    glRasterPos2f(-0.03f, 0.25f);
    char energy_str[10];
    sprintf(energy_str, "E: %d", energy);
    for (char* c = energy_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    }

    float energy_ratio = (float)energy / max_energy;
    float bar_width = 0.1f;
    float bar_height = 0.02f;
    glPushMatrix();
    glTranslatef(-bar_width / 2, -0.10f, 0.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(bar_width, 0.0f);
    glVertex2f(bar_width, bar_height);
    glVertex2f(0.0f, bar_height);
    glEnd();
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(bar_width * energy_ratio, 0.0f);
    glVertex2f(bar_width * energy_ratio, bar_height);
    glVertex2f(0.0f, bar_height);
    glEnd();
    glPopMatrix();

    if (is_fallen && recovery_time > 0) {
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glRasterPos2f(-0.03f, 0.35f);
        char timer_str[10];
        sprintf(timer_str, "%.1f", recovery_time);
        for (char* c = timer_str; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }
    }

    glPopMatrix();
    glDisable(GL_BLEND);
}

void drawRope(float ropePosition) {
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(-0.825f - ropePosition, rope_y_position);
    glVertex2f(0.825f - ropePosition, rope_y_position);
    glEnd();
    glLineWidth(1.0f);


    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-ropePosition, rope_y_position, 0.0f);
    glutSolidSphere(0.01f, 20, 20);
    glPopMatrix();

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, rope_y_position - 0.05f);
    glVertex2f(0.0f, rope_y_position + 0.05f);
    glEnd();

    glRasterPos2f(-0.04f, rope_y_position + 0.1f);
    const char* message = "Center";
    for (const char* c = message; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

void drawReferee(float x, float y, float wave_angle, const char* message) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(0.7f, 0.7f, 0.7f);

    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.03, 20, 20);

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex2f(-0.01f, 0.01f);
    glVertex2f(0.01f, 0.01f);
    glEnd();
    glPointSize(1.0f);

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 10; i++) {
        float angle = 3.14159f * i / 10.0f;
        float mx = 0.015f * cos(angle);
        float my = -0.015f * sin(angle) - 0.005f;
        glVertex2f(mx, my);
    }
    glEnd();
    glPopMatrix();

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.12f);
    glVertex2f(0.0f, 0.0f);
    glEnd();


    glBegin(GL_LINES);
    glVertex2f(-0.05f, 0.08f);
    glVertex2f(0.0f, 0.10f);
    glVertex2f(0.0f, 0.10f);
    glVertex2f(0.05f, 0.08f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(-0.05f, -0.05f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.05f, -0.05f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.05f, 0.08f, 0.0f);
    glRotatef(wave_angle, 0.0f, 0.0f, 1.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.12f);
    glEnd();
    glLineWidth(1.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.12f);
    glVertex2f(0.06f, 0.12f);
    glVertex2f(0.06f, 0.06f);
    glVertex2f(0.0f, 0.06f);
    glEnd();
    glPopMatrix();

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glRasterPos2f(-0.06f, 0.30f);
    char label[] = "Referee";
    for (char* c = label; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }

    if (strlen(message) > 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(-0.05f, 0.45f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(-0.1f, -0.05f);
        glVertex2f(0.15f, -0.05f);
        glVertex2f(0.15f, 0.1f);
        glVertex2f(-0.1f, 0.1f);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.05f, -0.05f);
        glVertex2f(-0.02f, -0.12f);
        glVertex2f(0.0f, -0.05f);
        glEnd();
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        glRasterPos2f(-0.09f, 0.0f);
        for (const char* c = message; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        glPopMatrix();
    }

    glPopMatrix();
}
void drawScoreboard() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(-0.3f, 0.9f);
    char score_str[100];
    sprintf(score_str, "Team 1 Score: %d | Team 2 Score: %d | Round: %d | Time Remaining: %.0f",
            teams[0].score, teams[1].score, current_round, time_remaining);
    for (char* c = score_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawGround() {
    glColor3f(0.2f, 0.8f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -0.025f);
    glVertex2f(1.0f, -0.025f);
    glVertex2f(1.0f, -0.075f);
    glVertex2f(-1.0f, -0.075f);
    glEnd();
}

void drawRefereePlatform(float x, float y) {
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(x - 0.1f, y);
    glVertex2f(x + 0.1f, y);
    glVertex2f(x + 0.1f, y - 0.02f);
    glVertex2f(x - 0.1f, y - 0.02f);
    glEnd();
}

void drawTree(float x, float y, float height_scale, float trunk_width, float foliage_color_offset) {
    glColor3f(0.5f, 0.3f, 0.0f);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(0.5f, 0.5f * height_scale, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.04f * trunk_width, 0.0f);
    glVertex2f(0.04f * trunk_width, 0.0f);
    glVertex2f(0.03f * trunk_width, 0.2f);
    glVertex2f(-0.03f * trunk_width, 0.2f);
    glEnd();
    glPopMatrix();

    glColor3f(0.0f, 0.6f + foliage_color_offset, 0.1f);
    glPushMatrix();
    glTranslatef(x, y + 0.25f * height_scale * 0.5f, 0.0f);
    glutSolidCone(0.12f * 0.5f, 0.2f * 0.5f * height_scale, 20, 20);
    glPopMatrix();

    glColor3f(0.0f, 0.8f + foliage_color_offset, 0.2f);
    glPushMatrix();
    glTranslatef(x, y + 0.4f * height_scale * 0.5f, 0.0f);
    glutSolidCone(0.09f * 0.5f, 0.15f * 0.5f * height_scale, 20, 20);
    glPopMatrix();

    glColor3f(0.0f, 0.9f + foliage_color_offset, 0.3f);
    glPushMatrix();
    glTranslatef(x, y + 0.5f * height_scale * 0.5f, 0.0f);
    glutSolidCone(0.06f * 0.5f, 0.1f * 0.5f * height_scale, 20, 20);
    glPopMatrix();
}


void drawNotification(const char* message) {
    if (strlen(message) == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, -0.75f, 0.0f);

    float quad_width = 0.4f;
    float quad_height = 0.1f;
    glBegin(GL_QUADS);
    glVertex2f(-quad_width, -quad_height);
    glVertex2f(quad_width, -quad_height);
    glVertex2f(quad_width, quad_height);
    glVertex2f(-quad_width, quad_height);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(-0.19f, -0.02f);

    int text_width = 0;
    const char* c;
    for (c = message; *c != '\0'; c++) {
        text_width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c);
    }
    glTranslatef(-text_width/2200.0f, 0.0f, 0.0f);

    for (c = message; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    glPopMatrix();
    glDisable(GL_BLEND);
}

void drawRoundResult() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f, 0.5f, 0.8f, 0.9f);

    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.75f);
    glVertex2f(0.5f, -0.75f);
    glVertex2f(0.5f, -0.55f);
    glVertex2f(-0.5f, -0.55f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    glRasterPos2f(-0.4f, -0.60f);
    for (const char* c = round_result; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    char effort_str[50];
    sprintf(effort_str, "Team 1 Effort: %d | Team 2 Effort: %d", teams[0].total_effort, teams[1].total_effort);
    glRasterPos2f(-0.4f, -0.70f);
    for (const char* c = effort_str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    if (game_state == ROUND_END_SCREEN) {
    rope_position = 0.0f;  // Reset the rope to center during result display
}
    glDisable(GL_BLEND);
}

void drawGameResult() {
    char winner[20] = "Tie";
    char reason[50] = "unknown";
    char* winner_start = strstr(game_result, "Final Winner: Team ");
    char* reason_start = strstr(game_result, "Reason: ");

    if (winner_start) {
        sscanf(winner_start, "Final Winner: Team %19s", winner);
        winner[strcspn(winner, "!")] = '\0';

    }

    if (reason_start) {
        sscanf(reason_start, "Reason: %49[^\n]", reason);

    }

    glColor4f(0.47f, 0.87f, 0.47f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.25f);
    glVertex2f(0.5f, -0.25f);
    glVertex2f(0.5f, 0.25f);
    glVertex2f(-0.5f, 0.25f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    glRasterPos2f(-0.15f, 0.20f);
    const char* line1 = "Game Over!";
    for (const char* c = line1; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    char line2[50];
    snprintf(line2, sizeof(line2), "Final Winner: %s!", winner);
    glRasterPos2f(-0.15f, 0.10f);
    for (const char* c = line2; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    char line3[50];
    snprintf(line3, sizeof(line3), "Team 1 Score: %d | Team 2 Score: %d", teams[0].score, teams[1].score);
    glRasterPos2f(-0.15f, 0.0f);
    for (const char* c = line3; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    char line4[30];
    snprintf(line4, sizeof(line4), "Rounds Played: %d", current_round);
    glRasterPos2f(-0.15f, -0.10f);
    for (const char* c = line4; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    char line5[50];
    snprintf(line5, sizeof(line5), "Reason: %s", reason);
    glRasterPos2f(-0.15f, -0.20f);
    for (const char* c = line5; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void* gui_fifo_listener(void* arg) {
    int fd = open(FIFO_GUI, O_RDONLY | O_NONBLOCK);
    usleep(100000);
    if (fd == -1) {
        perror("[GUI] Failed to open FIFO_GUI for reading");
        return NULL;
    }

    while (1) {
        GuiMessage msg;
        ssize_t n = read(fd, &msg, sizeof(GuiMessage));
        if (n == sizeof(GuiMessage)) {
            current_state = msg;
            glutPostRedisplay();

            if (game_state == GAME_END_SCREEN) {
                sleep(5);
                //printf("[GUI] Game over. Exiting OpenGL window.\n");
                should_exit = 1;
                break;
            }
        } else {
            usleep(100000);
        }
    }

    close(fd);
    return NULL;
}