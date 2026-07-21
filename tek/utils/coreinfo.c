#include <stdio.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <time.h>
#include "kernutils.h"
#include "ph.h"
#include "uniflex/task.h"
#include "uniflex/userbl.h"

#ifdef __clang__
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netinet/ip.h>
#else
#include <net/in.h>
#endif

/* S_IFDIR block table, 32 per block */
struct dirblk {
	unsigned short d_fdn;
	char d_name[14];
};

struct filedes {
	unsigned short s1,s2;
	ptr32 bufferlist;
	ptr32 bufferlist2;
};

int header[1024];

int stackoffset(sp,map)
int sp;
struct mt *map;
{
	return sp - ntohl(map[2].vaddr);
}

#define MC_DIRTY 0x80000000
#define MC_WRITE 0x40000000
#define MC_SWAP 0x20000000
#define MC_SHARED 0x10000000
#define MC_PAGE 0x000fffff

void printpages(map_cell_pages, len)
unsigned int *map_cell_pages;
int len;
{
int i;
char c1,c2,c3;

	for(i=0; i<len/4; i++)
	{
		if(map_cell_pages[i])
		{
			unsigned int v = ntohl(map_cell_pages[i]);
			c1 = '.';
			c2 = '.';
			c3 = '.';
			if (v & MC_DIRTY)
				c1 = 'D';
			if (v & MC_WRITE)
				c2 = 'W';
			if (v & MC_SHARED)
				c3 = 'S';

			printf("[%c%c%c]",c1,c2,c3);
		}
	}
	printf("\n");
}

