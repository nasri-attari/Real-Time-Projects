#include <GL/glut.h>
#include "gui.h"
#include <math.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <string.h>

#include "../include/config.h"

extern Config config;  // Ensure this is declared globally
int elapsed_seconds = 0;  // global or static variable
GangPoliceState* global_state = NULL;

void update_gui(int value) {
    glutPostRedisplay();                   // Mark the window for redisplay
    glutTimerFunc(1000 / 30, update_gui, 0); // Call again in ~33ms (30 FPS)
}

void init_gui(int argc, char *argv[]) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600); // Window size
    glutCreateWindow("Organized Crime Simulation GUI");

    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 600.0);
    glMatrixMode(GL_MODELVIEW);

// ⬇ REGISTER the display function BEFORE the main loop
    glutDisplayFunc(display_gui);

    glutTimerFunc(0, update_gui, 0);  // Kick off the timer-based redisplay

// ⬇ ENTER MAIN LOOP LAST
    glutMainLoop();

}

// === Render text at specific position
void render_text(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    }
}
// === Gui display
void display_gui() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    draw_bankers();
    draw_jewelry_and_gold();
    draw_artwork_section();
    draw_arm_trafficking();
    draw_drug_trafficking();
    draw_wealthy_people();
    draw_police_station();
    draw_police_gang_messages();
    draw_prison_section();
    draw_status_panel();
    draw_gang_grid_layout();
    draw_gang_box(0, 210.0f, 600.0f);
    draw_gang_box(1, 520.0f, 600.0f);
    draw_gang_box(2, 210.0f, 400.0f);
    draw_gang_box(3, 520.0f, 400.0f);
    draw_gang_box(4, 210.0f, 200.0f);
    draw_gang_box(5, 520.0f, 200.0f);
    glutSwapBuffers();
    glutTimerFunc(0, update_gui, 0);
}
// === Draw a stick figure at given x, y
void draw_stick_figure(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);

    float head_radius = 3.0f;  // reduced from 5
    float torso_height = 7.0f;  // reduced from 10
    float arm_length = 3.0f;    // reduced from 5
    float leg_length = 3.0f;    // reduced from 5

    // Head
    glBegin(GL_POLYGON);
    for (int i = 0; i < 20; ++i) {
        float theta = 2.0f * 3.14159f * i / 20;
        glVertex2f(head_radius * cosf(theta), torso_height + head_radius * sinf(theta));
    }
    glEnd();

    // Body
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, torso_height); glVertex2f(0.0f, -torso_height);            // Torso
    glVertex2f(0.0f, torso_height / 2); glVertex2f(-arm_length, 0.0f);          // Left arm
    glVertex2f(0.0f, torso_height / 2); glVertex2f(arm_length, 0.0f);           // Right arm
    glVertex2f(0.0f, -torso_height); glVertex2f(-leg_length, -torso_height * 2); // Left leg
    glVertex2f(0.0f, -torso_height); glVertex2f(leg_length, -torso_height * 2);  // Right leg
    glEnd();
    glLineWidth(1.0f);

    glPopMatrix();
}
// === Draw a counter at given x, y
void draw_counter(float x, float y, float width, float height) {
    glColor3f(0.2f, 0.2f, 0.2f); // Dark gray
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y - height);
    glVertex2f(x, y - height);
    glEnd();
}
//=== Draw the "Status " area with some status  ===
void draw_status_panel() {
    GangPoliceState* state = global_state;
    if (!state) return;

    float x = 5.0f;    // near the left edge
    float y = 580.0f;  // near the top edge

    glColor3f(0.0f, 0.0f, 0.0f);  // black text

    // Title
    render_text(x, y, "Status Panel");

    // Get live stats
    int total_thwarted = state->thwarted;
    int total_successful = state->successful;
    int total_agents_uncovered = state->agents_exposed;
    int total_agents_active = state->agents_active;
    int total_failed = state->failed;

    int threshold_thwarted = config.max_thwarted;
    int threshold_successful = config.max_successful;
    int threshold_agents = config.max_agents_exposed;

    // Divider line
    glBegin(GL_LINES);
    glVertex2f(x - 5, 450);
    glVertex2f(x + 78, 450);
    glEnd();

    char buffer[64];

    snprintf(buffer, sizeof(buffer), "Thwarted Plans: %d / %d", total_thwarted, threshold_thwarted);
    render_text(x, y - 20, buffer);

    snprintf(buffer, sizeof(buffer), "Successful Crimes: %d / %d", total_successful, threshold_successful);
    render_text(x, y - 40, buffer);

    snprintf(buffer, sizeof(buffer), "Agents Uncovered: %d / %d", total_agents_uncovered, threshold_agents);
    render_text(x, y - 60, buffer);

    snprintf(buffer, sizeof(buffer), "Active Agents: %d", total_agents_active);
    render_text(x, y - 80, buffer);

    snprintf(buffer, sizeof(buffer), "Failed Ops: %d", total_failed);
    render_text(x, y - 100, buffer);
}

