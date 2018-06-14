#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_QUEUE_NAME "/server_queue\0"
#define MSG_SIZE 32
#define BUFF_SIZE 32

typedef enum msg_type
{
	START = 1, 
	MIRROR,
	CALC,
	TIME,
	STOP,
	END,
	UNDEFINED
} msg_type;

#endif