#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH 60
#define HEIGHT 20
#define MAX_OBJECTS 100

// ANSI Color codes for styled output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BOLD    "\033[1m"

typedef enum {
    TYPE_LINE = 1,
    TYPE_RECTANGLE,
    TYPE_TRIANGLE,
    TYPE_CIRCLE
} ObjectType;

typedef struct {
    int x1, y1, x2, y2;
} LineParams;

typedef struct {
    int x, y, w, h;
} RectParams;

typedef struct {
    int x1, y1, x2, y2, x3, y3;
} TriParams;

typedef struct {
    int cx, cy, r;
} CircleParams;

typedef struct {
    int id;
    ObjectType type;
    union {
        LineParams line;
        RectParams rect;
        TriParams tri;
        CircleParams circle;
    } params;
    int is_active;
} GraphicObject;

GraphicObject objects[MAX_OBJECTS];
int next_object_id = 1;
char canvas[HEIGHT][WIDTH];

// Safely clear screen
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    printf("\033[H\033[J");
#endif
}

// Prompt utility to safely read an integer with min/max bounds validation
int get_input_int(const char* prompt, int min_val, int max_val) {
    char buf[128];
    int val;
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            return min_val; // Fail-safe
        }
        // Remove newline
        buf[strcspn(buf, "\n")] = '\0';
        
        // Check if string is empty
        if (strlen(buf) == 0) {
            continue;
        }

        // Try parsing
        if (sscanf(buf, "%d", &val) == 1) {
            if (val >= min_val && val <= max_val) {
                return val;
            } else {
                printf(COLOR_RED "Error: Input must be between %d and %d.\n" COLOR_RESET, min_val, max_val);
            }
        } else {
            printf(COLOR_RED "Error: Invalid number. Please enter a valid integer.\n" COLOR_RESET);
        }
    }
}

// Safely read a string (for filenames)
void get_input_str(const char* prompt, char* out_str, int max_len) {
    char buf[256];
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            out_str[0] = '\0';
            return;
        }
        buf[strcspn(buf, "\n")] = '\0';
        if (strlen(buf) > 0) {
            strncpy(out_str, buf, max_len - 1);
            out_str[max_len - 1] = '\0';
            return;
        }
    }
}

// Bresenham's line algorithm
void draw_line_on_canvas(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
            canvas[y0][x0] = '*';
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Circle points plotting helper
void plot_circle_points(int xc, int yc, int x, int y) {
    #define PLOT(px, py) do { \
        if ((px) >= 0 && (px) < WIDTH && (py) >= 0 && (py) < HEIGHT) { \
            canvas[(py)][(px)] = '*'; \
        } \
    } while(0)

    PLOT(xc + x, yc + y);
    PLOT(xc - x, yc + y);
    PLOT(xc + x, yc - y);
    PLOT(xc - x, yc - y);
    PLOT(xc + y, yc + x);
    PLOT(xc - y, yc + x);
    PLOT(xc + y, yc - x);
    PLOT(xc - y, yc - x);

    #undef PLOT
}

// Midpoint circle algorithm
void draw_circle_on_canvas(int xc, int yc, int r) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    
    plot_circle_points(xc, yc, x, y);
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        plot_circle_points(xc, yc, x, y);
    }
}

// Rectangle drawing
void draw_rectangle_on_canvas(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    // Draw top and bottom sides
    for (int i = x; i < x + w; i++) {
        if (i >= 0 && i < WIDTH) {
            if (y >= 0 && y < HEIGHT) canvas[y][i] = '*';
            if (y + h - 1 >= 0 && y + h - 1 < HEIGHT) canvas[y + h - 1][i] = '*';
        }
    }
    // Draw left and right sides
    for (int j = y; j < y + h; j++) {
        if (j >= 0 && j < HEIGHT) {
            if (x >= 0 && x < WIDTH) canvas[j][x] = '*';
            if (x + w - 1 >= 0 && x + w - 1 < WIDTH) canvas[j][x + w - 1] = '*';
        }
    }
}

