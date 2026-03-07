#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <locale.h>
#include <wchar.h>
#include <getopt.h>

/* --- USER DEFINABLE PARAMETERS --- */

#define DELTA_TIME 0.5f             // DT for simulations.
#define FRAME_TIME 100000           // Time between frames (in microseconds).
#define SNOW_INCREMENT 0.2f         // Increment constant for fallen snow.
#define MAX_GROUND_LEVEL 4.0f       // Preferred max height of snow layer.

int flakes_count = 200;
bool do_draw_frame = TRUE;

/* --- PROGRAM VARIABLES --- */

int term_width, term_height;
int canv_width, canv_height;
int canv_offset;

int old_width = 0, old_height = 0;

float *snow_levels;                 // Individual snow columns.
float snow_levels_max = 0.0f;       // Current highest snow column.

float elapsed_time = 0.0f;          // Used for wind simulation.

WINDOW *frame_win = NULL, *canvas_win = NULL;

// NOTE: when a flake's y coordinate is 0 it gets rendered at the bottom
//  of the window (it has reached the ground)...
typedef struct Flake {
    float x, y;
    float vy;
} Flake;

Flake *flakes;

// Write a wide char on a window.
void writewch(WINDOW* win, int y, int x, wchar_t c)
{
    cchar_t block;
    setcchar(&block, &c, 0, 0, NULL);
    mvwadd_wch(win, y, x, &block);
}

int get_random_num(int min, int max)
{
    // 'min' and 'max' are inclusive.
    return (rand() % (max - min + 1)) + min;
}

/* --- UPDATE FUNCTIONS --- */

Flake generate_flake()
{
    return (Flake) {
        get_random_num(-10, canv_width + 9), 
        get_random_num(canv_height, canv_height + 10),
        get_random_num(1, 4) };
}

void init_snow()
{
    Flake *new_flakes = malloc(flakes_count * sizeof(Flake));
    free(flakes);
    flakes = new_flakes;

    srand(time(NULL));
    for(int i = 0; i < flakes_count; i++)
        flakes[i] = generate_flake();
}

void resize_ground()
{
    float *new_levels = malloc(canv_width * sizeof(float));
    free(snow_levels);
    snow_levels = new_levels;

    for(int i = 0; i < canv_width; i++)
        snow_levels[i] = 0.0f;
}

bool update_dimensions()
{
    getmaxyx(stdscr, term_height, term_width); 
    
    canv_offset = (do_draw_frame) ? 1 : 0;
    canv_width  = term_width  - 2 * canv_offset;
    canv_height = term_height - 2 * canv_offset;

    // Returns true if dimensions have changed.
    // NOTE: returns false at the start of the program to inizialise everything.
    return (term_width != old_width || term_height != old_height);
}

// Update snow columns dynamically.
// This function prevents the formation of uneven snow towers.
//  It simulates the effect of weight by letting snow fall from the tallest
//  columns.
void update_ground()
{
    for(int i = 0; i < canv_width; i++)
    {
        // If the current column is 0.4 higher compared to it's neighbouring ones
        //  each column gets 0.1 while 0.2 is removed from the current one.
        if(i > 0 && i < (canv_width - 1) &&
            (snow_levels[i] - snow_levels[i - 1]) > 0.4f &&
            (snow_levels[i] - snow_levels[i + 1]) > 0.4f)
        {
            snow_levels[i] -= 0.2f;
            snow_levels[i - 1] += 0.1f;
            snow_levels[i + 1] += 0.1f;
        }

        // If the current column is 0.4 higher compared to the last one
        //  that one gets 0.2 while 0.2 is removed from the current one.
        if(i > 0 && 
            (snow_levels[i] - snow_levels[i - 1]) > 0.4f)
        {
            snow_levels[i] -= 0.2f;
            snow_levels[i - 1] += 0.2f;
        }

        // If the current column is 0.4 higher compared to the next one
        //  that one gets 0.2 while 0.2 is removed from the current one.
        if(i < (canv_width - 1) && 
            (snow_levels[i] - snow_levels[i + 1]) > 0.4f)
        {
            snow_levels[i] -= 0.2f;
            snow_levels[i + 1] += 0.2f;
        }

        if(snow_levels_max < snow_levels[i])
            snow_levels_max = snow_levels[i];
    }
}

float get_wind_velocity(float delta)
{
    float new_velocity = sinf(elapsed_time / 10.0f);
    
    // 'elapsed_time' gets reset every 20*pi so that it cannot possibily
    //  overflow...
    elapsed_time += delta;
    if(elapsed_time >= 62.83f)
        elapsed_time = 0.0f;

    return new_velocity;
}

