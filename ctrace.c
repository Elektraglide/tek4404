#include<stdio.h>
#include <sys/signal.h>
#include <sys/ctask.h>
#include <sys/fcntl.h>
#include "kernutils.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

char *defaultprocess[] = {"/tek/forever", NULL};	/* somthing that sits there doing stuff */

int done = 0;

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
	unsigned char mem[32];
	unsigned int special[32];
	struct ctask *task;
	char **tracee;

	/* what to trace */
	tracee = defaultprocess;
	if (argc > 1)
	{
		tracee = argv + 1;
	}

	task = create_controlled_task();
	printtask("create",task);
	
	/* do parent processing */
	if (task->task_fd != 0)
	{
		printf("parent wakes\n");
		signal(SIGDEAD, childreap);

		/* this makes this process show up in status as "trce"  Why? */
/*		signal(SIGTRACE, childstop);
		signal(SIGTRAP6, childstop);
*/


		done = 0;
		while(!done)
		{
			/* execute until breakpoint */
		execute_controlled_task(task); 
/*		    step_controlled_task(task); 	*/

			printtask("PAUSE",task);
			if (done)
				break;

			printf("***********\n");
				i = (task->task_state >> 24);
				if (i)
				{
					printf(" signal(%d)\n", i);
			/*  NB documented as being called clear_controlled_task_signals() */
			/* clear_controlled_task(task); */
				}
				else
				{
					task->task_PC = 0;
					get_controlled_task_registers(task);
					printf("task_PC %8.8x   D0:%8.8x D1:%8.8x\n", task->task_PC, task->task_REGS[0],task->task_REGS[1]);

					/* NB undocumented parameters */
					get_controlled_task_memory(task, task->task_PC, mem, sizeof(mem));
					printf("memory %2.2x %2.2x %2.2x %2.2x\n", mem[0],mem[1],mem[2],mem[3]);
				}
			printf("***********\n");


/*			resume_controlled_task(task);	*/
			printtask("resumed",task);

		}
	}
	else
	{
	
			signal(SIGTRACE, childstop);
			
			signal(SIGTRAPV, childstop);
			signal(SIGTRAP1, childstop);
			signal(SIGTRAP2, childstop);
			signal(SIGTRAP3, childstop);
			signal(SIGTRAP4, childstop);
			signal(SIGTRAP5, childstop);
			signal(SIGTRAP6, childstop);

			signal(SIGUSR1, childstop);
			signal(SIGUSR2, childstop);
			signal(SIGUSR3, childstop);

			kill(getpid(), SIGTRACE);		/* this arrives at PARENT! */
			
			printf("child doing execvp(%s)\n",tracee[0]);

      /* launch cmd */
			rc = execvp(tracee[0], tracee);
      if (rc < 0)
      {
        fprintf(stderr, "execvp: %s\n", strerror(errno));
      }
	}
	
	return (int)task;
}

