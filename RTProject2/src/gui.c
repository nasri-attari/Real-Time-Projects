#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "gui.h"



int prev_chef_roles[MAX_CHEFS];
int chef_change_timer[MAX_CHEFS];  // in seconds
int mock_change_triggered = 0;
void timer(int);

extern BakeryState *shared_state;

// === Initialize GUI (mocked)
void init_gui(int argc, char *argv[]) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Bakery Simulation Dashboard");

    glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // Light gray background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600); // 2D orthographic projection
    glMatrixMode(GL_MODELVIEW);

    glutDisplayFunc(display_gui);
    timer(0);

    for (int i = 0; i < MAX_CHEFS; i++) {
        prev_chef_roles[i] = shared_state->chef_roles[i];
        chef_change_timer[i] = 0;
    }
    glutMainLoop();
}
// === Render text at specific position
void render_text(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    for (const char *c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

// === Display callback
void display_gui() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    draw_ingredients_info();
    draw_status_box();
    draw_suppliers();
    // Chef teams on top left side
    draw_chef_team_by_role(10, 600, "Paste Chefs", ROLE_PASTE);
    draw_chef_team_by_role(205, 600, "Cake Chefs", ROLE_CAKE);
    draw_chef_team_by_role(10, 495, "Sandwich Chefs", ROLE_SANDWICH);
    draw_chef_team_by_role(205, 495, "Sweet Chefs", ROLE_SWEET);
    draw_chef_team_by_role(10, 390, "Savory Patisseries Chefs",  ROLE_SAVORY_PASTRY);
    draw_chef_team_by_role(205, 390, "Sweet patisseries Chefs", ROLE_SWEET_PASTRY );
    draw_bakers_header_bar();
    draw_prepared_items_near_chefs();
    // Baker teams on bottom left side
    draw_baker_team_by_role(10, 245, "Cakes & Sweets Bakers", ROLE_BAKE_CAKE_SWEET);
    draw_baker_team_by_role(205, 245, "Sweet & Savory Patisseries Bakers", ROLE_BAKE_PASTRY);
    draw_baker_team_by_role(10, 120, "Bread Bakers", ROLE_BAKE_BREAD);
    draw_baked_items_near_bakers();
    draw_kitchen_divider();
    draw_storage_bar();
    draw_manager();
    draw_sellers();
    draw_counter_with_items();
    draw_customer_queues();
    glutSwapBuffers();


}
void draw_ingredients_info() {
    char buffer[128];
    const float start_x = 520;         // Move slightly more right
    const float start_y = 560;

    const float column_spacing = 90.0f;  // Tighter horizontal space
    const float row_spacing = 22.0f;      // Keep vertical spacing as-is
    const int columns = 3;

    const char *ingredient_names[NUM_INGREDIENTS] = {
            "Wheat", "Yeast", "Butter", "Milk", "Sugar",
            "Salt", "Cheese", "Salami", "Sweet Items"
    };

    glColor3f(0.0f, 0.0f, 0.0f); // black
    render_text(start_x, start_y + 20, "Ingredients:");

    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        int col = i % columns;
        int row = i / columns;

        float x = start_x + col * column_spacing;
        float y = start_y - row * row_spacing;

        snprintf(buffer, sizeof(buffer), "%s: %d/%d",
                 ingredient_names[i],
                 shared_state->ingredient_stock[i],
                 global_config.ingredient_limits[i]);
        // Set text color BEFORE positioning or rendering
        if (shared_state->ingredient_stock[i] == 0)
            glColor3f(1.0f, 0.0f, 0.0f); // red
        else if (shared_state->ingredient_stock[i] ==global_config.ingredient_limits[i] )
            glColor3f(0.0f, 1.0f, 0.0f);
        else
            glColor3f(0.0f, 0.0f, 0.0f); // black

        render_text(x, y, buffer);
    }
}
void draw_status_box() {
    char buffer[128];
    float box_width = 330;
    float box_height = 70;

    float base_x = 520;
    float base_y = 480;  // direct alignment under ingredients

    float box_x = base_x;
    float box_y = base_y ;

    // === Draw box background border (simple rectangle)
    glColor3f(0.2f, 0.2f, 0.2f);  // dark gray border
    glBegin(GL_LINE_LOOP);
    glVertex2f(box_x, box_y);
    glVertex2f(box_x + box_width, box_y);
    glVertex2f(box_x + box_width, box_y - box_height);
    glVertex2f(box_x, box_y - box_height);
    glEnd();

    // === Text: inside the box
    glColor3f(0.0f, 0.0f, 0.0f); // black

    // Profit
    snprintf(buffer, sizeof(buffer), "Profit: $%d", shared_state->profit);
    render_text(box_x + 10, box_y - 15, buffer);

    // Customer stats
    glColor3f(1.0f, 0.5f, 0.0f);  // orange
    snprintf(buffer, sizeof(buffer), "Complaints: %d", shared_state->complaining_customers);
    render_text(box_x + 70, box_y - 15, buffer);

    glColor3f(1.0f, 0.0f, 0.0f);  // red
    snprintf(buffer, sizeof(buffer), "Frustrated: %d", shared_state->frustrated_customers);
    render_text(box_x + 70, box_y - 30, buffer);

    glColor3f(0.6f, 0.6f, 0.6f);  // gray
    snprintf(buffer, sizeof(buffer), "Missing: %d", shared_state->missing_item_customers);
    render_text(box_x + 70, box_y - 45, buffer);

    // Time countdown
    // Convert total simulation time and elapsed to seconds
    int total_seconds = global_config.simulation_duration_minutes * 60;
    int elapsed_seconds = shared_state->simulation_minutes_elapsed;
    int remaining_seconds = total_seconds - elapsed_seconds;
    if (remaining_seconds < 0) remaining_seconds = 0;

    int minutes = remaining_seconds / 60;
    int seconds = remaining_seconds % 60;

    glColor3f(0.0f, 0.0f, 0.0f);  // normal
    snprintf(buffer, sizeof(buffer), "Time: %02d:%02d", minutes, seconds);
    render_text(box_x + 10, box_y - 60, buffer);


}
void draw_suppliers() {
    float left_x = 400.0f;
    float right_x = 480.0f;
    float center_y = 540.0f;

    float spacing = (right_x - left_x) / 2.0f;

    for (int i = 0; i < 3; i++) {
        int status = shared_state->supply_worker_active[i];

        if (status == NOT_EXIST)
            continue;  // Skip inactive suppliers

        // Base color (blue-ish)
        float r = 0.2f, g = 0.5f, b = 0.9f;

        if (status == NOT_WORKING) {
            // Lighter version
            r = (r + 1.0f) / 2.0f;
            g = (g + 1.0f) / 2.0f;
            b = (b + 1.0f) / 2.0f;
        }

        float x = left_x + i * spacing;
        float y = center_y;

        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glScalef(1.0f, 1.0f, 1.0f);

        // Head
        glColor3f(r, g, b);
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.14159f * j / 20.0f;
            float cx = 0.0f + 10.0f * cos(theta);
            float cy = 30.0f + 10.0f * sin(theta);
            glVertex2f(cx, cy);
        }
        glEnd();

        // Body
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, 17.0f); glVertex2f(0.0f, 0.0f);         // torso
        glVertex2f(0.0f, 20.0f); glVertex2f(-10.0f, 15.0f);      // left arm
        glVertex2f(0.0f, 20.0f); glVertex2f(10.0f, 15.0f);       // right arm
        glVertex2f(0.0f, 0.0f); glVertex2f(-7.0f, -15.0f);       // left leg
        glVertex2f(0.0f, 0.0f); glVertex2f(7.0f, -15.0f);        // right leg
        glEnd();
        glLineWidth(1.0f);

        // Label
        glRasterPos2f(-15.0f, -25.0f);
        char label[16];
        sprintf(label, "Supplier %d", i + 1);
        for (char* c = label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }

        glPopMatrix();
    }
}
void draw_chef_team_by_role(float x, float y, const char* label, int role) {
    float box_width = 120;
    float box_height = 100;

    // Draw black team box
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + box_width, y);
    glVertex2f(x + box_width, y - box_height);
    glVertex2f(x, y - box_height);
    glEnd();

    // Label
    glRasterPos2f(x + 10, y - 15);
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // === Wider Vertical Table in Center ===
    float table_width = 20.0f;
    float table_height = 60.0f;
    float table_x = x + box_width / 2.0f - table_width / 2.0f;
    float table_y = y - box_height / 1.2f + table_height / 2.0f;

    glColor3f(0.6f, 0.6f, 0.6f);  // gray
    glBegin(GL_QUADS);
    glVertex2f(table_x, table_y + table_height / 2.0f);
    glVertex2f(table_x + table_width, table_y + table_height / 2.0f);
    glVertex2f(table_x + table_width, table_y - table_height / 2.0f);
    glVertex2f(table_x, table_y - table_height / 2.0f);
    glEnd();

    // === Add 6 Diverse Items on Table ===
    float item_center_x = table_x + table_width / 2.0f;
    float item_start_y = table_y + 25.0f;
    float spacing_y = 10.0f;

    for (int i = 0; i < 6; i++) {
        float item_y = item_start_y - i * spacing_y;

        switch (i % 3) {
            case 0: // Square
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2f(item_center_x - 3, item_y - 3);
                glVertex2f(item_center_x + 3, item_y - 3);
                glVertex2f(item_center_x + 3, item_y + 3);
                glVertex2f(item_center_x - 3, item_y + 3);
                glEnd();
                break;

            case 1: // Circle
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_POLYGON);
                for (int j = 0; j < 20; j++) {
                    float theta = 2.0f * 3.14159f * j / 20.0f;
                    float cx = item_center_x + 3.5f * cos(theta);
                    float cy = item_y + 3.5f * sin(theta);
                    glVertex2f(cx, cy);
                }
                glEnd();
                break;

            case 2: // Triangle
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_TRIANGLES);
                glVertex2f(item_center_x, item_y + 4);
                glVertex2f(item_center_x - 4, item_y - 3);
                glVertex2f(item_center_x + 4, item_y - 3);
                glEnd();
                break;
        }
    }

    // === Draw Chefs in 2x2 Grid ===
    float spacing_x = 80;
    float spacing_y_chef = 50;
    float chef_start_x = x + 25;
    float chef_start_y = y - 30;

    int drawn = 0;
    for (int i = 0; i < MAX_CHEFS && drawn < 4; i++) {
        if (shared_state->chef_roles[i] != role)
            continue;

        float cx = chef_start_x + (drawn % 2) * spacing_x;
        float cy = chef_start_y - (drawn / 2) * spacing_y_chef;

        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);

        // Flash if recently reassigned
        bool is_highlighted = chef_change_timer[i] > 0;
        glColor3f(is_highlighted ? 1.0f : 0.9f, is_highlighted ? 1.0f : 0.6f, is_highlighted ? 0.0f : 0.2f);

        // Head
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.14159f * j / 20.0f;
            glVertex2f(6.0f * cos(theta), 7.0f + 6.0f * sin(theta));
        }
        glEnd();

        // Body
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, 3.0f); glVertex2f(0.0f, -10.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(-5.0f, -5.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(5.0f, -5.0f);
        glVertex2f(0.0f, -10.0f); glVertex2f(-4.0f, -15.0f);
        glVertex2f(0.0f, -10.0f); glVertex2f(4.0f, -15.0f);
        glEnd();

        glPopMatrix();
        drawn++;
    }
}
void draw_prepared_items_near_chefs() {
    // X and Y positions mapped to each item type
    float item_positions[NUM_ITEM_TYPES +1][2] = {
            {135.0f, 415.0f},   // Sandwich
            {135.0f, 430.0f},   // Cake
            {135.0f, 445.0f},   // Sweet
            {135.0f, 460.0f},   // Sweet Patisserie
            {135.0f, 475.0f},   // Paste
            {135.0f, 490.0f},   // Savory Patisserie
    };

    const char* item_names[NUM_ITEM_TYPES +1] = { "Paste", "Cake", "Sweet","Sweet Patisserie" ,"Savory Patisserie","Sandwich" };

    // === Draw box around the items ===
    float box_x = 132.0f;
    float box_y_top = 530.0f;
    float box_width = 72.0f;
    float box_height = 140.0f;

    glColor3f(0.0f, 0.0f, 0.0f);  // Black border
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box_x, box_y_top);
    glVertex2f(box_x + box_width, box_y_top);
    glVertex2f(box_x + box_width, box_y_top - box_height);
    glVertex2f(box_x, box_y_top - box_height);
    glEnd();

    // === Draw title above all items ===
    glLineWidth(5.0f);
    glColor3f(0.0f, 0.0f, 0.0f);  // Black color
    glRasterPos2f(135.0f, 510.0f);
    const char* title = "Prepared Items: ";
    for (const char* c = title; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    for (int i = 0; i < NUM_ITEM_TYPES +1; i++) {
        float x = item_positions[i][0];
        float y = item_positions[i][1];

        char buffer[64];
        sprintf(buffer, "%s:%d", item_names[i], shared_state->prepared_items[i]);
        if(shared_state->prepared_items[i] == 0)
            glColor3f(1.0f, 0.0f, 0.0f); //Red
        else
            glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(x, y);
        for (char* c = buffer; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }
}
void draw_bakers_header_bar() {
    float bar_x = 10.0f;
    float bar_y = 280.0f;
    float bar_width = 320.0f;
    float bar_height = 30.0f;

    // Draw the black horizontal bar
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(bar_x, bar_y);
    glVertex2f(bar_x + bar_width, bar_y);
    glVertex2f(bar_x + bar_width, bar_y - bar_height);
    glVertex2f(bar_x, bar_y - bar_height);
    glEnd();

    // Write "Bakers" in white
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(bar_x + 130, bar_y - 20);
    const char *label = "Bakers";
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}
void draw_kitchen_divider() {
    float x = 330.0f;        // X position of the bar
    float y_top = 600.0f;    // Top of screen
    float y_bottom =0.0f;  // Bottom of usable area
    float width = 25.0f;

    // Draw the black vertical bar
    glColor3f(0.0f, 0.0f, 0.0f);  // black
    glBegin(GL_QUADS);
    glVertex2f(x, y_top);
    glVertex2f(x + width, y_top);
    glVertex2f(x + width, y_bottom);
    glVertex2f(x, y_bottom);
    glEnd();

    // Label: "KITCHEN" written vertically (one character at a time)
    glColor3f(1.0f, 1.0f, 1.0f); // white on black
    float label_x = x + 4.0f;     // center inside the black bar
    float label_y = y_top - 190.0f;
    const char* label = "KITCHEN";
    for (int i = 0; label[i] != '\0'; i++) {
        glRasterPos2f(label_x, label_y - i * 20.0f);  // step down per letter
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, label[i]);
    }

    glPopMatrix();
}
void draw_storage_bar() {
    float bar_x = 350.0f;
    float bar_y = 510.0f;           // just above suppliers
    float bar_width = 500.0f;
    float bar_height = 25.0f;

    // Draw the black rectangle
    glColor3f(0.0f, 0.0f, 0.0f);    // black
    glBegin(GL_QUADS);
    glVertex2f(bar_x, bar_y);
    glVertex2f(bar_x + bar_width, bar_y);
    glVertex2f(bar_x + bar_width, bar_y - bar_height);
    glVertex2f(bar_x, bar_y - bar_height);
    glEnd();

    // Draw the label "Storage" in white
    glColor3f(1.0f, 1.0f, 1.0f);    // white
    glRasterPos2f(bar_x + 220.0f, bar_y - 17.0f);
    const char* label = "Storage";
    for (const char* c = label; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}
void draw_manager() {
    float manager_x = 400.0f;
    float manager_y = 450.0f;

    glPushMatrix();
    glTranslatef(manager_x, manager_y, 0.0f);

    // Head
    glColor3f(0.0f, 0.0f, 0.6f);  // dark blue
    glBegin(GL_POLYGON);
    for (int j = 0; j < 20; j++) {
        float theta = 2.0f * 3.14159f * j / 20.0f;
        float px = 0.0f + 10.0f * cos(theta);
        float py = 20.0f + 10.0f * sin(theta);
        glVertex2f(px, py);
    }
    glEnd();

    // Body
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 15.0f); glVertex2f(0.0f, -5.0f);
    glVertex2f(0.0f, 5.0f); glVertex2f(-7.0f, 0.0f);
    glVertex2f(0.0f, 5.0f); glVertex2f(7.0f, 0.0f);
    glVertex2f(0.0f, -5.0f); glVertex2f(-5.0f, -15.0f);
    glVertex2f(0.0f, -5.0f); glVertex2f(5.0f, -15.0f);
    glEnd();

    // Label
    glRasterPos2f(-18.0f, -30.0f);
    const char* label = "Manager";
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // === Manager Speech Bubble ===
    const char* message = shared_state->manager_command;
    if (message[0] != '\0') {
        float bubble_width = 150.0f;
        float bubble_height = 30.0f;
        float bubble_x =  10;
        float bubble_y = 30.0f;

        // Background
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2f(bubble_x, bubble_y);
        glVertex2f(bubble_x + bubble_width, bubble_y);
        glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
        glVertex2f(bubble_x, bubble_y + bubble_height);
        glEnd();

        // Border
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(bubble_x, bubble_y);
        glVertex2f(bubble_x + bubble_width, bubble_y);
        glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
        glVertex2f(bubble_x, bubble_y + bubble_height);
        glEnd();

        // Text
        glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(bubble_x + 5.0f, bubble_y + 10.0f);
        for (const char* c = message; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }
    }

    glPopMatrix();
}
void draw_sellers() {
    float start_x = 400.0f;      // starting x-position
    float y = 380.0f;            // fixed y-position for all sellers
    float spacing_x = 70.0f;     // horizontal spacing between sellers

    for (int i = 0; i < 6; i++) {
        int status = shared_state->seller_active[i];

        if (status == NOT_EXIST)
            continue;  // skip inactive seller

        // Base color (green)
        float r = 0.0f, g = 0.6f, b = 0.0f;

        if (status == NOT_WORKING) {
            // Lighter version of green
            r = (r + 1.0f) / 2.0f;
            g = (g + 1.0f) / 2.0f;
            b = (b + 1.0f) / 2.0f;
        }

        float x = start_x + i * spacing_x;

        glPushMatrix();
        glTranslatef(x, y, 0.0f);

        // Head
        glColor3f(r, g, b);
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.14159f * j / 20.0f;
            float px = 0.0f + 8.0f * cos(theta);
            float py = 11.0f + 8.0f * sin(theta);
            glVertex2f(px, py);
        }
        glEnd();

        // Body
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, 6.0f); glVertex2f(0.0f, -5.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(-5.0f, -3.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(5.0f, -3.0f);
        glVertex2f(0.0f, -5.0f); glVertex2f(-4.0f, -12.0f);
        glVertex2f(0.0f, -5.0f); glVertex2f(4.0f, -12.0f);
        glEnd();

        // Label
        char label[16];
        sprintf(label, "Seller %d", i + 1);
        glRasterPos2f(-3.0f, -25.0f);
        for (char* c = label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }

        // === Response Bubble ===
        const char* response = shared_state->seller_responds[i];
        if (response[0] != '\0') {
            // Bubble dimensions
            float bubble_width = 69.0f;
            float bubble_height = 20.0f;
            float bubble_x = -bubble_width / 2;
            float bubble_y = 15.0f;

            // Background
            glColor3f(1.0f, 1.0f, 1.0f);  // white background
            glBegin(GL_QUADS);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
            glVertex2f(bubble_x, bubble_y + bubble_height);
            glEnd();

            // Border
            glColor3f(0.0f, 0.0f, 0.0f);  // black border
            glLineWidth(1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y);
            glVertex2f(bubble_x + bubble_width, bubble_y + bubble_height);
            glVertex2f(bubble_x, bubble_y + bubble_height);
            glEnd();

            // Text
            glColor3f(0.0f, 0.0f, 0.0f);
            glRasterPos2f(bubble_x + 5.0f, bubble_y + 10.0f);
            for (const char* c = response; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
            }
        }

        glPopMatrix();
    }
}
void draw_counter_with_items() {
    float counter_x = 360.0f;
    float counter_y = 350.0f;
    float counter_width = 500.0f;
    float counter_height = 45.0f;
    float section_width = counter_width / 7.0f;

    // Outer counter border
    glColor3f(0.1f, 0.1f, 0.1f);  // dark frame
    glBegin(GL_QUADS);
    glVertex2f(counter_x, counter_y);
    glVertex2f(counter_x + counter_width, counter_y);
    glVertex2f(counter_x + counter_width, counter_y - counter_height);
    glVertex2f(counter_x, counter_y - counter_height);
    glEnd();

    // Draw segment dividers
    glColor3f(0.3f, 0.3f, 0.3f);  // slightly lighter gray
    for (int i = 1; i < 6; i++) {
        float line_x = counter_x + i * section_width;
        glBegin(GL_LINES);
        glVertex2f(line_x, counter_y);
        glVertex2f(line_x, counter_y - counter_height);
        glEnd();
    }

    // Item names (UI order)
    const char* items[6] = {
            "Cake", "Sweet", "Sandwich", "Sweet Patisseries", "Savory Patisseries", "Bread"
    };

    // Matching baked_items[] indices (except Sandwich)
    int item_indices[6] = {
            ITEM_CAKE, ITEM_SWEET, ITEM_SANDWICH, ITEM_SWEET_PATISSERIE,
            ITEM_SAVORY_PATISSERIE, ITEM_BREAD
    };

    // Render names and values
    for (int i = 0; i < 6; i++) {
        float text_x = counter_x + i * section_width + 10.0f;

        // Item name
        float name_y = counter_y - 28.0f;
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(text_x, name_y);
        for (const char* c = items[i]; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }

        // Value
        int value = (i == 2)  // index 2 = Sandwich
                    ? shared_state->prepared_items[ITEM_SANDWICH]
                    : shared_state->baked_items[item_indices[i]];

        char count_str[10];
        sprintf(count_str, "%d", value);

        float count_y = counter_y - 42.0f;
        glRasterPos2f(text_x, count_y);
        for (char* c = count_str; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }
    }
}
void draw_customer_queues() {
    float start_x = 380.0f;
    float spacing_x = 80.0f;
    float base_y = 250.0f;
    float spacing_y = 70.0f;

    static float customer_y[MAX_CUSTOMERS] = {0};
    static int customer_initialized[MAX_CUSTOMERS] = {0};
    static int customer_label_number[MAX_CUSTOMERS] = {0};
    static int next_customer_number = 1;

    int rendered_customers[MAX_CUSTOMERS];
    int rendered_count = 0;

    // Collect active customers (state == CUSTOMER_EXIST)
    for (int i = 0; i < MAX_CUSTOMERS; i++) {
        if (shared_state->customer_status[i] == CUSTOMER_EXIST ||
            shared_state->customer_status[i] == CUSTOMER_FRUSTRATED ||
            shared_state->customer_status[i] == CUSTOMER_COMPLAINING ||
            shared_state->customer_status[i] == CUSTOMER_NO_ITEMS)
        {
            rendered_customers[rendered_count++] = i;
        } else {
            customer_initialized[i] = 0;  // Reset if inactive or removed
        }
    }

    // Render all active customers in visual grid order
    for (int r = 0; r < rendered_count; r++) {
        int i = rendered_customers[r];

        int col = r % 6;
        int row = r / 6;

        float x = start_x + col * spacing_x;
        float target_y = base_y - row * spacing_y;

        // First time activation
        if (!customer_initialized[i]) {
            customer_y[i] = 0.0f;
            customer_label_number[i] = next_customer_number++;
            customer_initialized[i] = 1;
        }

        // Animate toward target Y
        if (customer_y[i] < target_y) {
            customer_y[i] += 70.0f;
            if (customer_y[i] > target_y) customer_y[i] = target_y;
        } else if (customer_y[i] > target_y) {
            customer_y[i] -= 4.0f;
            if (customer_y[i] < target_y) customer_y[i] = target_y;
        }

        // === Draw customer figure ===

        // Head

        glPushMatrix();
        glTranslatef(x, customer_y[i], 0.0f);

        switch (shared_state->customer_status[i]) {
            case CUSTOMER_EXIST:
                glColor3f(0.2f, 0.5f, 0.9f);  // normal
                break;
            case CUSTOMER_FRUSTRATED:
                glColor3f(1.0f, 0.0f, 0.0f);  // red
                break;
            case CUSTOMER_COMPLAINING:
                glColor3f(1.0f, 0.5f, 0.0f);  // orange
                break;
            case CUSTOMER_NO_ITEMS:
                glColor3f(0.6f, 0.6f, 0.6f);  // gray
                break;
        }

        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.14159f * j / 20.0f;
            float cx = 10.0f * cos(theta);
            float cy = 30.0f + 10.0f * sin(theta);
            glVertex2f(cx, cy);
        }
        glEnd();

        // Body
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, 17.0f); glVertex2f(0.0f, 0.0f);
        glVertex2f(0.0f, 20.0f); glVertex2f(-10.0f, 15.0f);
        glVertex2f(0.0f, 20.0f); glVertex2f(10.0f, 15.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(-7.0f, -15.0f);
        glVertex2f(0.0f, 0.0f); glVertex2f(7.0f, -15.0f);
        glEnd();
        glLineWidth(1.0f);

        glPopMatrix();

        // === Customer label ===
        char label[21];
        sprintf(label, "Customer %d", customer_label_number[i] - 1);
        glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(x - 25.0f, customer_y[i] - 25.0f);
        for (char* c = label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }

        // === Speech Bubble ===
        const char* msg = shared_state->customer_requests[i];
        if (msg[0] != '\0') {
            float bubble_w = 78.0f;
            float bubble_h = 20.0f;
            float bubble_x = x - bubble_w / 2;
            float bubble_y = customer_y[i] + 35.0f;

            // Background
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_w, bubble_y);
            glVertex2f(bubble_x + bubble_w, bubble_y + bubble_h);
            glVertex2f(bubble_x, bubble_y + bubble_h);
            glEnd();

            // Border
            glColor3f(0.0f, 0.0f, 0.0f);
            glLineWidth(1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bubble_x, bubble_y);
            glVertex2f(bubble_x + bubble_w, bubble_y);
            glVertex2f(bubble_x + bubble_w, bubble_y + bubble_h);
            glVertex2f(bubble_x, bubble_y + bubble_h);
            glEnd();

            // Text
            glColor3f(0.0f, 0.0f, 0.0f);
            glRasterPos2f(bubble_x + 5.0f, bubble_y + 8.0f);
            for (const char* c = msg; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
            }
        }
    }
}