bool any_gang_has_target(int crime_type) {
    if (!global_state) return false;
    for (int i = 0; i < config.gang_count; i++) {
        if (global_state->gangs[i].current_target == crime_type) {
            return true;
        }
    }
    return false;
}

// === Draw the "Bank and Financial Institutions" area with bankers ===
void draw_bankers() {
    float x = 0.0f;
    float y = 370.0f;
    float width = 83.0f;
    float height = 80.0f;


// Right vertical divider
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(x + width, y+20);
    glVertex2f(x + width, y - height);
    glEnd();

    // Title
    glColor3f(0.0f, 0.0f, 0.0f);
    render_text(5, 370, "Bank &");
    render_text(5, 360, "financial institutions (2)");
    draw_counter(10,340,70,20);
    draw_counter_number_per_person(30.0f, 330.0f);
        float px = x + 30.0f;
        float py = y - 30.0f;
        // If targeted, draw red
        if (any_gang_has_target(ROB_BANKS)) {
            glColor3f(1.0f, 0.0f, 0.0f);  // Red
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);  // Black
        }
        draw_stick_figure(px, py);
    glColor3f(0.0f, 0.0f, 0.0f);
}
void draw_counter_number_per_person(float x, float y) {
    render_text(x,        y, "Counter");
}
// === Draw the "Jewelry And Gold" area with Sellers ===
void draw_jewelry_and_gold() {
    float x = 0.0f;
    float y = 320.0f;  // Below the bankers section
    float width = 83.0f;
    float height = 70.0f;


    // Section border (custom)
    glBegin(GL_LINES);
    glVertex2f(x, y); glVertex2f(x + width, y);               // Top
    glVertex2f(x + width, y); glVertex2f(x + width, y - height);  // Right
    glEnd();

    // Label
    glColor3f(0.0f, 0.0f, 0.0f);
    render_text(x + 8, y-20 , "Jewelry & Gold (3)");


    // Sellers
    draw_sellers(x + 20, y - 40);

    // Counter
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_counter(x + 10, y - 40,70, 20);

    // Items
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_jewelry_items(x + 15, y - 50);
}
void draw_sellers(float x, float y) {
    // If targeted, draw red
    if (any_gang_has_target(ROB_JEWELRY_SHOPS)) {
        glColor3f(1.0f, 0.0f, 0.0f);  // Red
    } else {
        glColor3f(0.0f, 0.0f, 0.0f);  // Black
    }
        draw_stick_figure(x+20 , y);
}
void draw_jewelry_items(float x, float y) {
    render_text(x,        y, "Gold");
    render_text(x + 35.0f, y, "jewelry");
}
// === Draw the "Art works" area with some Art works ===
void draw_artwork_section() {
    float x = 0.0f;
    float y = 250.0f;  // Below Jewelry & Gold
    float width = 83.0f;
    float height = 60.0f;

    // Section border
    glColor3f(0.0f, 0.0f, 0.0f); // black
    glBegin(GL_LINES);
    glVertex2f(x, y); glVertex2f(x + width, y);               // Top
    glVertex2f(x, y - height); glVertex2f(x + width, y - height); // Bottom
    glVertex2f(x + width, y); glVertex2f(x + width, y - height);  // Right
    glEnd();

    // Title
    render_text(x + 10, y - 20, "Art Works (4)");

    // Render art pieces
    draw_art_pieces(x + 10, y - 30);
}
void draw_art_pieces(float x, float y) {
    float art_width = 15.0f;
    float art_height = 20.0f;
    float spacing_x = 40.0f;

        for (int col = 0; col < 2; col++) {
            float px = x + col * spacing_x;
            float py = y;

            // If targeted, draw red
            if (any_gang_has_target(ROB_ART_WORK)) {
                glColor3f(1.0f, 0.0f, 0.0f);  // Red
            } else {
                glColor3f(0.0f, 0.0f, 0.0f);  // Black
            }            glBegin(GL_LINE_LOOP);
            glVertex2f(px, py);
            glVertex2f(px + art_width, py);
            glVertex2f(px + art_width, py - art_height);
            glVertex2f(px, py - art_height);
            glEnd();

    }
}
// === Draw the "Arm Trafficking" area with some Arms ===
void draw_arm_trafficking() {
    float x = 0.0f;
    float y =180.0f;  // Below Art Works
    float width = 83.0f;
    float height = 95.0f;

    // Section border
    glColor3f(0.0f, 0.0f, 0.0f); // black
    glBegin(GL_LINES);
    glVertex2f(x, y - height); glVertex2f(x + width, y - height); // Bottom
    glVertex2f(x + width, y); glVertex2f(x + width, y - height);  // Right
    glEnd();

    // Title
    render_text(x + 5.0f, y -20.0f, "Arm trafficking (5)");

    // Traffickers
    draw_arms_sellers(x + 20.0f, y - 40.0f);

    // Counter
    draw_counter(x + 10.0f, y - 40.0f, +70.0f, 20.0f);

    // Weapon names (as text)
    draw_weapon_labels(x + 20.0f, y - 55.0f);
}
void draw_arms_sellers(float x, float y) {
// If targeted, draw red
    if (any_gang_has_target(ARM_TRAFFICKING)) {
        glColor3f(1.0f, 0.0f, 0.0f);  // Red
    } else {
        glColor3f(0.0f, 0.0f, 0.0f);  // Black
    }
    draw_stick_figure(x+20, y);
    }
