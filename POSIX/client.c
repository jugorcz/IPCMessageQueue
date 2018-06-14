#include "common.h"

mqd_t server_queue;
mqd_t client_queue;

typedef struct client_request
{
	int type;
	char* content;
} client_request;


void get_server_queue_id()
{
	server_queue = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
	if(server_queue == -1)
	{
		printf("Error: could not open server message queue\n");
		exit(1);
	}
}

void create_client_queue()
{
	char client_queue_name[BUFF_SIZE];
	sprintf(client_queue_name, "/%d", getpid());
	printf("client queue name: %s\n",client_queue_name);

	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSG_SIZE;
	attr.mq_curmsgs = 0;
	client_queue = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0666, &attr);
	if(client_queue == -1)
	{
		printf("Error: could not open message queue.\n");
		exit(1);
	}
}


void get_access_to_server()
{
	char message[MSG_SIZE];
	sprintf(message, "%d %d %d", START, getpid(), client_queue);
	printf("first message to send: %s\n",message);
	int sent_message = mq_send(server_queue, message, MSG_SIZE, 0);
	if(sent_message == -1)
	{
		printf("Error: cannot send message to server.\n");
		exit(1);
	}

	char response[MSG_SIZE];
	int got_message = mq_receive(client_queue, response, MSG_SIZE, NULL);
	if(got_message == -1)
	{
		printf("Error: cannot get message from server.\n");
		exit(1);
	}

	char* r = strtok(response, " \t");
	r = strtok(NULL, " \t");
	r = strtok(NULL, " \t");
	printf("Access to server reached. Client has ID: %s\n", r);
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


bool send_message(char* message)
{
	int sent_request = mq_send(server_queue, message, MSG_SIZE, 0);
	if(sent_request == -1)
	{
		printf("Error: could not send message to server.\n");
		return false;
	}
	return true;
}


void get_response()
{
	char response[MSG_SIZE];
	int got_response = mq_receive(client_queue, response, MSG_SIZE, NULL);
	if(got_response == -1)
	{
		printf("Error: cannot get message from server.\n");
		return;
	
	}
	printf("Response: %s\n\n",response);
}


void mirror_request(char* content)
{
	if(content == NULL)
	{
		printf("There is no content to sent to server!\n");
		return;
	}

	char message[MSG_SIZE];
	sprintf(message, "%d %d %s", MIRROR, getpid(), content);

	if(!send_message(message))
		return;

	get_response();
}


void calc_request(char* content)
{
	if(content == NULL)
	{
		printf("There is no content to sent to server!\n");
		return;
	}

	char tmp[10];
	sprintf(tmp, "%s", content);

	char message[MSG_SIZE];
	sprintf(message, "%d %d %s", CALC, getpid(), tmp);

	if(!send_message(message))
		return;

	get_response();
	
}

void time_request()
{
	char message[MSG_SIZE];
	sprintf(message, "%d %d %s", TIME, getpid(), " ");

	if(!send_message(message))
		return;

	get_response();
}

void stop_request()
{
	char message[MSG_SIZE];
	sprintf(message, "%d %d %s", STOP, getpid(), " ");

	if(!send_message(message))
		return;

	if(mq_close(server_queue) != 0)
	{
		printf("Warning: cannot close server queue.\n");
		return;
	}

	if(mq_close(client_queue) != 0)
	{
		printf("Warning: cannot close client queue.\n");
		return;
	}

	char client_queue_name[BUFF_SIZE];
	sprintf(client_queue_name, "/%d", getpid());
	if(mq_unlink(client_queue_name) != 0)
	{
		printf("Warning: cannot delete client queue.\n");
		return;
	}

	printf("\nClient end of work.\n");
	exit(0);
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