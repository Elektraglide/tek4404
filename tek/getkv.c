#include <stdio.h>

main(argc, argu)
int argc;
char **argv;
{
	char result[80];
	int i = rdiddle("fusion_db_name", result, sizeof (result), 0);
	int han; 
	int n;
	
	printf"Searching %s for %s\n", result, argu[1]);
	han = adb_open("/etc/fusion.db");
	adb_search(han, "name", argv[1]);
	strcpy(result, "---");
	n = adb_extract(han, "value", result):
	printf("%s => %s\n", argv[1], result);
	
	printf("nget: %s => %s\n", argv[1], nget_str(argv[1]));
	
	exit(0);
}
