#include "common.h"

mqd_t server_queue;

typedef struct client
{
	struct client* next;
	pid_t pid;
	int request_type;
    char message[MSG_SIZE];
    bool on_list;
} client;

void create_server_queue()
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSG_SIZE;
	attr.mq_curmsgs = 0;
	server_queue = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &attr);

	if(server_queue == -1)
	{
		printf("Error: could not create server queue.\n");
		exit(1);
	}
}


client* identify_client(client* first_client, char* message)
{

	char* type = strtok(message, " \t");
	char* pid = strtok(NULL, " \t");
	
	int client_request = strtol(type, NULL, 10);
	int client_pid = strtol(pid, NULL, 10);
	char* client_message = strtok(NULL, "");

	printf("\n---------------\nclient pid: %d  \nmessage type %d \nmessage: %s\n---------------\n", client_pid, client_request, client_message);

	client* tmp_client = first_client;
	while(tmp_client != NULL)
	{
		if(tmp_client -> pid == client_pid)
		{
			printf("\nClient on list\n");
			tmp_client -> request_type = client_request;
			sprintf(tmp_client -> message, "%s", client_message);
			tmp_client -> on_list = true;
			return tmp_client;
		}

		tmp_client = tmp_client -> next;
	}

	if(client_request != START){
		printf("\nWarning: client didn't identify himself before sending request, will be skipped.\n");
		return NULL;
	}

	client* new_client = NULL;
	new_client = malloc(sizeof(client));
	new_client -> next = NULL;
	new_client -> pid = client_pid;
	new_client -> request_type = client_request;
	new_client -> on_list = false;
	return new_client; 
}


client* add_client(client* first_client, client* new_client)
{
	printf("\nNew client added to list.\n");
	if(first_client == NULL)
	{
		first_client = new_client;
		return first_client;
	}

	client* tmp_client = first_client;
	while(tmp_client -> next != NULL)
		tmp_client = tmp_client -> next;

	tmp_client -> next = new_client;
	new_client -> next = NULL;
	return first_client;
}


bool send_message(client* client, char* message)
{
	char client_queue_name[BUFF_SIZE];
	sprintf(client_queue_name, "/%d", client -> pid);

	mqd_t client_queue = mq_open(client_queue_name, O_WRONLY);
	if(client_queue == -1)
	{
		printf("Error: could not open client %d queue.\n", client -> pid);
		return false;
	}

	int sent_response = mq_send(client_queue, message, MSG_SIZE, 0);
	if(sent_response == -1)
	{
		printf("Error: could not send messahe to client %d queue.\n", client -> pid);
		return false;
	}

	int closed_queue = mq_close(client_queue);
	if(closed_queue == -1)
	{
		printf("Error: could not close client %d queue.\n", client -> pid);
		return false;
	}

	return true;
}

void start_request(client* client)
{
	char message[MSG_SIZE];
	int client_id = (getpid() + client -> pid) % 256;
	printf("ID for this client: %d\n",client_id);
	sprintf(message, "%d %d %d", START, client -> pid, client_id);

	send_message(client, message);
}

void mirror_request(client* client)
{
	int message_size = strlen(client -> message);
	char* new_message = malloc(sizeof(char) * message_size);
	for(int i = message_size - 1; i >= 0; i--)
	{
		char tmp[1];
		tmp[0] = client -> message[i];
		strcat(new_message, tmp);
	}

	send_message(client, new_message);
}

int add_operation(char* a, char* b)
{
	int aa = strtol(a, NULL, 10);
	int bb = strtol(b, NULL, 10);
	return aa + bb;
}

int sub_operation(char* a, char* b)
{
	int aa = strtol(a, NULL, 10);
	int bb = strtol(b, NULL, 10);
	return aa - bb;
}

int mul_operation(char* a, char* b)
{
	int aa = strtol(a, NULL, 10);
	int bb = strtol(b, NULL, 10);
	return aa * bb;
}

int div_operation(char* a, char* b)
{
	int aa = strtol(a, NULL, 10);
	int bb = strtol(b, NULL, 10);
	if(bb == 0) return -1;
	return aa / bb;
}

void calc_request(client* client)
{
	char* component1 = strtok(client -> message," \t\n");
	char* component2 = strtok(NULL, " \t\n");
	char* component3 = strtok(NULL, " \t\n");

	if(component1 == NULL || component2 == NULL || component3 == NULL)
	{
		send_message(client, "error");
		return;
	}

	if(strcmp(component1, "ADD") == 0)
	{
		int result = add_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component1, "SUB") == 0)
	{
		int result = sub_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component1, "MUL") == 0)
	{
		int result = mul_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component1, "DIV") == 0)
	{
		int result = div_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component2, "+") == 0)
	{
		int result = add_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component2, "-") == 0)
	{
		int result = sub_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component2, "*") == 0)
	{
		int result = mul_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else if(strcmp(component2, "/") == 0)
	{
		int result = div_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message(client, result_to_sent);
	} 
	else
	{
		send_message(client, "Undefined operation!");
	}

}

void time_request(client* client)
{
	time_t result = time(NULL);
	char* time_to_send = asctime(gmtime(&result));

	char message[MSG_SIZE];
	sprintf(message, "%s", time_to_send);
	send_message(client, message);
}


client* remove_client(client* first_client, client* client_to_remove)
{
	if(first_client -> pid == client_to_remove -> pid)
	{
		first_client = first_client -> next;
		client_to_remove -> next = NULL;
		free(client_to_remove);
		return first_client;
	}

	client* tmp = first_client;
	while(tmp -> next != NULL)
	{
		if(tmp -> next -> pid == client_to_remove -> pid)
		{
			tmp -> next = client_to_remove -> next;
			client_to_remove -> next = NULL;
			free(client_to_remove);
			break;
		}
		tmp = tmp -> next;
	}

	return first_client;
}


int main(int argc, char const *argv[])
{
	srand(time(NULL));
	create_server_queue();
	client* first_client = NULL;

	while(1)
	{
		char message[MSG_SIZE];
		int got_message = mq_receive(server_queue, message, MSG_SIZE, NULL);
		if(got_message == -1)
			continue;

		client* client = identify_client(first_client, message);
		if(client == NULL)
			continue;
		if(client -> on_list == false)
			first_client = add_client(first_client, client);

		switch(client -> request_type)
		{
			case START:
				start_request(client);
				break;
			case MIRROR:
				mirror_request(client);
				break;
			case CALC:
				calc_request(client);
				break;
			case TIME:
				time_request(client);
				break;
			case STOP:
				first_client = remove_client(first_client, client);
				printf("\nClient removed from list.\n");
				break;
			default:
				break;
		}

		if(first_client == NULL)
		{ 	
			printf("\nThere is no clients, end of server work.\n");
			mq_close(server_queue);
			mq_unlink(SERVER_QUEUE_NAME);
			break;
		}
	}
	return 0;
	
}