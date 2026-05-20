#include <stdio.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include "kernutils.h"
#include "ph.h"
#include "uniflex/task.h"
#include "uniflex/userbl.h"

#ifdef __clang__
#include <unistd.h>
#include <string.h>
#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#include <sys/_types/_u_int.h>
#include <netinet/ip.h>
#else


#endif


/* S_IFDIR block table, 32 per block */
struct dirblk {
	unsigned short d_fdn;
	char d_name[14];
};

struct filedes {
	ushort s1,s2;
	ptr32 bufferlist;
	ushort s3,s4;
};

int header[1024];

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
		unsigned char buffer[128],utmat[128];
		char *status;
		unsigned long timestamp;
		
		/* first 4k */
		read(fd, header, sizeof(header));

		userbl = (struct userbl *)(((char*)header) + ntohl(header[0]));

	timestamp = userbl->ustart;
	printf("task start: %2.2d:%2.2d:%2.2d \n", (timestamp/3600), (timestamp%3600) / 60, (timestamp%3600) % 60);

		i = 1 + ntohs(userbl->usizet) + ntohs(userbl->usized) + ntohs(userbl->usizes);
		lseek(fd, i * 4096, SEEK_SET);
		read(fd, &atask, sizeof(atask));
				
		// reading VM page map?
		read(fd, &utmat, 0x1a);
		printf("umemc %d\n", ntohs(userbl->umemc));
		// based on task size, writes more or less
		i = ntohs(userbl->umemc) ;
		read(fd, buffer, 0x80 << (i & 0x3f));
		read(fd, buffer, 0x80 << (i & 0x3f));

		// reading open files
		printf("files:\n");
		for(i=0; i<UNFILS; i++)
		{
			if (userbl->ufiles[i])
			{
					read(fd, &afile, sizeof(afile));
					printf("%d: 0x%4.4x 0x%4.4x  0x%8.8x\n", i, ntohs(afile.s1),ntohs(afile.s2),ntohl(afile.bufferlist));
					
					read(fd, buffer, 0x5e);
			}
		}
		
		printf("process:\n");
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
		printf("utimu(%d) utims(%d)\n", ntohl(userbl->utimu),ntohl(userbl->utims));

		
		printf("map:\n");
		map = (struct mt *)userbl->umem;
		while (ntohs(map->numpages) > 0)
		{
			printf("%8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
			map++;
		}
		if (userbl->udep_chunks > 0)
		{
			printf("extended map:\n");
			map = (struct mt *)userbl->umdep_segs;
			while (ntohs(map->numpages) > 0)
			{
				printf("%8.8x - %8.8x\n", ntohl(map->vaddr), ntohl(map->vaddr) + ntohs(map->numpages) * 4096 - 1);
				map++;
			}
		}

		/* relies on userbl being on a 4k boundary.. */
		printf("registers:\n");
		regs =  (unsigned int *)((char*)header + (ntohl(userbl->uregs) & 0xfff));
		printf("Dn ");
		for (i = 0; i < 8; i++)
			printf("%8.8x ", ntohl(regs[i]));
		printf("\n");
		printf("An ");
		for (i = 0; i < 8; i++)
			printf("%8.8x ", ntohl(regs[8+i]));
		printf("\n");

		printf("bin flags: 0x%4.4x\n", ntohs(userbl->ubin_flags));
		
		struct dirblk *fdn = (struct dirblk *)userbl->ufdn;
		printf("cwd entry: \"%s\"\n", fdn->d_name);
		
		close(fd);
	}
}

