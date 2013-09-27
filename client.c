#define _POSIX_SOURCE 1
#include<sys/types.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define FLAG_COUNT 26
#define BEEP_MAX_DURATION "30000"
#define BEEP_EXECUTABLE "/usr/bin/beep"

typedef struct a_beep {
    int pid;
} a_beep;

a_beep *begin_beep(int freq);
void end_beep(a_beep *beep);
int get_freq(int *flags);

a_beep *current_beep = 0;

void sighandler(int sig)
{
    // Clean up after ourselves if we get cancelled.
    if (current_beep)
        end_beep(current_beep);
}

int main(int argc, char **argv)
{
    signal(SIGTERM, sighandler);
    signal(SIGINT , sighandler);

    int flags[FLAG_COUNT];
    int buf = 0;

    while (read(STDIN_FILENO, (char *) &buf, 4) > 0)
    {
        // Update the flags
        int a_flag_down = 0;
        for (int i = 0; i < FLAG_COUNT; i++) {
            flags[i] = (1 << i) & buf;
            if (flags[i])
                a_flag_down = 1;
        }
        
        if (current_beep) {
            end_beep(current_beep);
            current_beep = 0;
        }

        if (a_flag_down) {
            // SOME flag is down.
            int freq = get_freq(flags);
            current_beep = begin_beep(freq); 
        }
    }

    if (current_beep) {
        end_beep(current_beep);
        current_beep = 0;
    }

}

a_beep *begin_beep(int freq)
{
    a_beep *beep = malloc(sizeof(a_beep));
    memset(beep, 0, sizeof(a_beep));

    // TODO: Could avoid forking and just move writing beeps to the tty
    // into this file.
    
    int pid = fork();

    if (pid < 0) {
        free(beep);
        perror("fork");
        return NULL;
    }

    if (!pid) {
        // Create a string from the frequency
        char freq_str[32];
        sprintf(freq_str, "%d", freq);

        // Child process, run beep!
        char * const argv[] = { BEEP_EXECUTABLE, "-l", BEEP_MAX_DURATION, "-f", freq_str };
        char * const envp[] = { NULL };
        if (execve(BEEP_EXECUTABLE, argv, envp) == -1) {
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    beep->pid = pid;
    return beep;
}

void end_beep(a_beep *beep)
{
    kill(beep->pid, SIGINT);
    free(beep);
}

int get_freq(int *flags)
{
    if (flags['a'-'a'])
        return 440;
    else if (flags['b'-'a'])
        return 494;
    else if(flags['c'-'a'])
        return 262;
    else if(flags['d'-'a'])
        return 294;
    else if(flags['e'-'a'])
        return 330;
    else if(flags['f'-'a'])
        return 349;
    else if(flags['g'-'a'])
        return 392;
}

