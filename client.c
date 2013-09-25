#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

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
