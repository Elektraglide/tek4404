#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>

extern int read();

int main(argc, argv)
int argc;
char **argv;
{
    int pipe_fd[2],reader,writer;
    char *fname;
    FILE *logFile;
	  int pid;
    char ch;

    fname = argv[1];
    logFile = fname ? fopen(fname,"a"): NULL;
    if(fname && !logFile)
        fprintf(stderr,"cannot open log file \"%s\": %d (%s)\n",fname,errno,strerror(errno));

    while(read(fileno(stdin),&ch,1) > 0)
    {
        putchar(ch);
        if(logFile)
            fputc(ch,logFile);
        if('\n' == ch)
        {
            fflush(stdout);
            if(logFile)
                fflush(logFile);
        }
    }
    putchar('\n');
    if(logFile)
        fclose(logFile);

    return(0);
}