// Triangle drawing
void draw_triangle_on_canvas(int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line_on_canvas(x1, y1, x2, y2);
    draw_line_on_canvas(x2, y2, x3, y3);
    draw_line_on_canvas(x3, y3, x1, y1);
}

// Render all shapes to canvas
void render_canvas() {
    // Fill background with '_'
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x] = '_';
        }
    }
    
    // Draw each active shape
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].is_active) continue;
        
        switch (objects[i].type) {
            case TYPE_LINE:
                draw_line_on_canvas(
                    objects[i].params.line.x1, objects[i].params.line.y1,
                    objects[i].params.line.x2, objects[i].params.line.y2);
                break;
            case TYPE_RECTANGLE:
                draw_rectangle_on_canvas(
                    objects[i].params.rect.x, objects[i].params.rect.y,
                    objects[i].params.rect.w, objects[i].params.rect.h);
                break;
            case TYPE_TRIANGLE:
                draw_triangle_on_canvas(
                    objects[i].params.tri.x1, objects[i].params.tri.y1,
                    objects[i].params.tri.x2, objects[i].params.tri.y2,
                    objects[i].params.tri.x3, objects[i].params.tri.y3);
                break;
            case TYPE_CIRCLE:
                draw_circle_on_canvas(
                    objects[i].params.circle.cx, objects[i].params.circle.cy,
                    objects[i].params.circle.r);
                break;
        }
    }
}

// Display canvas to stdout
void display_canvas() {
    clear_screen();
    render_canvas();
    
    printf(COLOR_CYAN "============================================================\n" COLOR_RESET);
    printf(COLOR_GREEN "                   2D C VECTOR CANVAS\n" COLOR_RESET);
    printf(COLOR_CYAN "============================================================\n" COLOR_RESET);
    
    // Print column ruler (tens digit)
    printf("    ");
    for (int x = 0; x < WIDTH; x++) {
        if (x % 10 == 0) {
            printf("%d", x / 10);
        } else {
            printf(" ");
        }
    }
    printf("\n");
    
    // Print column ruler (units digit)
    printf("    ");
    for (int x = 0; x < WIDTH; x++) {
        printf("%d", x % 10);
    }
    printf("\n");
    
    // Print top border
    printf("   +");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");
    
    // Print grid contents
    for (int y = 0; y < HEIGHT; y++) {
        printf("%2d |", y);
        for (int x = 0; x < WIDTH; x++) {
            if (canvas[y][x] == '*') {
                printf(COLOR_YELLOW "*" COLOR_RESET);
            } else {
                printf("_");
            }
        }
        printf("|\n");
    }
    
    // Print bottom border
    printf("   +");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");
    printf(COLOR_CYAN "============================================================\n" COLOR_RESET);
}

// Print object description
void print_object_details(GraphicObject* obj) {
    if (!obj->is_active) return;
    
    printf(COLOR_CYAN "[ID: %d] " COLOR_RESET, obj->id);
    switch (obj->type) {
        case TYPE_LINE:
            printf(COLOR_BOLD "LINE      " COLOR_RESET "from (%d, %d) to (%d, %d)\n",
                   obj->params.line.x1, obj->params.line.y1,
                   obj->params.line.x2, obj->params.line.y2);
            break;
        case TYPE_RECTANGLE:
            printf(COLOR_BOLD "RECTANGLE " COLOR_RESET "at (%d, %d) width=%d, height=%d\n",
                   obj->params.rect.x, obj->params.rect.y,
                   obj->params.rect.w, obj->params.rect.h);
            break;
        case TYPE_TRIANGLE:
            printf(COLOR_BOLD "TRIANGLE  " COLOR_RESET "vertices (%d,%d), (%d,%d), (%d,%d)\n",
                   obj->params.tri.x1, obj->params.tri.y1,
                   obj->params.tri.x2, obj->params.tri.y2,
                   obj->params.tri.x3, obj->params.tri.y3);
            break;
        case TYPE_CIRCLE:
            printf(COLOR_BOLD "CIRCLE    " COLOR_RESET "center (%d, %d) radius=%d\n",
                   obj->params.circle.cx, obj->params.circle.cy,
                   obj->params.circle.r);
            break;
    }
}

