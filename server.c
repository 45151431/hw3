#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include"status.h"

#define SIZE_OF_BUFFER 128
#define SIZE_OF_STRING 100
typedef struct _DATA
{
	char domain[SIZE_OF_STRING];
	char ipv4[SIZE_OF_STRING];
}Data;
Data data[100];
int data_num = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//thread
void *connection_handler(void *);

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 12345 );

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    listen(socket_desc , 10);

    puts("Wait for connect");
    c = sizeof(struct sockaddr_in);
	
    while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
    {
        puts("Client connect");
		pthread_t thread_id;
		int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}

void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    size_t size;
    char request[SIZE_OF_BUFFER];
    char response[SIZE_OF_BUFFER];
    char method[SIZE_OF_STRING], domain[SIZE_OF_STRING], ipv4[SIZE_OF_STRING];
	int ret, check, num, n1, n2, n3, n4, i, j;

    while(1)
	{
		memset(request, '\0', sizeof(request));
		ret = read(sock, &size, sizeof(size_t));
		if(ret<=0)
			break;
		ret = read(sock, request, size);
		if(ret<=0)
			break;
		memset(method, '\0', sizeof(method));
		memset(domain, '\0', sizeof(domain));
		memset(ipv4, '\0', sizeof(ipv4));
		sscanf(request, "%s %s %s", method, domain, ipv4);
		memset(response, '\0', sizeof(response));

		pthread_mutex_lock(&mutex);
		if(strcmp(method, "SET")==0)
		{
			check = 0;
			for(j=1; j<SIZE_OF_STRING-1, domain[j]!='\0'; j++)
				if(domain[j]=='.')
					check = 1;
			if(domain[0]=='.' || domain[SIZE_OF_STRING-1]=='.')
				check = 0;
			if(check==1)
			{
				check = 0;
				num = 0;
				for(j=0; j<SIZE_OF_STRING, ipv4[j]!='\0'; j++)
				{
					if(ipv4[j]=='.')
					{
						check++;
						if(num>255)
						{
							check = 0;
							break;
						}
						num = 0;
	   				}
					else if(ipv4[j]>='0' && ipv4[j]<='9')
					{
						num *= 10;
						num += ipv4[j]-'0';
					}
					else
					{
						check = 0;
						break;
					}
				}
				if(check==3 && num<=255)
				{
					check = 0;
					for(i=0; i<data_num; i++)
						if(strcasecmp(domain, data[i].domain)==0)
						{
							check = 1;
							break;
						}
					if(check==0)
						data_num++;
					strcpy(data[i].domain, domain);
					sscanf(ipv4, "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
					sprintf(data[i].ipv4, "%d.%d.%d.%d", n1, n2, n3, n4);
					size = sprintf(response, "200 \"OK\"");
				}
				else
					size = sprintf(response, "400 \"Bad Request\"");
			}
			else
				size = sprintf(response, "400 \"Bad Request\"");
		}
		else if(strcmp(method, "GET")==0)
		{
			check = 0;
			for(j=1; j<SIZE_OF_STRING-1, domain[j]!='\0'; j++)
				if(domain[j]=='.')
					check = 1;
			if(domain[0]=='.' || domain[SIZE_OF_STRING-1]=='.')
				check = 0;
			if(check==1 && ipv4[0]=='\0')
			{
				check = 0;
				for(i=0; i<data_num; i++)
					if(strcasecmp(domain, data[i].domain)==0)
					{
						check = 1;
						break;
					}
				if(check==1)
					size = sprintf(response, "200 \"OK\" %s", data[i].ipv4);
				else
					size = sprintf(response, "404 \"Not Found\"");
	   		}
			else
				size = sprintf(response, "400 \"Bad Request\"");
		}
		else if(strcmp(method, "INFO")==0)
		{
			if(domain[0]=='\0' && ipv4[0]=='\0')
				size = sprintf(response, "200 \"OK\" %d", data_num);
			else
				size = sprintf(response, "400 \"Bad Request\"");
		}
		else
			size = sprintf(response, "405 \"Method Not Allowed\"");
		pthread_mutex_unlock(&mutex);

		ret = write(sock, &size, sizeof(size_t));
		if(ret<0)
			break;
		ret = write(sock, response, size);
		if(ret<0)
			break;
	}

	free(socket_desc);
	puts("Client leave");
	return 0;
} 
