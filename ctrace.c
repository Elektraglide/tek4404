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

unsigned char dottext[4096];
patchtext(exename, task)
char *exename;
struct ctask *task;
{
PH ph;
int fd;
int i,j;
int pcount;

	fd = open(exename,0);
	read(fd, &ph, sizeof(ph));
	close(fd);
	printf(".text start = %8.8x  length = %8.8x\n", ph.textstart,ph.textsize);

	pcount = 0;	
	i = 0;
	while(i < ph.textsize)
	{
		int start,len,icount;
		unsigned short *instr;

		/* grab a 4k block of text */						
		start = ph.textstart + i;
		len = sizeof(dottext);
		if (i + len > ph.textsize)
			len = ph.textsize - i;	
		get_controlled_task_memory(task, start, dottext, len);
		if (verbose) printf("fetched %d bytes\n", len);
		
		instr = (unsigned char *)dottext;
		icount = len >> 1;	/* 16-bit words */
		j = 0;
		while(j < icount)
		{
			static int extwords[] = {1,1,1,1,1,2,2,2,3,2,2,2,0};
			int opcode = instr[j];
			int code = (opcode >> 12);
			int mode = (opcode & 0x38) >> 3;
			int reg = (opcode & 0x07);

			if (verbose)			
				printf("%8.8x: %4.4x code=%d mode=%d reg=%d \n", start+j*2, opcode, code, mode, reg);
			
			if (opcode == TRAP15)
			{
				instr[j] = ILLEGAL;
	        	update_controlled_task_memory(task, start+j*2, dottext+j*2,2);
	        	if (verbose) printf("patched %8.8x\n", start+j*2); 
	        	pcount++;
			}

j++;
#if 0
			/* how many extension words for this instruction? */
			if (mode == 7)
				mode += reg;
			j += extwords[mode];
			if (extwords[mode] > 1 && (opcode & 0x80))
				j++;
#endif				
		}

		i += len;
	}
	printf("patched %d kernel calls\n", pcount);
}

char *gettrapname(code, arglist)
unsigned short code;
paramtype *arglist;
{
	static char buffer[32];
	char *name;

		switch(code)
		{
			case 4:
				name = "wait";
				break;
			case 5:
				done = 1;			/* terminate parent */
				name = "term";
				break;
			case 6:
				name = "sbrk";
				arglist[0] = VALUE;
				break;
			case 7:
				name = "stack";
				arglist[0] = A0;
				break;
			case 8:
				name = "cpint";
				arglist[0] = VALUE;
				arglist[1] = POINTER;
				break;
			case 9:
				name = "spint";
				arglist[0] = D0;
				arglist[1] = VALUE;
				break;
			case 10:
				name = "open";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 11:
				name = "create";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 12:
				name = "read";
				arglist[0] = D0;
				arglist[1] = POINTER;
				arglist[2] = VALUE;
				break;
			case 13:
				name = "write";
				arglist[0] = D0;
				arglist[1] = POINTER;
				arglist[2] = VALUE;
				break;
			case 14:
				name = "seek";
				arglist[0] = D0;
				arglist[1] = VALUE;
				arglist[2] = VALUE;
				break;
			case 15:
				name = "close";
				arglist[0] = D0;
				break;
			case 16:
				name = "dup";
				arglist[0] = D0;
				break;
			case 17:
				name = "dup2";
				arglist[0] = D0;
				break;
			case 19:
				name = "unlink";
				arglist[0] = STRINGPTR;
				break;
			case 21:
				name = "chdir";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 22:
				name = "lock";
				arglist[0] = D0;
				break;
			case 23:
				name = "chown";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 24:
				name = "chprm";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 25:
				name = "chacc";
				arglist[0] = STRINGPTR;
				arglist[1] = VALUE;
				break;
			case 26:
				name = "defacc";
				arglist[0] = D0;
				break;
			case 27:
				name = "ofstat";
				arglist[0] = D0;
				break;
			case 28:
				name = "status";
				arglist[0] = STRINGPTR;
				arglist[1] = POINTER;
				break;
			case 32:
				name = "gtid";
				break;
			case 33:
				name = "guid";
				break;
			case 34:
				name = "suid";
				arglist[0] = D0;
				break;
			case 35:
				name = "setpr";
				arglist[0] = D0;
				break;
			case 39:
				name = "time";
				arglist[0] = POINTER;
				break;
			case 40:
				name = "stime";
				arglist[0] = D0;
				break;
			case 41:
				name = "ttime";
				arglist[0] = POINTER;
				break;
			case 42:
				name = "update";
				break;
			case 43:
				name = "alarm";
				arglist[0] = D0;
				break;
			case 44:
				name = "stop";
				break;
			case 45:
				name = "ttyget";
				arglist[0] = D0;
				arglist[1] = POINTER;
				break;
			case 46:
				name = "ttyset";
				arglist[0] = D0;
				arglist[1] = POINTER;
				break;
			case 49:
				name = "systat";
				arglist[0] = POINTER;
				break;
			case 51:
				name = "ttynum";
				break;
			case 53:
				name = "truncate";
				arglist[0] = D0;
				break;
			case 54:
				name = "phys";
				arglist[0] = VALUE;
				break;
			case 64:
				name = "make_realtime (unimplemented)";
				break;
			case 65:
				name = "control_pty";
				arglist[0] = VALUE;
				arglist[1] = VALUE;
				break;
			default:
				sprintf(buffer,"trap%d", code);
				name = buffer;
				break;
		}

	return name;	
}