// Show list of all active objects
void list_objects() {
    int count = 0;
    printf(COLOR_CYAN "\nActive Objects in Picture:\n" COLOR_RESET);
    printf("----------------------------------------\n");
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active) {
            print_object_details(&objects[i]);
            count++;
        }
    }
    if (count == 0) {
        printf(COLOR_YELLOW "(No objects drawn on the canvas yet)\n" COLOR_RESET);
    }
    printf("----------------------------------------\n");
}

// Add Object
void menu_add_object() {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf(COLOR_RED "Error: Maximum object capacity (%d) reached. Please delete some objects.\n" COLOR_RESET, MAX_OBJECTS);
        return;
    }
    
    printf(COLOR_CYAN "\nSelect Shape Type to Add:\n" COLOR_RESET);
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Triangle\n");
    printf("4. Circle\n");
    printf("5. Cancel\n");
    
    int choice = get_input_int("Enter selection (1-5): ", 1, 5);
    if (choice == 5) return;
    
    GraphicObject new_obj;
    new_obj.id = next_object_id++;
    new_obj.type = (ObjectType)choice;
    new_obj.is_active = 1;
    
    switch (new_obj.type) {
        case TYPE_LINE:
            printf("\nAdding LINE:\n");
            new_obj.params.line.x1 = get_input_int("Enter X1 (0-59): ", 0, WIDTH - 1);
            new_obj.params.line.y1 = get_input_int("Enter Y1 (0-19): ", 0, HEIGHT - 1);
            new_obj.params.line.x2 = get_input_int("Enter X2 (0-59): ", 0, WIDTH - 1);
            new_obj.params.line.y2 = get_input_int("Enter Y2 (0-19): ", 0, HEIGHT - 1);
            break;
            
        case TYPE_RECTANGLE:
            printf("\nAdding RECTANGLE:\n");
            new_obj.params.rect.x = get_input_int("Enter Top-Left X (0-59): ", 0, WIDTH - 1);
            new_obj.params.rect.y = get_input_int("Enter Top-Left Y (0-19): ", 0, HEIGHT - 1);
            new_obj.params.rect.w = get_input_int("Enter Width (1-60): ", 1, WIDTH);
            new_obj.params.rect.h = get_input_int("Enter Height (1-20): ", 1, HEIGHT);
            break;
            
        case TYPE_TRIANGLE:
            printf("\nAdding TRIANGLE:\n");
            new_obj.params.tri.x1 = get_input_int("Enter Vertex 1 X (0-59): ", 0, WIDTH - 1);
            new_obj.params.tri.y1 = get_input_int("Enter Vertex 1 Y (0-19): ", 0, HEIGHT - 1);
            new_obj.params.tri.x2 = get_input_int("Enter Vertex 2 X (0-59): ", 0, WIDTH - 1);
            new_obj.params.tri.y2 = get_input_int("Enter Vertex 2 Y (0-19): ", 0, HEIGHT - 1);
            new_obj.params.tri.x3 = get_input_int("Enter Vertex 3 X (0-59): ", 0, WIDTH - 1);
            new_obj.params.tri.y3 = get_input_int("Enter Vertex 3 Y (0-19): ", 0, HEIGHT - 1);
            break;
            
        case TYPE_CIRCLE:
            printf("\nAdding CIRCLE:\n");
            new_obj.params.circle.cx = get_input_int("Enter Center X (0-59): ", 0, WIDTH - 1);
            new_obj.params.circle.cy = get_input_int("Enter Center Y (0-19): ", 0, HEIGHT - 1);
            new_obj.params.circle.r  = get_input_int("Enter Radius (0-40): ", 0, 40);
            break;
    }
    
    objects[slot] = new_obj;
    printf(COLOR_GREEN "\nObject ID %d added successfully!\n" COLOR_RESET, new_obj.id);
}