void draw_weapon_labels(float x, float y) {
    render_text(x, y, "AK47");
    render_text(x + 30, y, "M16");
}
// === Draw the "Drug Trafficking" area with some Drug dealers ===
void draw_drug_trafficking() {
    float x = 0.0f;
    float y = 90.0f;  // Enough spacing below Arm Trafficking
    float width = 83.0f;
    float height = 60.0f;

    // Section border
    glColor3f(0.0f, 0.0f, 0.0f); // black
    glBegin(GL_LINES);
    glVertex2f(x + width, y); glVertex2f(x + width, y - height*2);  // Right
    glEnd();


    // Title
    render_text(x + 5.0f, y - 20.0f, "Drug Trafficking (6)");

    // Stick figures
    draw_drug_dealers(x + 10.0f, y - 50.0f);
}
void draw_drug_dealers(float x, float y) {
    int spacing_x = 15.0f;
    for (int row = 0; row < 1; row++) {
        for (int col = 0; col <5; col++) {
            float px = x + col * spacing_x;
            float py = y - row ;
            // If targeted, draw red
            if (any_gang_has_target(DRUG_TRAFFICKING)) {
                glColor3f(1.0f, 0.0f, 0.0f);  // Red
            } else {
                glColor3f(0.0f, 0.0f, 0.0f);  // Black
            }
            draw_stick_figure(px, py);
        }
    }
}
// === Draw the "Wealthy people" area with some people ===
void draw_wealthy_people() {
    float x = 0.0f;     // Right side start
    float y = 460.0f;     // Aligned with Bankers
    float width = 83.0f;
    float height = 75.0f;

    // Section border (top, bottom, right)
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(x, y - height); glVertex2f(x + width, y - height); // Bottom
    glVertex2f(x + width, 600); glVertex2f(x + width,380);  // Right
    glEnd();

    // Title
    render_text(x + 5 , y - 20.0f, "Wealthy People (0)/(1)");

    // Figures
    draw_wealthy_figures(x +9 , y - 40.0f);
}
void draw_wealthy_figures(float x, float y) {
    float spacing = 15.0f;
    // If targeted, draw red
    if (any_gang_has_target(BLACKMAIL) || any_gang_has_target(KIDNAPPING)) {
        glColor3f(1.0f, 0.0f, 0.0f);  // Red
    } else {
        glColor3f(0.0f, 0.0f, 0.0f);  // Black
    }
    for (int i = 0; i <5; i++) {
        draw_stick_figure(x + i * spacing, y);
    }
}
// === Draw the "Police Station" area with some Policemen ===
void draw_police_station() {
    float x = 80.0f;
    float y = 600.0f;  // Below Wealthy People
    float width = 100.0f;
    float height = 500.0f;

    // Section border (top, bottom, right)
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(x + width, y); glVertex2f(x + width, y - height);  // Right
    glEnd();

    // Title
    render_text(x + 30.0f, y - 20.0f, "Police");

    // Policemen (2 rows × 5)
    draw_police_figures(x + 20.0f, y - 350.0f);
}
void draw_police_figures(float x, float y) {
    float spacing_x = 15.0f;
    float spacing_y = 30.0f;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 5; col++) {
            float px = x + col * spacing_x;
            float py = y - row * spacing_y;
            draw_stick_figure(px, py);
        }
    }
}
void draw_police_gang_messages() {
    GangPoliceState* state = global_state;
    if (!state) return;

    float start_x = 85.0f;  // inside police area
    float start_y = 550.0f; // top of police area
    float box_width = 95.0f;
    float box_height = 20.0f;
    float vertical_spacing = 50.0f;

    for (int i = 0; i < config.gang_count; i++) {
        float y = start_y - i * vertical_spacing;

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Gang %d:", i + 1);

        // Gang label
        glColor3f(0.0f, 0.0f, 0.0f);
        render_text(start_x, y, buffer);

        // Message box (empty rectangle)
        glBegin(GL_LINE_LOOP);
        glVertex2f(start_x, y - 10);
        glVertex2f(start_x + box_width, y - 10);
        glVertex2f(start_x + box_width, y - 10 - box_height);
        glVertex2f(start_x, y - 10 - box_height);
        glEnd();

        // Render the message inside the box
        const char* message = state->police.status_message[i];
        if (message && strlen(message) > 0) {
            glRasterPos2f(start_x + 5, y - 20 - box_height / 2 + 5);
            for (const char* c = message; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
            }
        }
    }
    glutTimerFunc(0, update_gui, 0);
}

