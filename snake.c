
#include <stdlib.h>
#include <ncurses.h>

// debug log file
FILE *log_file;

// define possible look directions
typedef enum {
    LOOKING_NONE = -1,
    LOOKING_UP,
    LOOKING_RIGHT,
    LOOKING_DOWN,
    LOOKING_LEFT,
} LookDirection;

// define struct for segment of snake
typedef struct segment_t {
    int x; 
    int y;
    struct segment_t *prev;
    struct segment_t *next;
} Segment;

// define struct for snake
typedef struct snake_t {
    Segment *head;
    Segment *tail;
    LookDirection looking;
    int num_segments;
} Snake;


// helper function to generate a random float
float float_rand(float min, float max) {
    float scale = rand() / (float) RAND_MAX;
    return min + scale * (max - min);
}


// create and initialize the snake struct
Snake *init_snake() {

    int add_2nd_seg = 0;

    Snake *snake = malloc(sizeof(Snake));
    Segment *head = malloc(sizeof(Segment));

    head->x = COLS / 2;
    head->y = LINES / 2;
    head->prev = NULL;
    head->next = NULL;

    snake->looking = LOOKING_UP;
    snake->head = head;
    snake->tail = head;
    snake->num_segments = 1;

    if (add_2nd_seg) {
        Segment *tail = malloc(sizeof(Segment));

        head->next = tail;
        tail->prev = head;
        tail->next = NULL;
        tail->x = head->x;
        tail->y = head->y + 1;
        snake->num_segments += 1;
        snake->tail = tail;
    }

    return snake;
}


// free memory used by the snake
void free_snake(Snake *snake) {
    
    // free each segment
    Segment *temp;
    Segment *curr = snake->head;
    while (curr != NULL) {
        temp = curr->next;
        free(curr);
        curr = temp;
    }

    // free the snake
    free(snake);
}


// log the snake to file
void log_snake(Snake *snake) {

    fprintf(log_file, "Snake: %p\n", snake);
    fprintf(log_file, "\tlooking: %d\n", snake->looking);
    fprintf(log_file, "\tnum_segments: %d\n", snake->num_segments);
    
    Segment *curr = snake->head;
    while (curr != NULL) {
        fprintf(log_file, "\t\tsegment: (%d, %d)\n", curr->x, curr->y);
        curr = curr->next;
    }
}


// adds a new segment to the snake with the given coordiantes
void extend_snake_tail(Snake *snake, int x, int y) {

    fprintf(log_file, "extending snake (%d, %d)...\n", x, y);

    Segment *new_tail = malloc(sizeof(Segment));

    // initialize new tail struct
    new_tail->next = NULL;
    new_tail->prev = snake->tail;
    new_tail->x = x;
    new_tail->y = y;

    // update last tail and snake attributes
    snake->tail->next = new_tail;
    snake->tail = new_tail;
    snake->num_segments += 1;

    log_snake(snake);
}


// move the snake in the direction it is looking
// returns: 1 for game over, 0 for continuing
int move_snake(Snake *snake) {

    fprintf(log_file, "moving snake ...\n");

    // do not move snake if invalid data
    if (snake->looking == LOOKING_NONE || snake->head == NULL) {
        fprintf(log_file, "invalid snake, doing nothing ...\n");
        return 1;
    }

    int move_x = 0;
    int move_y = 0;

    // determine the coordinate adjustment based on the look direction
    switch (snake->looking) {
        case LOOKING_UP:
            fprintf(log_file, "looking: up\n");
            move_y = -1;
            break;
        case LOOKING_RIGHT: 
            fprintf(log_file, "looking: right\n");
            move_x = 1;
            break;
        case LOOKING_DOWN:
            fprintf(log_file, "looking: down\n");
            move_y = 1;
            break;
        case LOOKING_LEFT:
            fprintf(log_file, "looking: left\n");
            move_x = -1;
            break;
    }

    // define variables to store new / prev coordinates 
    int new_head_x = snake->head->x + move_x;
    int new_head_y = snake->head->y + move_y;
    int new_x = new_head_x;
    int new_y = new_head_y;
    int temp_x, temp_y;
    fprintf(log_file, "new x: %d, y: %d\n", new_x, new_y);

    // next coordinates intersect a segment
    int hit_segment = 0;

    // shift coordinates down every segment
    Segment *tail = snake->tail;
    Segment *curr = snake->head;
    int num_segments = snake->num_segments;
    while (curr != NULL) {

        temp_x = curr->x;
        temp_y = curr->y;

        // check if new coordinates hit a previous segment
        if (temp_x == new_head_x && temp_y == new_head_y && 
           (curr != tail || (curr == tail && num_segments < 3))) {
            hit_segment = 1;
            break;
        }

        curr->x = new_x;
        curr->y = new_y;

        new_x = temp_x;
        new_y = temp_y;

        curr = curr->next;
    }

    // the new coordinate intersected the previous segment, game over
    if (hit_segment) {
        fprintf(log_file, "GAME OVER!\n");
        return 1;
    }

    // add a new segment to the snake
    if (float_rand(0, 1) > 0.75) {
        extend_snake_tail(snake, new_x, new_y);
    }

    return 0;
}


// print the snake on the window
void display_snake(Snake *snake) {

    fprintf(log_file, "displaying snake ...\n");

    // reset the window
    clear();

    if (snake->head == NULL) {
        return;
    }

    // iterate over every segment
    Segment *curr = snake->head;
    while (curr != NULL) {

        fprintf(log_file, "printing segment: (%d, %d)\n", curr->x, curr->y);

        // attron(COLOR_PAIR(pair));
        mvprintw(curr->y, curr->x, "X");
        // attroff(COLOR_PAIR(pair));
        
        curr = curr->next;
    }

    // refresh the window
    refresh();
}


// main function
int main(int argc, char *argv[]) {

    // open log file
    log_file = fopen("log.txt", "w+");
    
    // initialize curses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // initialize colors
	// if(has_colors()) {
    //     start_color();
    //     init_pair(1, COLOR_RED, COLOR_BLACK);
    //     init_pair(2, COLOR_BLACK, COLOR_CYAN);
	// }

    // create snake struct
    Snake *snake = init_snake();
    int ch;
    int game_over = 0;

    log_snake(snake);

    // display the initial snake on screen
    display_snake(snake);

    // main game loop
    while (!game_over) {

        // get the next key press
        ch = getch();

        // update the snake's looking direction based on arrow keys
        switch (ch) {
            case 'W':
            case 'w':
            case KEY_UP:
                snake->looking = LOOKING_UP;
                break;
            case 'D':
            case 'd':
            case KEY_RIGHT:
                snake->looking = LOOKING_RIGHT;
                break;
            case 'S':
            case 's':
            case KEY_DOWN:
                snake->looking = LOOKING_DOWN;
                break;
            case 'A':
            case 'a':
            case KEY_LEFT:
                snake->looking = LOOKING_LEFT;
                break;
            case 'Q':
            case 'q':
                game_over = 1;
                break;
            default:
                break;
        }

        // update and display the snake
        game_over |= move_snake(snake);
        display_snake(snake);
    }

    free_snake(snake);
    fclose(log_file);
	endwin();

    return 0;
}