// Delete Object
void menu_delete_object() {
    list_objects();
    
    // Check if there are active objects
    int active_exists = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active) {
            active_exists = 1;
            break;
        }
    }
    if (!active_exists) return;
    
    int delete_id = get_input_int("Enter the ID of the object to delete (or 0 to cancel): ", 0, 9999);
    if (delete_id == 0) return;
    
    int found = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active && objects[i].id == delete_id) {
            objects[i].is_active = 0;
            printf(COLOR_GREEN "Object ID %d deleted successfully.\n" COLOR_RESET, delete_id);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf(COLOR_RED "Error: Object ID %d not found.\n" COLOR_RESET, delete_id);
    }
}

// Modify Object
void menu_modify_object() {
    list_objects();
    
    // Check if there are active objects
    int active_exists = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active) {
            active_exists = 1;
            break;
        }
    }
    if (!active_exists) return;
    
    int modify_id = get_input_int("Enter the ID of the object to modify (or 0 to cancel): ", 0, 9999);
    if (modify_id == 0) return;
    
    int found_index = -1;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active && objects[i].id == modify_id) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == -1) {
        printf(COLOR_RED "Error: Object ID %d not found.\n" COLOR_RESET, modify_id);
        return;
    }
    
    GraphicObject* obj = &objects[found_index];
    printf("\nModifying Object Details:\n");
    print_object_details(obj);
    printf("\n");
    
    switch (obj->type) {
        case TYPE_LINE:
            obj->params.line.x1 = get_input_int("Enter new X1 (0-59): ", 0, WIDTH - 1);
            obj->params.line.y1 = get_input_int("Enter new Y1 (0-19): ", 0, HEIGHT - 1);
            obj->params.line.x2 = get_input_int("Enter new X2 (0-59): ", 0, WIDTH - 1);
            obj->params.line.y2 = get_input_int("Enter new Y2 (0-19): ", 0, HEIGHT - 1);
            break;
            
        case TYPE_RECTANGLE:
            obj->params.rect.x = get_input_int("Enter new Top-Left X (0-59): ", 0, WIDTH - 1);
            obj->params.rect.y = get_input_int("Enter new Top-Left Y (0-19): ", 0, HEIGHT - 1);
            obj->params.rect.w = get_input_int("Enter new Width (1-60): ", 1, WIDTH);
            obj->params.rect.h = get_input_int("Enter new Height (1-20): ", 1, HEIGHT);
            break;
            
        case TYPE_TRIANGLE:
            obj->params.tri.x1 = get_input_int("Enter new Vertex 1 X (0-59): ", 0, WIDTH - 1);
            obj->params.tri.y1 = get_input_int("Enter new Vertex 1 Y (0-19): ", 0, HEIGHT - 1);
            obj->params.tri.x2 = get_input_int("Enter new Vertex 2 X (0-59): ", 0, WIDTH - 1);
            obj->params.tri.y2 = get_input_int("Enter new Vertex 2 Y (0-19): ", 0, HEIGHT - 1);
            obj->params.tri.x3 = get_input_int("Enter new Vertex 3 X (0-59): ", 0, WIDTH - 1);
            obj->params.tri.y3 = get_input_int("Enter new Vertex 3 Y (0-19): ", 0, HEIGHT - 1);
            break;
            
        case TYPE_CIRCLE:
            obj->params.circle.cx = get_input_int("Enter new Center X (0-59): ", 0, WIDTH - 1);
            obj->params.circle.cy = get_input_int("Enter new Center Y (0-19): ", 0, HEIGHT - 1);
            obj->params.circle.r  = get_input_int("Enter new Radius (0-40): ", 0, 40);
            break;
    }
    
    printf(COLOR_GREEN "Object ID %d modified successfully!\n" COLOR_RESET, modify_id);
}