void printargs(task, name, arglist, params, epilog)
struct ctask *task;
char *name;
paramtype *arglist;
unsigned short *params;
char *epilog;
{
	unsigned int arg0;
	unsigned int *argptr;
	char stringarg[32];
	int p;

	printf("%s(", name);
	argptr = (unsigned int *)(params + 1);
	for(p=0; p<3; p++)
	{
		if (arglist[p] == NONE)
			break;

		if (p)
			putchar(',');

		switch(arglist[p])
		{
			case VALUE:
				printf("%d", *argptr++);
				break;				
			case POINTER:
				printf("%8.8x", *argptr++);
				break;				
			case STRINGPTR:
				arg0 = *argptr++;
				get_controlled_task_memory(task, arg0, stringarg, sizeof(stringarg));
				printf("\"%s\"", stringarg);
				break;
			case D0:
				printf("%d", task->task_REGS[0]);
				break;
			case A0:
				printf("%8.8x", task->task_REGS[8]);
				break;
		}
	}
	printf(epilog);
}

void executetrap(task)
struct ctask *task;
{
unsigned short mem[8];
unsigned int faultPC;
paramtype arglist[4];
char *name;
				
	/* NB undocumented parameters */
	get_controlled_task_registers(task);
	get_controlled_task_memory(task, task->task_PC, mem, sizeof(mem));
	if (mem[0] != ILLEGAL)
	{
		printf("-- %8.8x: %4.4x\n", task->task_PC, mem[0]);
		return;
	}
	
	printf("%8.8x: ", task->task_PC);

	arglist[0] = NONE;
	arglist[1] = NONE;
	arglist[2] = NONE;
	arglist[3] = NONE;
	if (mem[1] == 0)	/* use inline pointer for args */
	{
		unsigned short params[8];
		unsigned int argptr;
		
		argptr = *(unsigned int *)(mem + 2);
		get_controlled_task_memory(task, argptr, params, sizeof(params));
		name = gettrapname(params[0],arglist);
		printargs(task, name, arglist, params, ") ind");
	}
	else
	if (mem[1] == 1)	/* use A0 for args */
	{
		unsigned short params[8];
												
		get_controlled_task_memory(task, task->task_REGS[8], params, sizeof(params));
		name = gettrapname(params[0],arglist);
        printargs(task, name, arglist, params, ") indx");
	}
	else				/* use inline mem for args */
	{
		name = gettrapname(mem[1], arglist);
		printargs(task, name, arglist, mem + 1, ")");
	}	

    /* terminating tasks and ctask do not mix! */
    if(!done)
    {
	  /* install trap instr */
	  faultPC = task->task_PC;
	  mem[0] = TRAP15;
   	  update_controlled_task_memory(task, faultPC,mem, sizeof(mem));

	  /* execute trap */
	  task->task_PC -= 2;	/* FIXME: how much to backstep? */
	  update_controlled_task_registers(task);
      step_controlled_task(task); 

      /* get returned result */
	  get_controlled_task_registers(task);
    
	  /* re-install break instr */
  	  mem[0] = ILLEGAL;
   	  update_controlled_task_memory(task, faultPC,mem, sizeof(mem));
   	}

    printf(" => %d\n", task->task_REGS[0]);
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
		
		/* patch trap instr with ILLEGAL */
		patchtext(tracee[0], task);

		printf("tracing...\n");
		done = 0;
		while(!done)
		{
			/* execute until breakpoint */
			execute_controlled_task(task); 

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
				executetrap(task);
			}

			if (done)
				break;

			resume_controlled_task(task);  
			clear_controlled_task(task);

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