void draw_baker_team_by_role(float x, float y, const char* label, int role) {
    float box_width = 120;
    float box_height = 100;

    // Draw team box
    glColor3f(0.0f, 0.0f, 0.0f);  // black border
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + box_width, y);
    glVertex2f(x + box_width, y - box_height);
    glVertex2f(x, y - box_height);
    glEnd();

    // Label
    glRasterPos2f(x + 10, y - 15);
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // 2x2 layout for bakers
    float spacing_x = 80;
    float spacing_y = 50;
    float start_x = x + 25;
    float start_y = y - 35;

    int drawn = 0;
    for (int i = 0; i < MAX_BAKERS && drawn < 4; i++) {
        if (shared_state->baker_roles[i] != role)
            continue;

        float cx = start_x + (drawn % 2) * spacing_x;
        float cy = start_y - (drawn / 2) * spacing_y;

        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);

        // Head
        glColor3f(0.3f, 0.7f, 0.3f);  // green-ish for bakers
        glBegin(GL_POLYGON);
        for (int j = 0; j < 20; j++) {
            float theta = 2.0f * 3.14159f * j / 20.0f;
            float px = 6.0f * cos(theta);
            float py = 7.0f + 6.0f * sin(theta);
            glVertex2f(px, py);
        }
        glEnd();

        // Body
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(0.0f, 3.0f); glVertex2f(0.0f, -10.0f);     // torso
        glVertex2f(0.0f, 0.0f); glVertex2f(-5.0f, -5.0f);     // left arm
        glVertex2f(0.0f, 0.0f); glVertex2f(5.0f, -5.0f);      // right arm
        glVertex2f(0.0f, -10.0f); glVertex2f(-4.0f, -15.0f);  // left leg
        glVertex2f(0.0f, -10.0f); glVertex2f(4.0f, -15.0f);   // right leg
        glEnd();

        glPopMatrix();
        drawn++;
    }

    // ===== Render "Idle Bakers: N" =====
    int idle_count = 0;
    for (int i = 0; i < MAX_BAKERS; i++) {
        if (shared_state->baker_roles[i] == ROLE_BAKE_NOT_WORKING)
            idle_count++;
    }

    char label_idle[50];
    sprintf(label_idle, "Idle Bakers: %d", idle_count);
    glColor3f(0.0f, 0.0f, 1.0f);  // blue text
    glRasterPos2f(140, 140);  // to the left of the bread bakers box
    for (char* c = label_idle; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // === Vertical Table Centered ===
    float table_width = 20.0f;
    float table_height = 60.0f;
    float table_x = x + box_width / 2.0f - table_width / 2.0f;
    float table_y = y - box_height / 1.2f + table_height / 2.0f;

    glColor3f(0.6f, 0.6f, 0.6f);  // gray
    glBegin(GL_QUADS);
    glVertex2f(table_x, table_y + table_height / 2.0f);
    glVertex2f(table_x + table_width, table_y + table_height / 2.0f);
    glVertex2f(table_x + table_width, table_y - table_height / 2.0f);
    glVertex2f(table_x, table_y - table_height / 2.0f);
    glEnd();

    // === Place 6 Colorful Items on Table ===
    float item_center_x = table_x + table_width / 2.0f;
    float item_start_y = table_y + 25.0f;
    float spacing_y_items = 10.0f;

    for (int i = 0; i < 6; i++) {
        float item_y = item_start_y - i * spacing_y_items;

        switch (i % 3) {
            case 0: // Square
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2f(item_center_x - 3, item_y - 3);
                glVertex2f(item_center_x + 3, item_y - 3);
                glVertex2f(item_center_x + 3, item_y + 3);
                glVertex2f(item_center_x - 3, item_y + 3);
                glEnd();
                break;

            case 1: // Circle
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_POLYGON);
                for (int j = 0; j < 20; j++) {
                    float theta = 2.0f * 3.14159f * j / 20.0f;
                    float cx = item_center_x + 3.5f * cos(theta);
                    float cy = item_y + 3.5f * sin(theta);
                    glVertex2f(cx, cy);
                }
                glEnd();
                break;

            case 2: // Triangle
                glColor3f(0.2f, 0.6f, 1.0f);
                glBegin(GL_TRIANGLES);
                glVertex2f(item_center_x, item_y + 4);
                glVertex2f(item_center_x - 4, item_y - 3);
                glVertex2f(item_center_x + 4, item_y - 3);
                glEnd();
                break;
        }
    }
}
void draw_baked_items_near_bakers() {
    float item_positions[NUM_ITEM_TYPES][2] = {
            {210.0f, 100.0f},  // Bread
            {210.0f, 85.0f},  // Cake
            {210.0f, 70.0f},  // Sweet
            {210.0f, 55.0f},  // Sweet Patisserie
            {210.0f, 40.0f},  // Savory Patisserie
    };

    const char* item_names[NUM_ITEM_TYPES] = {
            "Bread", "Cake", "Sweet", "Sweet Patisserie","Savory Patisserie"
    };


    // === Draw box around baked items ===
    float box_x = 205.0f;
    float box_y_top = 130.0f;
    float box_width = 70.0f;
    float box_height = 115.0f;

    glColor3f(0.0f, 0.0f, 0.0f);  // Black border
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(box_x, box_y_top);
    glVertex2f(box_x + box_width, box_y_top);
    glVertex2f(box_x + box_width, box_y_top - box_height);
    glVertex2f(box_x, box_y_top - box_height);
    glEnd();

    // === Draw title above baked items ===
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(210.0f, 115.0f);
    const char* title = "Baked Items: ";
    for (const char* c = title; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // === Draw each baked item count ===
    for (int i = 0; i < NUM_ITEM_TYPES; i++) {
        float x = item_positions[i][0];
        float y = item_positions[i][1];

        char buffer[64];
        sprintf(buffer, "%s: %d", item_names[i], shared_state->baked_items[i]);
        if(shared_state->baked_items[i] == 0)
            glColor3f(1.0f, 0.0f, 0.0f); //Red
        else
            glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(x, y);
        for (char* c = buffer; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
        }
    }
}

int main(int argc, char *argv[]) {
    // Attach shared memory here
    shared_state = get_shared_memory();

    // Load config
    load_config_from_file("config.txt");

    // Start the GUI
    init_gui(argc, argv);

    return 0;
}


void timer(int value) {
    glutPostRedisplay();              // Force re-rendering
    glutTimerFunc(1000, timer, 0);    // Call again after 1000ms (1 second)
}