// Save objects to a text file
void menu_save_to_file() {
    char filename[128];
    get_input_str("Enter filename to save drawing (e.g. drawing.txt): ", filename, sizeof(filename));
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(COLOR_RED "Error: Could not open file %s for writing.\n" COLOR_RESET, filename);
        return;
    }
    
    // Save next object id
    fprintf(file, "%d\n", next_object_id);
    
    // Count active objects
    int count = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].is_active) count++;
    }
    fprintf(file, "%d\n", count);
    
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].is_active) continue;
        
        fprintf(file, "%d %d ", objects[i].id, (int)objects[i].type);
        
        switch (objects[i].type) {
            case TYPE_LINE:
                fprintf(file, "%d %d %d %d\n", 
                        objects[i].params.line.x1, objects[i].params.line.y1,
                        objects[i].params.line.x2, objects[i].params.line.y2);
                break;
            case TYPE_RECTANGLE:
                fprintf(file, "%d %d %d %d\n", 
                        objects[i].params.rect.x, objects[i].params.rect.y,
                        objects[i].params.rect.w, objects[i].params.rect.h);
                break;
            case TYPE_TRIANGLE:
                fprintf(file, "%d %d %d %d %d %d\n", 
                        objects[i].params.tri.x1, objects[i].params.tri.y1,
                        objects[i].params.tri.x2, objects[i].params.tri.y2,
                        objects[i].params.tri.x3, objects[i].params.tri.y3);
                break;
            case TYPE_CIRCLE:
                fprintf(file, "%d %d %d\n", 
                        objects[i].params.circle.cx, objects[i].params.circle.cy,
                        objects[i].params.circle.r);
                break;
        }
    }
    
    fclose(file);
    printf(COLOR_GREEN "Drawing saved to %s successfully!\n" COLOR_RESET, filename);
}

// Load objects from a text file
void menu_load_from_file() {
    char filename[128];
    get_input_str("Enter filename to load drawing from: ", filename, sizeof(filename));
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf(COLOR_RED "Error: Could not open file %s for reading.\n" COLOR_RESET, filename);
        return;
    }
    
    // Clear current database
    for (int i = 0; i < MAX_OBJECTS; i++) {
        objects[i].is_active = 0;
    }
    
    int temp_next_id;
    if (fscanf(file, "%d", &temp_next_id) != 1) {
        printf(COLOR_RED "Error: Invalid file format.\n" COLOR_RESET);
        fclose(file);
        return;
    }
    next_object_id = temp_next_id;
    
    int count;
    if (fscanf(file, "%d", &count) != 1) {
        printf(COLOR_RED "Error: Invalid file format.\n" COLOR_RESET);
        fclose(file);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (i >= MAX_OBJECTS) break;
        
        GraphicObject obj;
        int type_int;
        if (fscanf(file, "%d %d", &obj.id, &type_int) != 2) {
            printf(COLOR_RED "Error: Invalid object record in file.\n" COLOR_RESET);
            break;
        }
        obj.type = (ObjectType)type_int;
        obj.is_active = 1;
        
        int parse_ok = 0;
        switch (obj.type) {
            case TYPE_LINE:
                if (fscanf(file, "%d %d %d %d", 
                           &obj.params.line.x1, &obj.params.line.y1,
                           &obj.params.line.x2, &obj.params.line.y2) == 4) {
                    parse_ok = 1;
                }
                break;
            case TYPE_RECTANGLE:
                if (fscanf(file, "%d %d %d %d", 
                           &obj.params.rect.x, &obj.params.rect.y,
                           &obj.params.rect.w, &obj.params.rect.h) == 4) {
                    parse_ok = 1;
                }
                break;
            case TYPE_TRIANGLE:
                if (fscanf(file, "%d %d %d %d %d %d", 
                           &obj.params.tri.x1, &obj.params.tri.y1,
                           &obj.params.tri.x2, &obj.params.tri.y2,
                           &obj.params.tri.x3, &obj.params.tri.y3) == 6) {
                    parse_ok = 1;
                }
                break;
            case TYPE_CIRCLE:
                if (fscanf(file, "%d %d %d", 
                           &obj.params.circle.cx, &obj.params.circle.cy,
                           &obj.params.circle.r) == 3) {
                    parse_ok = 1;
                }
                break;
        }
        
        if (parse_ok) {
            // Find slot
            objects[i] = obj;
        } else {
            printf(COLOR_RED "Error: Failed to parse object parameters.\n" COLOR_RESET);
        }
    }
    
    fclose(file);
    printf(COLOR_GREEN "Drawing loaded from %s successfully!\n" COLOR_RESET, filename);
}

