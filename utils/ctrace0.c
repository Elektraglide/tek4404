#include<stdio.h>
#include <sys/signal.h>
#include <sys/ctask.h>
#include <sys/fcntl.h>
#include "kernutils.h"
#include "ph.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TRAP15 0x4e4f
#define ILLEGAL 0x4afc

typedef enum {
	NONE,
	VALUE,
	POINTER,
	STRINGPTR,
	D0,
	A0
} paramtype;

char *defaultprocess[] = {"/tek/forever", NULL};	/* somthing that sits there doing stuff */
char *cmdoptions[16];
int done = 0;
int verbose = 0;

void childreap(sig)
int sig;
{
	done = 1;

	printf("SIGDEAD\n");
}

void childstop(sig)
int sig;
{

	printf("******* SIGNAL %d *******\n", sig);
	
	signal(sig, childstop);
}

void printtask(msg, task)
char *msg;
struct ctask *task;
{
	int pid = getpid();
	
  printf("------ %s (%d) -------\n",msg, pid);
/***
  printf("task_id    %d\n", task->task_id);
  printf("task_fd    %d\n", task->task_fd);
  printf("task_state %8.8x\n", task->task_state);
  printf("task_flags %8.8x\n", task->task_flags);
  printf("task_control %8.8x\n", task->task_control);
***/  
}

int main(argc, argv)
int argc;
char **argv;
{
	int i,rc;
	unsigned char mem[8];
	unsigned int special[32];
	struct ctask *task;
	char **tracee;
	
	/* what to trace */
	tracee = defaultprocess;
	if (argc > 1)
	{
		for (i=1; i<argc; i++)
			cmdoptions[i-1] = argv[i];
		cmdoptions[i-1] = NULL;
		
		tracee = cmdoptions;
	}

	/* does it exist? */
	rc = open(tracee[0], 0);
	if (rc < 0)
	{
		fprintf(stderr, "cannot open: %s\n", tracee[0]);
		exit(1);	
	}
	close(rc);

	task = create_controlled_task();

	/* do parent processing */
	if (task->task_fd != 0)
	{
		signal(SIGDEAD, childreap);

		/* allow child to start */
		sleep(3);

		/* allow child to start */
	    step_controlled_task(task);  
		get_controlled_task_registers(task);
		
		printf("tracing...\n");
		done = 0;
		while(!done)
		{
			/* execute until breakpoint */
			execute_controlled_task(task); 

/*			get_controlled_task_registers(task); */
			i = (task->task_state >> 24);
			if (i)
			{
				printf(" signal(%d)\n", i);
				/*  NB documented as being called clear_controlled_task_signals() */
				/* clearing this NOW allows child process to immediate continue.. */
				/* clear_controlled_task(task);  */
			}
			else
			{

			}

			if (done)
				break;

			resume_controlled_task(task);  
/*			clear_controlled_task(task);  */

		}
	}
	else
	{	
			printf("tracing child: execvp(%s)\n",tracee[0]);

			/* launch cmd */
			rc = execvp(tracee[0], tracee);

      if (rc < 0)
      {
        fprintf(stderr, "execvp: %s\n", strerror(errno));
      }
	}
	
	return (int)task;
}

