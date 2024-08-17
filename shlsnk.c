
#define _XOPEN_SOURCE_EXTENDED

#include <stdlib.h>
// #include <ncurses.h>
#include <ncursesw/curses.h>
#include <locale.h>
#include <wchar.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// debug log file
FILE *log_file;

// block wide characters
wchar_t full_block[] = { L'\u2588', L'\0' };
wchar_t lower_half_block[] = { L'\u2584', L'\0' };
wchar_t upper_half_block[] = { L'\u2580', L'\0' };

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


// helper function to generate a random int
int int_rand(int min, int max) {
    return (rand() % (max - min + 1)) + min;
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
void extend_snake_tail(Snake *snake, int iterations) {

    fprintf(log_file, "extending snake (x%d)...\n", iterations);

    for (int i = 0; i < iterations; i++) {
        Segment *new_tail = malloc(sizeof(Segment));

        // initialize new tail struct
        new_tail->next = NULL;
        new_tail->prev = snake->tail;
        new_tail->x = snake->tail->x;
        new_tail->y = snake->tail->y;

        // update last tail and snake attributes
        snake->tail->next = new_tail;
        snake->tail = new_tail;
        snake->num_segments += 1;

        log_snake(snake);
    }
}


// determines if a given coordinate is covered by the snake
// returns: 1 if snake is at coords, 0 if not
int is_snake_at_coords(Snake *snake, int x, int y) {

    // iterate over every segment of the snake
    Segment *curr = snake->head;
    while (curr != NULL) {

        // check if the current coordinates match
        if (curr->x == x && curr->y == y) {
            return 1;
        }

        curr = curr->next;
    }

    return 0;
}


// move the snake in the direction it is looking
void move_snake(Snake *snake, int food_x, int food_y, int *game_over, int *should_gen_food, int *should_extend_snake) {

    fprintf(log_file, "moving snake ...\n");

    // do not move snake if invalid data
    if (snake->looking == LOOKING_NONE || snake->head == NULL) {
        fprintf(log_file, "invalid snake, exit ...\n");
        *game_over = 1;
        return;
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

    // collided with wall or segment
    int snake_collided = 0;

    // shift coordinates down every segment
    Segment *tail = snake->tail;
    Segment *curr = snake->head;
    int num_segments = snake->num_segments;
    while (curr != NULL) {

        temp_x = curr->x;
        temp_y = curr->y;

        // check if new coordinates hit a previous segment or collided with a wall
        if ((temp_x == new_head_x && temp_y == new_head_y && 
           (curr != tail || (curr == tail && num_segments < 3))) || 
           (new_x <= 0 || new_x >= COLS - 1 || new_y <= 0 || new_y >= LINES - 1)) {
            snake_collided = 1;
            break;
        }

        curr->x = new_x;
        curr->y = new_y;

        new_x = temp_x;
        new_y = temp_y;

        curr = curr->next;
    }

    // the new coordinate intersected the previous segment or wall, game over
    if (snake_collided) {
        fprintf(log_file, "GAME OVER!\n");
        *game_over = 1;
    }

    // check if the new head position ate the food
    if (new_head_x == food_x && new_head_y == food_y) {
        
        // set flags to generate new food coordinates and extend snake
        *should_gen_food = 1;
        *should_extend_snake = 1;
    }
}


// generate new food coordinates
// returns: 1 if successfully generated coordinates, 0 if failure (all spots are snake)
int gen_food_coords(Snake *snake, int *food_x, int *food_y) {

    int x, y;
    int valid = 0;

    // randomly generate x and y coordinates until valid one is found
    for (int tries = 0; tries < 1000; tries++) {
        
        x = int_rand(1, COLS-2);
        y = int_rand(1, LINES-2);

        fprintf(log_file, "random try: %d, (%d, %d)\n", tries, x, y);

        // found coordinate not covered by the snake
        if (!is_snake_at_coords(snake, x, y)) {
            fprintf(log_file, "\tfound valid food coords! (random)\n");
            valid = 1;
            break;
        }
    }

    // random coordinate not found
    if (!valid) {

        // iterate over every possible coordinate
        for (y = 1; !valid && y < LINES-1; y++) {
            for (x = 1; x < COLS-1; x++) {

                // found coordinate not covered by the snake
                if (!is_snake_at_coords(snake, x, y)) {
                    fprintf(log_file, "\tfound valid food coords! (linear)\n");
                    valid = 1;
                    break;
                }
            }
        }
    }

    // set x and y to -1 if no valid coordinates found
    if (!valid) {
        x = -1;
        y = -1;
    }

    // set food x and y
    *food_x = x;
    *food_y = y;

    return valid;
}


// check if food coords are out of bounds
void check_food_bounds(int food_x, int food_y, int *should_gen_food) {

    if (food_x <= 0 || food_x >= COLS - 1 || food_y <= 0 || food_y >= LINES - 1) {
        *should_gen_food = 1;
    }
}


// check if game was won
int did_win_snake(Snake *snake) {

    // game is won if every valid game tile is covered by the snake
    for (int y = 1; y < LINES - 1; y++) {
        for (int x = 1; x < COLS - 1; x++) {

            if (!is_snake_at_coords(snake, x, y)) {
                return 0;
            }
        }
    }

    return 1;
}


// print the snake on the window
void print_snake(Snake *snake) {

    fprintf(log_file, "displaying snake ...\n");

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
}


// print the food on the window
void print_food(int x, int y) {
    mvprintw(y, x, "o");
}


// print the score on the window
void print_score(Snake *snake) {
    mvprintw(0, 0, "SCORE: %d", snake->num_segments);
}


// print the walls on the window
void print_walls() {

    // print vertical walls
    for (int y = 0; y < LINES; y++) {
        mvaddwstr(y, 0, full_block);
        mvaddwstr(y, COLS-1, full_block);
    }

    // print horizontal walls
    for (int x = 0; x < COLS; x++) {
        mvaddwstr(0, x, lower_half_block);
        mvaddwstr(LINES-1, x, upper_half_block);
    }
}


// print the game on the window
void print_game(Snake *snake, int food_x, int food_y) {

    // clear the previous screen
    erase();

    // call various game print methods
    print_walls();
    print_score(snake);
    print_food(food_x, food_y);
    print_snake(snake);

    // display the changes to the window
    refresh();
}


// print the game over screen
void print_game_over(Snake *snake) {

    // clear the previous screen
    erase();

    // define game over score strings
    char *win_str = "*** YOU WON :) CONGRATULATIONS!!! ***";
    char *game_over_str = "GAME OVER!";
    char *score_part_str = "SCORE:";
    char *retry_str = "RETRY? (Y/N)";

    int win_str_len = strlen(win_str);
    int game_over_len = strlen(game_over_str);
    int score_part_len = strlen(score_part_str) + floor(log10(abs(snake->num_segments))) + 1;
    int retry_len = strlen(retry_str);

    // print the strings on screen
    mvprintw(MAX(LINES/2 - 2, 1), (COLS-game_over_len)/2, "%s", game_over_str);
    mvprintw(MAX(LINES/2 - 1, 2), (COLS-score_part_len)/2, "%s %d", score_part_str, snake->num_segments);
    mvprintw(MAX(LINES/2 + 1, 3), (COLS-retry_len)/2, "%s", retry_str);

    // only print win text if actually won
    if (did_win_snake(snake)) {
        mvprintw(MAX(LINES/2 - 6, 0), (COLS-win_str_len)/2, "%s", win_str);
    }

    // refresh the window
    refresh();
}


// contains loop that runs the game
int play_snake() {

    // initialize data structures and variables
    Snake *snake = init_snake();
    struct timeval time_start, time_end; 
    double frame_time_duration = (1./3) * 1000000L; 
    int game_over = 0;
    int should_gen_food = 0;
    int should_extend_snake = 0;
    int food_x, food_y;
    int ch;

    // generate initial food coords
    gen_food_coords(snake, &food_x, &food_y);

    // log the initial snake to file
    log_snake(snake);

    // display the initial state of the game
    print_game(snake, food_x, food_y);

    // main game loop
    while (!game_over) {

        // get the time the frame started
        gettimeofday(&time_start, NULL);

        // update and display the snake
        move_snake(snake, food_x, food_y, &game_over, &should_gen_food, &should_extend_snake);
        print_game(snake, food_x, food_y);

        // check if food is out of bounds
        check_food_bounds(food_x, food_y, &should_gen_food);

        // break immediately if game over
        if (game_over) {
            break;
        }

        // check if should generate new food coordinates
        if (should_gen_food) {
            should_gen_food = 0;
            gen_food_coords(snake, &food_x, &food_y);
        }

        // check if should extend snake
        if (should_extend_snake) {
            should_extend_snake = 0;
            extend_snake_tail(snake, 1);
        }

        // get the time the frame ended
        gettimeofday(&time_end, NULL);
        double frame_time_left = frame_time_duration - ((time_end.tv_sec - time_start.tv_sec) * 1000000L + time_end.tv_usec - time_start.tv_usec);

        // use rest of frame time to read input characters
        do {
            
            // get start time
            gettimeofday(&time_start, NULL);

            // get the next key press
            ch = getch();

            // update the snake's looking direction based on arrow keys
            switch (ch) {
                case 'W':
                case 'w':
                case KEY_UP:
                    if (snake->looking != LOOKING_DOWN || snake->num_segments <= 1) {
                        snake->looking = LOOKING_UP;
                    }
                    break;
                case 'D':
                case 'd':
                case KEY_RIGHT:
                    if (snake->looking != LOOKING_LEFT || snake->num_segments <= 1) {
                        snake->looking = LOOKING_RIGHT;
                    }
                    break;
                case 'S':
                case 's':
                case KEY_DOWN:
                    if (snake->looking != LOOKING_UP || snake->num_segments <= 1) {
                        snake->looking = LOOKING_DOWN;
                    }
                    break;
                case 'A':
                case 'a':
                case KEY_LEFT:
                    if (snake->looking != LOOKING_RIGHT || snake->num_segments <= 1) {
                        snake->looking = LOOKING_LEFT;
                    }
                    break;
                case 'Q':
                case 'q':
                    game_over = 1;
                    break;
                case 'R':
                case 'r':
                    free_snake(snake);
                    snake = init_snake();
                    gen_food_coords(snake, &food_x, &food_y);
                    // TODO: maybe put reset confirmation y/n?
                    break;
                default:
                    break;
            }

            // get duration of iteration and subtract from remaining frame time
            gettimeofday(&time_end, NULL);
            frame_time_left -= (time_end.tv_sec - time_start.tv_sec) * 1000000L + time_end.tv_usec - time_start.tv_usec;

        } while (frame_time_left > 0);
    }

    // print the final score
    print_game_over(snake);

    int retry = -1;
    while (retry < 0) {

        ch = getch();
        switch (ch) {
            case 'Y':
            case 'y':
                retry = 1;
                break;
            case 'N':
            case 'n':
            case 'Q':
            case 'q':
                retry = 0;
                break;
            default:
                break;
        }
    }

    // cleanup memory
    free_snake(snake);

    // return retry result
    return retry;
}


// start the game
void start_game() {

    // retry snake game until quit
    int retry = 1;
    while (retry) {
        retry = play_snake();
    }
}


// main function
int main(int argc, char *argv[]) {

    // open log file
    log_file = fopen("log.txt", "w+");
    
    // initialize curses
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, 1);

    // initialize colors
	// if(has_colors()) {
    //     start_color();
    //     init_pair(1, COLOR_RED, COLOR_BLACK);
    //     init_pair(2, COLOR_BLACK, COLOR_CYAN);
	// }

    // start the game
    start_game();

    // cleanup and exit
    fclose(log_file);
	endwin();

    return 0;
}
