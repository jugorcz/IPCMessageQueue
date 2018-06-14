#include "common.h"

int server_queue_id;
extern int msgbuf_size;


typedef struct client
{
	struct client* next;
	pid_t pid;
	int queue_id;
	int request_type;
    char message[MSG_SIZE];
    bool on_list;
} client;

void create_server_queue()
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
		printf("Error: could not open message queue\n");
		exit(1);
	}
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

client* identify_client(client* first_client, msgbuf msgb)
{
	pid_t client_pid = msgb.mpid;
	int client_request = msgb.mtype;
	char client_message[MSG_SIZE];
	sprintf(client_message, "%s", msgb.mtext);


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
	new_client -> queue_id = strtol(client_message, NULL, 10);
	new_client -> request_type = client_request;
	new_client -> on_list = false;
	return new_client;
}


void send_message_to_client(client* client, char* message)
{
	msgbuf msgb;
	msgb.mtype = client -> request_type;
	msgb.mpid = client -> pid;
	sprintf(msgb.mtext, "%s", message);
	int sent_message = msgsnd(client -> queue_id, &msgb, msgbuf_size, 0);
	if (sent_message == -1)
	{
		printf("Warning: cannot send message to client %d.\n", client -> pid);
		return;
	}
}


void start_request(client* client)
{
	msgbuf msgb;
	msgb.mtype = (long)client -> request_type;
	msgb.mpid = client -> pid;
	int client_id = (getpid() + client -> pid) % 256;
	printf("ID for this client: %d\n",client_id);
	sprintf(msgb.mtext, "%d", client_id);

	int sent_message = msgsnd(client -> queue_id, &msgb, msgbuf_size, 0);
	if (sent_message == -1)
	{
		printf("Warning: cannot send message to client %d.\n", client -> pid);
		return;
	}
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

	send_message_to_client(client, new_message);
}

void time_request(client* client)
{
	time_t result = time(NULL);
	char* time_to_send = asctime(gmtime(&result));
	send_message_to_client(client, time_to_send);
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
		send_message_to_client(client, "error");
		return;
	}

	if(strcmp(component1, "ADD") == 0)
	{
		int result = add_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component1, "SUB") == 0)
	{
		int result = sub_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component1, "MUL") == 0)
	{
		int result = mul_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component1, "DIV") == 0)
	{
		int result = div_operation(component2, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component2, "+") == 0)
	{
		int result = add_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component2, "-") == 0)
	{
		int result = sub_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component2, "*") == 0)
	{
		int result = mul_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else if(strcmp(component2, "/") == 0)
	{
		int result = div_operation(component1, component3);
		char result_to_sent[5];
		sprintf(result_to_sent, "%d", result);
		send_message_to_client(client, result_to_sent);
	} 
	else
	{
		send_message_to_client(client, "Undefined operation!");
	}

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
		msgbuf msgb;
		int messege_size = msgrcv(server_queue_id, &msgb, msgbuf_size, 0, 0);

		if(messege_size == -1) 
			continue;

		client* client = identify_client(first_client, msgb);
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
			msgctl(server_queue_id, IPC_RMID, NULL);
			break;
		}
	}
	return 0;
	
}