/* parsing core files base on reverse engineering function 0xa814 which dumps a core file for kernel */
int main(argc,argv)
int argc;
char **argv;
{
	int fd;
	int i;

	fd = open(argc > 1 ? argv[1] : "core", O_RDONLY);
	if (fd > 0)
	{
		struct userbl* userbl;
		struct task atask;
		struct mt* map;
		unsigned int* regs;
		struct filedes afile;
		unsigned char buffer[128];
		unsigned short utmat[128];
		unsigned int map_cell_pages[1024];
		char *status;
		unsigned long timestamp;
		struct dirblk *fdn;
		time_t t;

		/* first 4k */
		read(fd, header, sizeof(header));

		userbl = (struct userbl *)(((char*)header) + ntohl(header[0]));

		i = 1 + ntohs(userbl->usizet) + ntohs(userbl->usized) + ntohs(userbl->usizes);
		lseek(fd, i * 4096, SEEK_SET);
		read(fd, &atask, sizeof(atask));
		printf("PROC:\n");
		t = ntohl(userbl->ustart);
#ifdef __clang__
		// 10 years of seconds; UniFLEX epoch starts 1980 not 1970!
		
#define SECONDS_IN_DAY (60*60*24)
#define NUM_OF_LEAP_DAYS_IN_10_YEARS 2
		t += (int)(SECONDS_IN_DAY * 365 * 10 + SECONDS_IN_DAY * NUM_OF_LEAP_DAYS_IN_10_YEARS);
#endif
		status = ctime(&t);
#ifndef __clang__
        /* UniFlex adds CR! */
		status[24] = '\0';
#endif		
		printf("started: %s\n", status);

		printf("utimu(%d) utims(%d)\n", ntohl(userbl->utimu),ntohl(userbl->utims));

		printf("uid(%d) uuid(%d) pid(%d) ppid(%d)\n", ntohs(atask.tsuid), ntohs(userbl->uuid), ntohs(atask.tstid),ntohs(atask.tstidp));

		status = "";
		if (atask.tsstat == TRUN)  status = "RUN";
		if (atask.tsstat == TSLEEP) status = "SLEEP";
		if (atask.tsstat == TWAIT) status = "WAIT";
		if (atask.tsstat == TTERM) status = "TERM";
		if (atask.tsstat == TTRACE) status = "TRACE";
		printf("tsstat(%d) %s\n", atask.tsstat, status);

		buffer[0] = '\0';
		if (atask.tsmode & TCORE) strcat(buffer, "CORE ");
		if (atask.tsmode & TLOCK) strcat(buffer, "LOCK ");
		if (atask.tsmode & TSYSTM) strcat(buffer, "SYSTM ");
		if (atask.tsmode & TTRACP) strcat(buffer, "TRACE ");
		if (atask.tsmode & TSWAPO) strcat(buffer, "SWAP ");
		if (atask.tsmode & TARGX) strcat(buffer, "ARGX ");
		printf("tsmode(%d) %s\n", atask.tsmode, buffer);
		fdn = (struct dirblk *)userbl->ufdn;
		printf("cwd entry: \"%s\"\n", fdn->d_name);
		printf("bin flags: 0x%4.4x\n", ntohs(userbl->ubin_flags));
		printf("uhltpri value: %d\n", ntohs(userbl->uhltpri));


		/* reading VM page map? */
		printf("PAGE MAP:\n");
		{
			read(fd, &utmat, 0x1a);
			printf("umemc %d => 0x%X\n", ntohs(utmat[9]), 0x80 << (ntohs(utmat[9]) & 0x3f));
			/* based on task size, writes more or less */
			i = ntohs(utmat[9]);
			read(fd, map_cell_pages, 0x80 << (i & 0x3f));
			printpages(map_cell_pages, 0x80 << (i & 0x3f));
			read(fd, map_cell_pages, 0x80 << (i & 0x3f));
		}

		/* reading open files */
		printf("FILES:\n");
		{
			for (i = 0; i < UNFILS; i++)
			{
				if (userbl->ufiles[i])
				{
					/* TODO: interpret / reverse engineer file descriptor entry */

					read(fd, &afile, sizeof(afile));
					printf("    %d: 0x%4.4x 0x%4.4x  0x%8.8x  0x%8.8x\n", i,
						ntohs(afile.s1), ntohs(afile.s2), ntohl(afile.bufferlist), ntohl(afile.bufferlist2));

					read(fd, utmat, 0x5e);
				}
			}
		}


		printf("MAP:\n");
		{
			map = (struct mt*)userbl->umem;
			while (ntohs(map->numpages) > 0)
			{
				printf("    %8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
				map++;
			}
			if (userbl->udep_chunks > 0)
			{
				printf("EXTENDED MAP:\n");
				map = (struct mt*)userbl->umdep_segs;
				while (ntohs(map->numpages) > 0)
				{
					printf("    %8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
					map++;
				}
			}
		}

		/* relies on userbl being on a 4k boundary.. */
		printf("REGISTERS:\n");
		{
			regs = (unsigned int*)((char*)header + (ntohl(userbl->uregs) & 0xfff));
			printf("    Dn ");
			for (i = 0; i < 8; i++)
				printf("%8.8x ", ntohl(regs[i]));
			printf("\n");
			printf("    An ");
			for (i = 0; i < 8; i++)
				printf("%8.8x ", ntohl(regs[8 + i]));
			printf("\n");
		}

		printf("STACKTRACE:\n");
		{
			int argc, argv, envp;
			int sp;
			unsigned int spoff;
			unsigned char* stack;

			i = 1 + ntohs(userbl->usizet) + ntohs(userbl->usized);
			lseek(fd, i * 4096, SEEK_SET);
			stack = (char*)malloc(4096 * ntohs(userbl->usizes));
			read(fd, stack, 4096 * ntohs(userbl->usizes));

			sp = ntohl(regs[15]);
			map = (struct mt*)userbl->umem;
			i = 0;
			while (sp)
			{
				/* TODO: lookup addr from cmd executable symbol table */

				/* SP, Return Addr, locals */
				spoff = stackoffset(sp, map);
				printf("    %d: 0x%8.8x\n", i++, ntohl(*(int*)(stack + spoff + 4)));
				sp = ntohl(*(int*)(stack + spoff));
			}

			spoff += 8;
			argc = ntohl(*(int*)(stack + spoff));
			argv = ntohl(*(int*)(stack + spoff + 4));
			envp = ntohl(*(int*)(stack + spoff + 8));

			/* walk argv array */
			printf("CMDLINE: ");
			for (i = 0; i < argc; i++)
			{
				char* p = ntohl(*(int*)(stack + stackoffset(argv, map)));
				printf("%s ", stack + stackoffset(p, map));

				argv += 4;
			}
			printf("\n");

			printf("ENVIRON:\n");
			while (envp)
			{
				char* p;

				spoff = stackoffset(envp, map);
				p = ntohl(*(int*)(stack + spoff));
				if (!p)
					break;

				printf("    %s\n", stack + stackoffset(p, map));

				envp += 4;
			}
		}

		close(fd);
	}
}

