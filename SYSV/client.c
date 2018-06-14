#include "common.h"

int server_queue_id;
int client_queue_id;
extern int msgbuf_size;

typedef struct client_request
{
	int type;
	char* content;
} client_request;

void get_server_queue_id()
{
	char* path = getenv("HOME");
	if(path == NULL)
	{
		printf("Error: could not get environment name.\n");
		exit(1);
	}

	key_t key = ftok(path, PROJECT_ID);
	if(key == -1)
	{
		printf("Error: could not convert pathname and projet id to key.\n");
		exit(1);
	}

	server_queue_id = msgget(key, IPC_CREAT | 0666);  
	if(server_queue_id == -1)
	{
		printf("Error: could not open server message queue\n");
		exit(1);
	}
}

void create_client_queue()
{
	client_queue_id = msgget(IPC_PRIVATE, 0666);
	if(client_queue_id == -1)
	{
		printf("Error: could not open message queue.\n");
		exit(1);
	}
}

void get_access_to_server()
{
	msgbuf msgb;
	msgb.mtype = (long)START;
	msgb.mpid = getpid();
	sprintf(msgb.mtext, "%d", client_queue_id);
	int sent_message = msgsnd(server_queue_id, &msgb, msgbuf_size, 0);
	if (sent_message == -1)
	{
		printf("Error: cannot send message to server.\n");
		exit(1);
	}

	int got_message = msgrcv(client_queue_id, &msgb, msgbuf_size, 0, 0);
	if(got_message == -1)
	{
		printf("Error: cannot get message prom server.\n");
		exit(1);
	}

	printf("Access to server reached. Client has ID: %s\n", msgb.mtext);
}

client_request* get_request()
{
	client_request* new_request = malloc(sizeof(client_request));

	char buffer[BUFF_SIZE];
	fgets(buffer, BUFF_SIZE, stdin);
	
	char* request_type = strtok(buffer," \t\n");
	if(request_type == NULL)
		return NULL;

	char* request_content = strtok(NULL, "\n");
	if(request_content != NULL)
	{
		new_request -> content = request_content;
	} else
	{
		new_request -> content = NULL;
	}

	if(strcmp(request_type,"START") == 0)
	{
		new_request -> type = START;
	} 
	else if(strcmp(request_type,"MIRROR") == 0)
	{
		new_request -> type = MIRROR;
	} 
	else if(strcmp(request_type,"CALC") == 0)
	{
		new_request -> type = CALC;
	} 
	else if(strcmp(request_type,"TIME") == 0)
	{
		new_request -> type = TIME;
	} 
	else if(strcmp(request_type,"STOP") == 0)
	{
		new_request -> type = STOP;
	} 
	else 
	{
		printf("Warning: command not recognized, will be skipped.\n");
		new_request -> type = UNDEFINED;
	}
	
	return new_request;
}

void send_message_to_server(msgbuf msgb)
{
	int sent_message = msgsnd(server_queue_id, &msgb, msgbuf_size, 0);
	if (sent_message == -1)
	{
		printf("Error: cannot send message %ld to server.\n",msgb.mtype);
	}
}

void receive_message()
{
	msgbuf msgb;
	int got_message = msgrcv(client_queue_id, &msgb, msgbuf_size, 0, 0);
	if(got_message == -1)
	{
		printf("Error: cannot get message prom server.\n");
		exit(1);
	} else 
	{
		printf("%s\n",msgb.mtext);
	}
}

void mirror_request(char* content)
{
	if(content == NULL)
	{
		printf("There is no content to sent to server!\n");
		return;
	}

	msgbuf msgb;
	msgb.mtype = (long)MIRROR;
	msgb.mpid = getpid();
	sprintf(msgb.mtext, "%s", content);
	send_message_to_server(msgb);
	receive_message();
}

void stop_request()
{
	msgbuf msgb;
	msgb.mtype = (long)STOP;
	msgb.mpid = getpid();
	sprintf(msgb.mtext, "%s", "");
	send_message_to_server(msgb);
	msgctl(client_queue_id , IPC_RMID, NULL);
	printf("\nClient end of work.\n");
	exit(0);
}

void time_request()
{
	msgbuf msgb;
	msgb.mtype = (long)TIME;
	msgb.mpid = getpid();
	sprintf(msgb.mtext, "%s", "");
	send_message_to_server(msgb);
	receive_message();
}

void calc_request(char* content)
{
	if(content == NULL)
	{
		printf("There is no content to sent to server!\n");
		return;
	}

	msgbuf msgb;
	msgb.mtype = (long)CALC;
	msgb.mpid = getpid();
	sprintf(msgb.mtext, "%s", content);
	send_message_to_server(msgb);
	receive_message();
}


int main(int argc, char const *argv[])
{
	signal(SIGINT, stop_request);
	signal(SIGTSTP, stop_request);

	get_server_queue_id();
	create_client_queue();
	get_access_to_server();

	while(1)
	{
		client_request* request = get_request();
		if(request == NULL)
			continue;

		switch(request -> type)
		{
			case MIRROR:
				mirror_request(request -> content);
				break;
			case CALC:
				calc_request(request -> content);
				break;
			case TIME:
				time_request();
				break;
			case STOP:
				stop_request();
				break;
			default:
				break;
		}
	}
	return 0;
}