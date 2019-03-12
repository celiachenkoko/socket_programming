#ifndef __MYMALLOC_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAX_HOP 512
struct _potato_t{
	int trace[MAX_HOP];
	int hopnum;
 int curr;
};
typedef struct _potato_t potato_t;

struct _player_info{
	int playerID;
	int connect_fd;
	int listen;
	char name[255];
};
typedef struct _player_info player_info;
#endif