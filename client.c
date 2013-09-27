#define _POSIX_SOURCE 1
#include<sys/types.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define BEEP_MAX_DURATION "20"
#define BEEP_EXECUTABLE "/usr/bin/beep"

typedef struct a_beep {
    int pid;
} a_beep;

a_beep *begin_beep(int freq);
void end_beep(a_beep *beep);

int main(int argc, char **argv)
{

    int flags[26];
    int buf;

    while (read(STDIN_FILENO, (char *) &buf, 4) > 0)
    {
        // Update the flags
        for (int i = 0; i < 26; i++)
            flags[i] = (1 >> i) & buf;
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

