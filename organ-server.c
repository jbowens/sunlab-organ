#include<curses.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

// Error messages
static const char *BAD_CHARACTER_ERROR_MSG = "Unsupported character";

// Interface elements
static const char *STR_HEADER = "The Sunlab Organ";
static const char *STR_FLAGS_HEADER = "Currently enabled flags:";

static void finish(int signal);
static void print_current_flags();
static void move_to_flags();
static void error(const char *msg);
static void start_flag(char c);
static void stop_flag(char c);

// Currently enabled flags
bool flags[26];

int main(int argc, char **args)
{
    memset(flags, 0, 26);

    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();
    
    // Make the cursor invisible
    curs_set(0);

    // Print static information
    move(0, 0);
    addstr(STR_HEADER);
    move(1,0);
    addstr(STR_FLAGS_HEADER);
    move_to_flags();
    refresh();

    for (;;)
    {
        int c = getch() - 'a';
        
        if (c < 0 || c > 25) {
            error(BAD_CHARACTER_ERROR_MSG);
            continue;
        }

        if (flags[c])
            stop_flag(c);
        else
            start_flag(c);
    }
}

static void start_flag(char c)
{
    flags[c] = 1;
    print_current_flags();
}

static void stop_flag(char c)
{
    flags[c] = 0;
    print_current_flags();
}

static void print_current_flags()
{
    char buffer[27];
    memset(buffer, 0, 27);
    
    int buffer_index = 0;

    for (int i = 0; i < 26; i++)
    {
        if (flags[i]) {
            buffer[buffer_index++] = i + 'a';
        }
    }

    move_to_flags();
    clrtoeol();
    addstr(buffer);
    refresh();
}

static void move_to_flags()
{
    move(2, 0);
}

static void error(const char *msg)
{
    move(4, 0);
    clrtoeol();
    addstr(msg);
    print_current_flags();
    // TODO: Add timer for a timeout on the message.
}

static void finish(int signal)
{
    endwin();

    exit(0);
}
