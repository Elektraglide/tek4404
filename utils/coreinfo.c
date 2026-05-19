#include <stdio.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include "kernutils.h"
#include "ph.h"
#include "uniflex/task.h"
#include "uniflex/userbl.h"

int header[1024];

int main(argc,argv)
int argc;
char **argv;
{
	int fd;
	int i;

	fd = open("core", O_RDONLY);
	if (fd > 0)
	{
		struct userbl* userbl;
		struct task* task;
		struct mt* map;
		unsigned int* regs;

		/* first 4k */
		read(fd, header, sizeof(header));

		userbl = (struct userbl *)(((char*)header) + header[0]);

		printf("utimu  %8.8x\n", userbl->utimu);
		printf("uregs  %8.8x\n", userbl->uregs);
		printf("utask  %8.8x\n", userbl->utask);

		map = (struct mt *)userbl->umem;
		while (map->numpages > 0)
		{
			printf("%8.8x - %8.8x\n", map->vaddr, map->vaddr + map->numpages * 4096 - 1);
			map++;
		}

		regs = (unsigned int *)((char*)userbl - 66 - (16 * 4));
		printf("Dn ");
		for (i = 0; i < 8; i++)
			printf("%8.8x ", regs[i]);
		printf("\n");
		printf("An ");
		for (i = 0; i < 8; i++)
			printf("%8.8x ", regs[8+i]);
		printf("\n");

printf("offset = %8.8x\n", header[0] + (char *)&userbl->usizet - (char *)userbl);

printf("%d %d %d\n",userbl->usizet, userbl->usized, userbl->usizes);

		i = 1 + userbl->usizet + userbl->usized + userbl->usizes;
		lseek(fd, i * 4096, SEEK_SET);
		lseek(fd, 4, SEEK_CUR);
		read(fd, &task, sizeof(task));
		printf("tsuid  %d\n", task->tsuid);
		printf("tstid  %d\n", task->tstid);
		printf("tstidp  %d\n", task->tstidp);
		printf("tsalrm  %d\n", task->tsalrm);
		printf("tsdatp  %d\n", task->tsdatp);
		printf("tsstkp  %d\n", task->tsstkp);

		printf("tsstat  %d\n", task->tsstat);
	}
}

