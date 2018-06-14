#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define PROJECT_ID 7
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

typedef struct msgbuf
{
	long mtype;  
	char mtext[MSG_SIZE];
	pid_t mpid;
} msgbuf;

int msgbuf_size = sizeof(msgbuf) - sizeof(long);

#endif