int main() {
    // Initialize objects database
    for (int i = 0; i < MAX_OBJECTS; i++) {
        objects[i].is_active = 0;
    }
    
    // Add default test shapes for user onboarding
    // 1. A nice circle in the center
    objects[0].id = next_object_id++;
    objects[0].type = TYPE_CIRCLE;
    objects[0].params.circle.cx = 30;
    objects[0].params.circle.cy = 10;
    objects[0].params.circle.r = 6;
    objects[0].is_active = 1;
    
    // 2. A rectangle frame in the corner
    objects[1].id = next_object_id++;
    objects[1].type = TYPE_RECTANGLE;
    objects[1].params.rect.x = 2;
    objects[1].params.rect.y = 1;
    objects[1].params.rect.w = 12;
    objects[1].params.rect.h = 6;
    objects[1].is_active = 1;

    while (1) {
        display_canvas();
        
        printf(COLOR_CYAN "================ MENU OPTIONS ================\n" COLOR_RESET);
        printf(COLOR_BOLD "1." COLOR_RESET " Add Shape to Canvas\n");
        printf(COLOR_BOLD "2." COLOR_RESET " Modify Shape in Canvas\n");
        printf(COLOR_BOLD "3." COLOR_RESET " Delete Shape from Canvas\n");
        printf(COLOR_BOLD "4." COLOR_RESET " List Shapes Details\n");
        printf(COLOR_BOLD "5." COLOR_RESET " Save Drawing to File\n");
        printf(COLOR_BOLD "6." COLOR_RESET " Load Drawing from File\n");
        printf(COLOR_BOLD "7." COLOR_RESET " Clear All Shapes\n");
        printf(COLOR_BOLD "8." COLOR_RESET " Exit Editor\n");
        printf(COLOR_CYAN "==============================================\n" COLOR_RESET);
        
        int choice = get_input_int("Select an option (1-8): ", 1, 8);
        
        if (choice == 8) {
            printf("\nExiting. Thank you for using the 2D graphics editor!\n");
            break;
        }
        
        switch (choice) {
            case 1:
                menu_add_object();
                break;
            case 2:
                menu_modify_object();
                break;
            case 3:
                menu_delete_object();
                break;
            case 4:
                list_objects();
                printf("\nPress ENTER to return to canvas...");
                {
                    char dummy[128];
                    fgets(dummy, sizeof(dummy), stdin);
                }
                break;
            case 5:
                menu_save_to_file();
                printf("\nPress ENTER to return to canvas...");
                {
                    char dummy[128];
                    fgets(dummy, sizeof(dummy), stdin);
                }
                break;
            case 6:
                menu_load_from_file();
                printf("\nPress ENTER to return to canvas...");
                {
                    char dummy[128];
                    fgets(dummy, sizeof(dummy), stdin);
                }
                break;
            case 7:
                {
                    int confirm = get_input_int("Are you sure you want to delete ALL shapes? (1=Yes, 0=No): ", 0, 1);
                    if (confirm == 1) {
                        for (int i = 0; i < MAX_OBJECTS; i++) {
                            objects[i].is_active = 0;
                        }
                        next_object_id = 1;
                        printf(COLOR_GREEN "Canvas cleared successfully!\n" COLOR_RESET);
                    } else {
                        printf(COLOR_YELLOW "Action cancelled.\n" COLOR_RESET);
                    }
                    printf("\nPress ENTER to return to canvas...");
                    {
                        char dummy[128];
                        fgets(dummy, sizeof(dummy), stdin);
                    }
                }
                break;
        }
    }
    
    return 0;
}
