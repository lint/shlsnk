
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

    int add_2nd_seg = 1;

    Snake *snake = malloc(sizeof(Snake));
    Segment *head = malloc(sizeof(Segment));

    head->x = COLS / 2;
    head->y = LINES / 2;
    head->prev = NULL;
    head->next = NULL;

    if (add_2nd_seg) {
        Segment *test = malloc(sizeof(Segment));

        head->next = test;
        test->prev = head;
        test->next = NULL;
        test->x = head->x;
        test->y = head->y - 1;
    }

    snake->looking = LOOKING_UP;
    snake->head = head;
    snake->tail = head;
    snake->num_segments = 1;

    return snake;
}


// adds a new segment to the snake at the given coordiantes
void extend_snake_tail(Snake *snake, int x, int y) {

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
}


// move the snake in the direction it is looking
void move_snake(Snake *snake) {

    // do not move snake if invalid data
    if (snake->looking == LOOKING_NONE || snake->head == NULL) {
        return;
    }

    int move_x = 0;
    int move_y = 0;

    // determine the coordinate adjustment based on the look direction
    switch (snake->looking) {
        case LOOKING_UP:
            move_y = -1;
            break;
        case LOOKING_RIGHT: 
            move_x = 1;
            break;
        case LOOKING_DOWN:
            move_y = 1;
            break;
        case LOOKING_LEFT:
            move_x = -1;
            break;
    }

    // define variables to store new / prev coordinates 
    int temp_x, temp_y;
    int new_x = snake->head->x + move_x;
    int new_y = snake->head->y + move_y;

    // shift coordinates down every segment
    // TODO: this is O(n), could make it better by just adding a head and popping the tail
    Segment *curr = snake->head;
    while (curr != NULL) {

        temp_x = curr->x;
        temp_y = curr->y;

        curr->x = new_x;
        curr->y = new_y;

        new_x = temp_x;
        new_y = temp_y;

        curr = curr->next;
    }

    // add a new segment to the snake
    if (float_rand(0, 1) > 0.5) {
        extend_snake_tail(snake, new_x, new_y);
    }
}


// print the snake on the window
void display_snake(Snake *snake) {

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

    fprintf(log_file,"done printing\n");

    // refresh the window
    refresh();
}


// main function
int main(int argc, char *argv[]) {

    // open log file
    log_file = fopen("log.txt", "a+");
    
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

    // display the initial snake on screen
    display_snake(snake);

    // main game loop
    while (1) {

        // get the next key press
        ch = getch();

        // update the snake's looking direction based on arrow keys
        switch (ch) {
            case KEY_UP:
                snake->looking = LOOKING_UP;
                break;
            case KEY_RIGHT:
                snake->looking = LOOKING_RIGHT;
                break;
            case KEY_DOWN:
                snake->looking = LOOKING_DOWN;
                break;
            case KEY_LEFT:
                snake->looking = LOOKING_LEFT;
                break;
            default:
                break;
        }

        // update and display the snake
        move_snake(snake);
        display_snake(snake);
    }

	endwin();
    return 0;
}