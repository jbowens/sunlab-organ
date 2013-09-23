#include<curses.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

// Constants
static const int MAX_HOSTNAME_LEN = 64;
static const char *MACHINES_SCRIPT = "/admin/consult/bin/lab-machines";

// Error messages
static const char *BAD_CHARACTER_ERROR_MSG = "Unsupported character";

// Interface elements
static const char *STR_HEADER = "The Sunlab Organ";
static const char *STR_FLAGS_HEADER = "Currently enabled flags:";

// Data type for storing information about remote children in a list.
typedef struct remote_child {
    char hostname[MAX_HOSTNAME_LEN];
    struct remote_child *next;
} remote_child;

// Header stubs
static void finish(int signal);
static void print_current_flags();
static void move_to_flags();
static void error(const char *msg);
static void start_flag(char c);
static void stop_flag(char c);
static remote_child *initialize_children();

// Currently enabled flags
bool flags[26];

int main(int argc, char **args)
{
    memset(flags, 0, 26);

    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();

    remote_child *children = initialize_children(); 
    
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

static remote_child *initialize_children()
{
    remote_child *head = 0;
    remote_child *tail = 0;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (!fork()) {
        // Child process
        close(pipefd[0]);
        // Setup the pipe as stdout
        close(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);

        // Execute the script to get a list of machines
        char *argv[0];
        char *env[0];
        if (execve(MACHINES_SCRIPT, argv, env) == -1) {
            exit(EXIT_FAILURE);
        }

    } else {
        // Lol, I'm the parent.
        // Don't need the write side of the pipe
        close(pipefd[1]);
        
        // Read the hostnames
        char buf[MAX_HOSTNAME_LEN];
        int read_ret;
        while (read(pipefd[0], &buf, MAX_HOSTNAME_LEN) > 0)
        {
            remote_child *new_child = malloc(sizeof(remote_child));
            memset(new_child, 0, sizeof(remote_child));
            strcpy(new_child->hostname, buf);

            if (!head) {
                head = new_child;
                tail = new_child;
            } else {
                tail->next = new_child;
                tail = new_child;
            }
        }

        close(pipefd[0]);
    }

    return head;
}

static void finish(int signal)
{
    endwin();

    exit(0);
}
