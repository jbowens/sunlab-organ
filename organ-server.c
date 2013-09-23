#include<curses.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

static const char *BAD_CHARACTER_ERROR_MSG = "Unsupported character";

static void finish(int signal);
static void print_to_top(const char *msg);
static void print_current_notes();
static void error(const char *msg);
static void start_note(char c);
static void stop_note(char c);

bool notes[26];

int main(int argc, char **args)
{
    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();

    for (;;)
    {
        int c = getch() - 'a';
        
        if (c < 0 || c > 25) {
            error(BAD_CHARACTER_ERROR_MSG);
            continue;
        }

        if (notes[c])
            stop_note(c);
        else
            start_note(c);
    }
}

static void start_note(char c)
{
    notes[c] = 1;
    print_current_notes();
}

static void stop_note(char c)
{
    notes[c] = 0;
    print_current_notes();
}

static void print_current_notes()
{
    char buffer[27];
    memset(buffer, 0, 27);
    
    int buffer_index = 0;

    for (int i = 0; i < 26; i++)
    {
        if (notes[i]) {
            buffer[buffer_index++] = i + 'a';
        }
    }

    print_to_top(buffer);
}

static void error(const char *msg)
{
    print_to_top(msg);
}

static void print_to_top(const char *msg)
{
    move(0, 0);
    clrtoeol();
    addstr(msg);
    refresh();
}

static void finish(int signal)
{
    endwin();

    exit(0);
}