// === Draw the "Prison" area with some cells ===
void draw_prison_section() {
    float x = 83.0f;
    float y = 190.0f;
    float width = 97.0f;
    float height = 200.0f;

    // Outer border
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y - height);
    glVertex2f(x, y - height);
    glEnd();

    // Section title
    render_text(x + 40.0f, y + 5.0f, "Prison");

    // Draw prison bars
    draw_prison_bars(x, y, width, height);
    draw_prison_dividers(x, y, width, height);

    // NEW: Sequentially render arrested gangs
    int display_index = 0;
    for (int i = 0; i < config.gang_count; i++) {
        Gang* gang = get_gang_by_index(i);
        if (!gang) continue;

        bool gang_arrested = false;
        for (int j = 0; j < config.members_per_gang; j++) {
            if (gang->members[j].arrested == 1) {
                gang_arrested = true;
                break;
            }
        }

        if (gang_arrested) {
            draw_arrested_gangs_in_prison(display_index, i);
            display_index++;
        }
    }
}
void draw_prison_dividers(float x, float y, float width, float height) {
    float row_height = height / 3.0f;
    float col_width = width / 2.0f;

    // Enable thicker lines
    glLineWidth(3.5f);
    glColor3f(0.0f, 0.0f, 0.0f); // Black

    glBegin(GL_LINES);

    // Thick middle vertical bar
    float mid_x = x + col_width;
    glVertex2f(mid_x, y);
    glVertex2f(mid_x, y - height);

    // Two thick horizontal bars
    float h1_y = y - row_height;
    float h2_y = y - 2 * row_height;

    glVertex2f(x, h1_y);
    glVertex2f(x + width, h1_y);

    glVertex2f(x, h2_y);
    glVertex2f(x + width, h2_y);

    glEnd();

    // Reset to normal line width
    glLineWidth(1.0f);
}
void draw_prison_bars(float x, float y, float width, float height) {
    float row_height = height / 3.0f;
    float col_width = width / 3.0f;

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);

    // Vertical dividers (grid)
    for (int i = 1; i < 3; i++) {
        glVertex2f(x + i * col_width, y);
        glVertex2f(x + i * col_width, y - height);
    }

    // === Inner bars per cell ===
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            float cell_x = x + col * col_width;
            float cell_y = y - row * row_height;

            // Draw 3 vertical bars inside the cell
            for (int b = 1; b <= 3; b++) {
                float bx = cell_x + (b * col_width) / 4.0f;
                glVertex2f(bx, cell_y);
                glVertex2f(bx, cell_y - row_height);
            }
        }
    }

    glEnd();
}