void update_snow(float delta)
{
    float wind_velocity = get_wind_velocity(delta);

    for(int i = 0; i < flakes_count; i++)
    {
        Flake flake = flakes[i];

        int x = (int)flake.x;
        int y = (int)flake.y;

        int level = ceil(snow_levels[x]) + 1;

        if(x >= 0 && x < canv_width && y <= level)
        {
            snow_levels[x] += SNOW_INCREMENT;
            flakes[i] = generate_flake();
        } else if(y < 0)
        {
            // the flake landed outside of the canvas, 
            //  do not update 'snow_levels'.
            flakes[i] = generate_flake();
        }

        flakes[i].y -= flakes[i].vy * delta;
        flakes[i].x += wind_velocity * delta;
    }
}

/* --- WINDOWS FUNCTIONS --- */

void draw_snow(WINDOW *win)
{
    for(int i = 0; i < flakes_count; i++)
    {
        Flake flake = flakes[i];
        int x = (int)flake.x, y = (int)flake.y;
        
        int render_y = canv_height - y;
        int level = ceil(snow_levels[x]);

        if(x >= 0 && x < canv_width && 
            render_y >= 0 && render_y < canv_height && 
            y > level)
        {
            mvwaddch(win, render_y, x, '*');
        }
    }
}

void draw_ground(WINDOW *win)
{
    for(int i = 0; i < canv_width; i++)
    {
        float level = snow_levels[i];
        int integer = (int)level;
        int decimal = (int)((level - integer) * 10);

        for(int y = 0; y < integer; y++)
            writewch(win, canv_height - y - 1, i, L'\u2588');

        if(decimal != 0)
        {
            // 0.1 (1) = 1/8 full block ---> 0.9 (9) = 7/8 full block.
            int offset = (int)(decimal * (6.0f / 9.0f));
            writewch(win, canv_height - integer - 1, i, L'\u2581' + offset);
        }
    }
}

WINDOW *create_frame()
{
    WINDOW *frame_win = newwin(term_height, term_width, 0, 0);

    return frame_win;
}

WINDOW *create_canvas()
{
    WINDOW *canvas_win = newwin(canv_height, canv_width, 
                                canv_offset, canv_offset);
    
    return canvas_win;
}

/* --- UTILITY FUNCTIONS --- */

float get_max_level()
{
    float max = snow_levels[0];
    for(int i = 1; i < canv_width; i++)
        if(snow_levels[i] > max) max = snow_levels[i];
    return max;
}

void handle_resize()
{
    init_snow();
    resize_ground();

    // Used to have a memory leak without these lines...
    delwin(canvas_win);
    delwin(frame_win);

    canvas_win = create_canvas();
    frame_win = create_frame();
}

void level_ground()
{
    for(int i = 0; i < canv_width; i++)
       if(snow_levels[i] >= (snow_levels_max - 1.0f))
            snow_levels[i] = fmax(0.2f, snow_levels[i] - 1.0f);

    // Update the highest column (DO NOT REMOVE: snow disappears if this line is
    //  commented out)...
    snow_levels_max = get_max_level();
}

// Returns true if all arguments have been parsed correctly.
bool parse_args(int argc, char *argv[])
{
    struct option long_options[] = {
        {"help",            no_argument,        0,  'h' },
        {"no-frame",        no_argument,        0,  'f' },
        {"flakes-count",    required_argument,  0,  'c' },
        {0,                 0,                  0,  0   }
    };

    int opt;
    while((opt = getopt_long(argc, argv, "hfc:", long_options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'h':
                fprintf(stderr, "Usage: %s [--no-frame] [--flakes-count count]\n", argv[0]);
                return false;
            case 'f':
                do_draw_frame = FALSE;
                break;
            case 'c':
                char *endptr;
                long val = strtol(optarg, &endptr, 10);

                if(*endptr != '\0')
                {
                    fprintf(stderr, "Invalid integer: %s\n", optarg);
                    return false;
                }

                if(val < 0)
                {
                    fprintf(stderr, "Invalid value (must be greater or equal to 0).\n");
                    return false;
                }

                flakes_count = (int)val;
                break;
            case '?':
                return false;
        }
    }

    return true;
}

/* --- MAIN FUNCTION --- */

int main(int argc, char *argv[])
{
    if(!parse_args(argc, argv))
        return EXIT_FAILURE;

    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);
    nodelay(stdscr, TRUE);

    /* --- MAIN ANIMATION LOOP --- */

    while(1) {
        if(getch() == 'q')
            break;
        
        if(update_dimensions())
            handle_resize();

        /* --- UPDATE AND RENDER --- */

        werase(canvas_win);

        if(do_draw_frame)
            box(frame_win, 0, 0);

        update_snow(DELTA_TIME);
        update_ground();

        draw_ground(canvas_win);
        draw_snow(canvas_win);
       
        wnoutrefresh(frame_win);
        wnoutrefresh(canvas_win);
        doupdate();

        if(snow_levels_max >= MAX_GROUND_LEVEL)
            level_ground();
        
        old_width = term_width; old_height = term_height;
        usleep(FRAME_TIME);
    }

    endwin();

    return EXIT_SUCCESS;
}