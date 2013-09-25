#include<curses.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>

// Constants
#define MAX_HOSTNAME_LEN    64
#define MACHINES_SCRIPT "/admin/consult/bin/lab-machines"
#define SSH_LOCATION "/usr/bin/ssh"
#define CLIENT_EXECUTABLE "/home/jbowens/scratch/sunlab-organ/organ-client"

// Error messages
static const char *BAD_CHARACTER_ERROR_MSG = "Unsupported character";

// Interface elements
static const char *STR_HEADER = "The Sunlab Organ v2 (with more bells... and more whistles)";
static const char *STR_FLAGS_HEADER = "Currently enabled flags:";

// Data type for storing information about remote children in a list.
typedef struct remote_child {
    char hostname[MAX_HOSTNAME_LEN];
    int pid;
    int read_fd;
    int write_fd;
    struct remote_child *next;
} remote_child;

// Header stubs
static void finish(int signal);
static void print_current_flags();
static void move_to_flags();
static void error(const char *msg);
static void start_flag(char c);
static void stop_flag(char c);
static void initialize_children();
static void launch_clients();
static void broadcast_state();

// Currently enabled flags
bool flags[26];

// Remote children
remote_child *child_head = 0;
int machine_count = 0;

int main(int argc, char **args)
{
    // Initialize all the flags to off
    memset(flags, 0, 26);
    
    // Get a list of sunlab machines, and initialize the data structures for
    // keeping track of them.
    initialize_children(); 

    // Launch clients on all the machines
    launch_clients();

    getchar();

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

static void initialize_children()
{
    remote_child *head = 0;
    remote_child *tail = 0;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (!pid) {
        // I'm the child process
        close(pipefd[0]);
        // Setup the pipe as stdout
        close(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);

        char * const argv[] = { "/admin/consult/bin/lab-machines", NULL };
        char * const envp[] = { NULL };

        // Execute the script to get a list of machines
        if (execve(MACHINES_SCRIPT, argv, envp) == -1) {
            perror("execve");
            exit(EXIT_FAILURE);
        }
    } else {
        // I'm the parent process.
        // Don't need the write side of the pipe
        close(pipefd[1]);
        
        // Read the hostnames all into one big buffer. Memory is cheap, right?
        char buf[1024];
        memset(buf, 0, 1024);
        if(read(pipefd[0], buf, 1023) < 0) {
            printf("Unable to read machine hostnames\n");
            exit(EXIT_FAILURE);
        }

        // Loop through the hostnames and create remote clients for each
        char *current_hostname = buf;
        while (current_hostname[0] != '\0')
        {
            char hostname[MAX_HOSTNAME_LEN];
            memset(hostname, 0, MAX_HOSTNAME_LEN);

            char *next_newline = strchr(current_hostname, '\n');

            if (next_newline == NULL) {
                strcpy(hostname, current_hostname);
            } else {
                strncpy(hostname, current_hostname, next_newline - current_hostname);
            }

            // Prepare current_hostname for the next iteration
            current_hostname = next_newline + 1;

            if (hostname[0] == 'm') {
                // Skip ms lab machines
                continue;
            }

            remote_child *new_child = malloc(sizeof(remote_child));
            memset(new_child, 0, sizeof(remote_child));
            strcpy(new_child->hostname, hostname);

            machine_count++;
            if (!head) {
                head = new_child;
                tail = new_child;
            } else {
                tail->next = new_child;
                tail = new_child;
            }
        }

        close(pipefd[0]);

        // Wait for our child process to die
        waitpid(pid, 0, 0);
    }

    child_head = head;
}

static void launch_clients()
{
    remote_child *curr_head = child_head;

    while (curr_head)
    {
        // We can communicate with the client over ssh through a pipe! :D 
        if (pipe(&curr_head->read_fd) == -1) {
            perror("client pipe");
            exit(EXIT_FAILURE);
        }

        // Fork a child process
        int pid = fork();

        if (pid < 0) {
            perror("client fork");
            exit(EXIT_FAILURE);
        }

        if (!pid) {
            // This is the child process. Make this process ssh into the machine
            // Don't need the write portion of the pipe.
            close(curr_head->write_fd);
            // Make the pipe be our new stdin
            close(STDIN_FILENO);
            dup2(curr_head->read_fd, STDIN_FILENO);
            // SSH into the client and run the client executable
            char * const argv[] = { SSH_LOCATION, curr_head->hostname, CLIENT_EXECUTABLE, NULL };
            char * const envp[] = { NULL };
            if (execve(argv[0], argv, envp) < 0) {
                perror("ssh client");
                exit(EXIT_FAILURE);
            }
        } else {
            // This is the parent process.
            // Don't need the read portion of the pipe.
            close(curr_head->read_fd);
        }

        curr_head->pid = pid;

        curr_head = curr_head->next;
    }
}

/* Broadcast the current state of the flags to all the clients
 * so that they know what the fuck is up, when it is up.
 */
static void broadcast_state()
{
    remote_child *client = child_head;

    // Create a bit mask of all the flags that are enabled. Since a 32bit int is less
    // than 26, we shouldn't run into any problems storing it like this.
    int bitmap = 0;
    for (int i = 0; i < 26; i++)
    {
        if (flags[i])
            bitmap |= (1 >> i);
    }

    while (client)
    {
        // Write exactly 4 bytes, the bitmap, to the client.
        write(client->write_fd, (char *) &bitmap, 4);
        
        client = client->next;
    }
}

// TODO: Actually connect this sig handler
static void finish(int signal)
{
    endwin();

    // Destroy all the remote children
    while (child_head)
    {
        remote_child *next = child_head->next;

        // Kill the pipe connection, telling the client to exit
        close(child_head->write_fd);
        // Wait for the client to exit cleanly
        waitpid(child_head->pid, 0, 0);

        free(child_head);
        child_head = next;
    }

    exit(0);
}