//=== Draw the "Gang" area with some Gangster and some status  ===
void draw_gang_grid_layout() {
    float x_start = 180.0f;  // Based on your image
    float y_start = 600.0f;
    float cell_width = 450.0f;
    float cell_height = 200.0f;
    float total_height = 3 * cell_height;

    glColor3f(0.0f, 0.0f, 0.0f);

    // Draw 2 horizontal dividers (3 rows total)
    for (int i = 1; i < 3; i++) {
        float y = y_start - i * cell_height;
        glBegin(GL_LINES);
        glVertex2f(x_start, y);
        glVertex2f(x_start + cell_width+400, y);
        glEnd();
    }

    // Draw 1 vertical divider (2 columns)
    float x_mid = x_start + cell_width / 1.45f;
    glBegin(GL_LINES);
    glVertex2f(x_mid, y_start);
    glVertex2f(x_mid, y_start - total_height);
    glEnd();
}
void draw_gang_box(int gang_index, float x, float y) {
    Gang* gang = get_gang_by_index(gang_index);
    if (!gang) return;

    char title[32];
    snprintf(title, sizeof(title), "Gang %d", gang_index + 1);

    // Check if any member is arrested
    bool gang_arrested = false;
    for (int i = 0; i < config.members_per_gang; i++) {
        if (gang->members[i].arrested == 1) {
            gang_arrested = true;
            break;
        }
    }

    if (gang_arrested) {
        char message[64];
        snprintf(message, sizeof(message), "Arrested, release in: %ds", gang->arrest_timer);

        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2f(x + 110, y - 15);
        for (const char* c = message; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(x + 70, y - 15);
        for (const char* c = title; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        // NEW: draw agents even if gang is arrested
        float start_x = x + 5;
        float start_y = y - 60;
        int per_row = (config.members_per_gang + 1) / 2;

        float spacing_x = 90.0f;
        float spacing_y = 75.0f;
        float bar_width_max = 30.0f;

        int grid_index = 0;

        for (int i = 0; i < config.members_per_gang; i++) {
            Member* m = &gang->members[i];
            if (!m->is_agent) continue;  // only show agents

            int row = (grid_index < per_row) ? 0 : 1;
            int col = (row == 0) ? grid_index : grid_index - per_row;

            float mx = start_x + col * spacing_x;
            float my = start_y - row * spacing_y;

            glColor3f(1.0f, 0.0f, 0.0f);  // Red for agent
            draw_stick_figure(mx, my);

            if (strlen(m->member_message) > 0) {
                float bubble_width = 100.0f;
                float bubble_height = 18.0f;
                float bubble_x = mx + 7 - bubble_width / 2;
                float bubble_y = my + 15.0f;

                glColor3f(1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2f(bubble_x, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
                glVertex2f(bubble_x, bubble_y + bubble_height);
                glEnd();

                glColor3f(0.0f, 0.0f, 0.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(bubble_x, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
                glVertex2f(bubble_x, bubble_y + bubble_height);
                glEnd();

                glRasterPos2f(bubble_x + 5.0f, bubble_y + 6.0f);
                for (const char* c = m->member_message; *c; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
                }
            }


            float bar_fill = (m->preparation / (float)config.max_preparation_level) * bar_width_max;

            glColor3f(0.2f, 0.8f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(mx - 15, my - 25);
            glVertex2f(mx - 15 + bar_fill, my - 25);
            glVertex2f(mx - 15 + bar_fill, my - 20);
            glVertex2f(mx - 15, my - 20);
            glEnd();

            glColor3f(0.0f, 0.0f, 0.0f);
            char member_label[32];
            snprintf(member_label, sizeof(member_label), "Member %d Rank %d", i + 1, m->rank);
            render_text(mx - 15, my - 40, member_label);

            grid_index++;
        }

        return;  // done after drawing agents
    }

    // Draw gang title
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x + 70, y - 15);
    for (const char* c = title; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Layout grid
    float start_x = x + 5;
    float start_y = y - 60;
    int per_row = (config.members_per_gang + 1) / 2;

    float spacing_x = 90.0f;
    float spacing_y = 75.0f;
    float bar_width_max = 30.0f;

    int grid_index = 0;

    // Draw the leader (directly from gang->leader)
    int leader_index = gang->leader_num;  // assumed zero-based
    if (leader_index >= 0 && leader_index < config.members_per_gang) {
        Member* m = &gang->members[leader_index];
        if (!m->arrested) {
            int row = 0;
            int col = 0;

            float mx = start_x + col * spacing_x;
            float my = start_y - row * spacing_y;

            glColor3f(0.0f, 0.0f, 1.0f);  // Blue for leader
            draw_stick_figure(mx, my);
            render_text(mx - 15, my + 18, "Leader");

            // Render member_message if present
            if (strlen(m->member_message) > 0) {
                float bubble_width = 90.0f;
                float bubble_height = 18.0f;
                float bubble_x = mx + 7 - bubble_width / 2;
                float bubble_y = my + 15.0f;

                glColor3f(1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2f(bubble_x, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
                glVertex2f(bubble_x, bubble_y + bubble_height);
                glEnd();

                glColor3f(0.0f, 0.0f, 0.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(bubble_x, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y);
                glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
                glVertex2f(bubble_x, bubble_y + bubble_height);
                glEnd();

                glRasterPos2f(bubble_x + 5.0f, bubble_y + 6.0f);
                for (const char* c = m->member_message; *c; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
                }
            }

            float bar_fill = (m->preparation / (float)config.max_preparation_level) * bar_width_max;

            glColor3f(0.2f, 0.8f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(mx - 15, my - 25);
            glVertex2f(mx - 15 + bar_fill, my - 25);
            glVertex2f(mx - 15 + bar_fill, my - 20);
            glVertex2f(mx - 15, my - 20);
            glEnd();

            glColor3f(0.0f, 0.0f, 0.0f);
            char member_label[32];
            snprintf(member_label, sizeof(member_label), "Member %d Rank %d", leader_index + 1, m->rank);
            render_text(mx - 15, my - 40, member_label);

            grid_index++;
        }
    }

    // Draw non-leader members
    for (int i = 0; i < config.members_per_gang; i++) {
        if (i == leader_index) continue;

        Member* m = &gang->members[i];
        if (m->arrested) continue;

        int row = (grid_index < per_row) ? 0 : 1;
        int col = (row == 0) ? grid_index : grid_index - per_row;

        float mx = start_x + col * spacing_x;
        float my = start_y - row * spacing_y;

        if (!m->is_alive) {
            glColor3f(0.6f, 0.6f, 0.6f);  // Gray for dead
            draw_stick_figure(mx, my);
            grid_index++;
            continue;
        } else if (m->is_agent) {
            glColor3f(1.0f, 0.0f, 0.0f);  // Red for agent
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);  // Black for regular
        }

        draw_stick_figure(mx, my);

        // Render member_message if present
        if (strlen(m->member_message) > 0) {
            float bubble_width = 80.0f;
            float bubble_height = 18.0f;
            float bubble_x = mx + 7 - bubble_width / 2;
            float bubble_y = my + 15.0f;

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
            glVertex2f(bubble_x, bubble_y + bubble_height);
            glEnd();

            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
            glVertex2f(bubble_x, bubble_y + bubble_height);
            glEnd();

            glRasterPos2f(bubble_x + 5.0f, bubble_y + 6.0f);
            for (const char* c = m->member_message; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
            }
        }


        float bar_fill = (m->preparation / (float)config.max_preparation_level) * bar_width_max;

        glColor3f(0.2f, 0.8f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2f(mx - 15, my - 25);
        glVertex2f(mx - 15 + bar_fill, my - 25);
        glVertex2f(mx - 15 + bar_fill, my - 20);
        glVertex2f(mx - 15, my - 20);
        glEnd();

        glColor3f(0.0f, 0.0f, 0.0f);
        char member_label[32];
        snprintf(member_label, sizeof(member_label), "Member %d Rank %d", i + 1, m->rank);
        render_text(mx - 15, my - 40, member_label);

        grid_index++;
    }

    // Gang Status Panel
    float sx = x + 240;
    float sy = y - 20;
    float spacing = 25.0f;
    char buffer[64];

    glColor3f(0.0f, 0.0f, 0.0f);

    render_text(sx, sy, "Status:");
    if (gang->current_target == -1)
        snprintf(buffer, sizeof(buffer), "Target: -");
    else
        snprintf(buffer, sizeof(buffer), "Target: %d", gang->current_target);
    render_text(sx, sy - spacing, buffer);

    if (gang->start_exec == 1) {
        snprintf(buffer, sizeof(buffer), "Exec Time: %d", gang->time_to_execute);
        render_text(sx, sy - 2 * spacing, buffer);
    } else if (gang->start_prep == 1) {
        snprintf(buffer, sizeof(buffer), "Prp Time: %d", gang->required_prep_time);
        render_text(sx, sy - 2 * spacing, buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "Prp Time: -");
        render_text(sx, sy - 2 * spacing, buffer);
    }

    snprintf(buffer, sizeof(buffer), "All Prep: %d", gang->all_required_prep);
    render_text(sx, sy - 3 * spacing, buffer);

    snprintf(buffer, sizeof(buffer), "Cur Prep: %d", gang->current_prep);
    render_text(sx, sy - 4 * spacing, buffer);

    snprintf(buffer, sizeof(buffer), "RTRP: %.1f", gang->rtrp_ratio);
    render_text(sx, sy - 5 * spacing, buffer);
}
void draw_arrested_gangs_in_prison(int display_index, int gang_index) {
    float x = 83.0f;
    float y = 190.0f;
    float width = 97.0f;
    float height = 350.0f;

    int row = display_index / 2;
    int col = display_index % 2;

    float row_height = height / 5.0f;
    float col_width = width / 2.0f;

    float cell_x = x + col * col_width + col_width /4;
    float cell_y = y - row * row_height - row_height /2;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Gang %d", gang_index + 1);
    glColor3f(1.0f, 0.5f, 0.0f);
    glRasterPos2f(cell_x, cell_y);
    for (const char* c = buffer; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    glColor3f(0.0f, 0.0f, 0.0f);
}
Gang* get_gang_by_index(int index) {
    if (index < 0 || index >= config.gang_count) return NULL;

    if (!global_state) {
        fprintf(stderr, "[GUI] Shared memory not initialized in get_gang_by_index()\n");
        return NULL;
    }
    return &global_state->gangs[index];

}

int main(int argc, char *argv[]) {
    // Load config
    if (load_config("config.txt") != 0) {
        fprintf(stderr, "[GUI] Failed to load config.txt\n");
        return 1;
    }

    // Get shared memory only once
    global_state = get_shared_memory_gangs();
    if (!global_state) {
        fprintf(stderr, "[GUI] Failed to attach to shared memory.\n");
        return 1;
    }

    // Launch GUI
    init_gui(argc, argv);
    return 0;
}

