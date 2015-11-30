/*---The following program demonstrates a gnutella like SERVENT-----*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#define MAX_CONNECTIONS 100
#define MAX_FILE_LENGTH 20
#define MAX_LINE_LENGTH 100
#define MAX_FILES 200

struct g04_header
{
	unsigned char messageid[17]; //16bytes
	unsigned char proc_id; //1 byte
	unsigned char ttl; //1byte
	unsigned char hops; //1byte
	unsigned char payload_length[5];//4 byte
};

struct g04_config
{
	int nPort;
	int fPort;
	int nop;
	int ttl;
	char seedTracker[MAX_FILE_LENGTH];
	int isSeed; //0 or 1
	char localFiles[MAX_FILE_LENGTH];
	char lfilepath[MAX_FILE_LENGTH];
};

struct nodeinf
{
	char IP[16];
	int port;
};

struct fileDB
{
	char **keywords;
	char *filename;
};

struct server_thread_args
{
	int port;
	char type;
};


//GLOBAL VARIABLES
struct g04_config* g04;
struct nodeinf seedInf[MAX_CONNECTIONS];
struct fileDB lFiles[MAX_FILES];
int connected_peers=0; 
int is_seed_connected=0; //not connected
int total_seeds = 0,present_seed=0;


//Connects to seed file and returns if success/fail
int connect_to_seed(char *IP,int port)
{
	int cl_fd,num_bytes;
	struct sockaddr_in cl_addr;
	struct hostent *client;
	char buffer[256]; 
	strcpy(buffer,"GNUTELLA CONNECT/0.4\r\n");

	if((cl_fd = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("Socket open failed in connecting to seed!");
      	return 0;
	}
	
	if((client = gethostbyname(IP))==NULL)
	{
		perror("Host not found");
      	return 0;
	}
	bzero((char *) &cl_addr, sizeof(cl_addr));
   	cl_addr.sin_family = AF_INET;
   	bcopy((char *)client->h_addr, (char *)&cl_addr.sin_addr.s_addr, client->h_length);
   	cl_addr.sin_port = htons(port);
   
    if (connect(cl_fd, (struct sockaddr*)&cl_addr, sizeof(cl_addr)) < 0) 
    {
       perror("ERROR connecting to seed!");
       return 0;
    }
    if((num_bytes = write(cl_fd, buffer, strlen(buffer)))<0)
    {
    	perror("Socket seed write failed!");
    	return 0;
    }
    bzero(buffer,256);
   	if((num_bytes = read(cl_fd, buffer, 255))<0)
   	{
      perror("ERROR reading from seed socket");
      exit(1);
   	}
   	printf("%s\n",buffer);
	return 1;
}

void *create_client()
{
	//0 FALSE, 1 TRUE
	int no_seeds_found=0;
	while(1)
	{
		//if the peer is not seed node
		if((connected_peers<g04->nop)&&(g04->isSeed==0))
		{
			//Try to connect to a seed if not
			if(present_seed<total_seeds&&is_seed_connected==0)
			{
				is_seed_connected = connect_to_seed(seedInf[present_seed].IP,seedInf[present_seed].port);
			}
			else
			{
				//Already connected, probe the seed(PINGS)

			}
		}
	}

}

char *removespaces(char *c)
{
	int end = strlen(c);
	if(isspace(c[end-1]))
	{
		c[end-1] = '\0';
	}
	return c;
}

void parseConfig(struct g04_config* g)
{
	FILE * fp;
	char *token,*value,line[MAX_LINE_LENGTH];
	char split[2] = "=";
	fp = fopen("g04.conf","r");
	if(fp == NULL)
	{
		perror("Error parsing the config file, exiting now!");
		exit(0);
	}
	else
	{
		while(fgets(line,MAX_LINE_LENGTH,fp)!=NULL)
		{
			token = NULL;
			token = strtok(line,split);
			if(token!=NULL)
			{
				value = strtok(NULL,split);
				value = removespaces(value);
			}
			if(strcmp(token,"neighbourPort")==0)
				g->nPort = atoi(value);
			else if(strcmp(token,"filePort")==0)
				g->fPort = atoi(value);
			else if(strcmp(token,"NumberOfPeers")==0)
				g->nop = atoi(value);
			else if(strcmp(token,"TTL")==0)
				g->ttl = atoi(value);
			else if(strcmp(token,"seedNodes")==0)
				strcpy(g->seedTracker,removespaces(value));
			else if(strcmp(token,"isSeedNode")==0)
			{
				if(strstr(value,"yes"))
					g->isSeed = 1;
				else if(strcmp(value,"no"))
					g->isSeed = 0;
				else
					g->isSeed = 0;
			}
			else if(strcmp(token,"localFiles")==0)
				strcpy(g->localFiles,removespaces(value));
			else if(strcmp(token,"localFilesDirectory")==0)
				strcpy(g->lfilepath,value);
			else
			{
				printf("Unkown key %s\n", token);
				exit(0);
			}
		}
	}
	fclose(fp);
}

void parseSeedFile(struct nodeinf* seedInf,char *filename)
{
	FILE * fp;
	char *token,*value,line[MAX_LINE_LENGTH];
	char split[2] = " ";
	int i=0;
	fp = fopen(filename,"r");
	if(fp == NULL)
	{
		printf("%s\n",filename);
		perror("Error parsing the seed file, exiting now!");
		exit(0);
	}
	else
	{
		while(fgets(line,MAX_LINE_LENGTH,fp)!=NULL)
		{
			token = NULL;
			token = strtok(line,split);
			if(token!=NULL)
			{
				value = strtok(NULL,split);
			}
			strcpy(seedInf[i].IP,token);
			seedInf[i].port = atoi(value);
			i++;
		}
	}
	//to mark the end of the file
	total_seeds = i-1;
	strcpy(seedInf[i].IP,"0.0.0.0");
	seedInf[i].port = -1;
	fclose(fp);
}

void parsefileDB(struct fileDB* lFiles,char *filename)
{
	FILE * fp;
	char *token,*value,line[MAX_LINE_LENGTH],*key;
	char split[2] = ":";
	int i=0,j,key_count;
	fp = fopen(filename,"r");
	if(fp == NULL)
	{
		printf("%s\n",filename);
		perror("Error parsing the localFiles file, exiting now!");
		exit(0);
	}
	else
	{
		while(fgets(line,MAX_LINE_LENGTH,fp)!=NULL)
		{
			token = NULL,key = NULL;
			token = strtok(line,split);
			if(token!=NULL)
			{
				value = strtok(NULL,split);
			}
			//printf("filename %s keywords %s\n",token,value);
			//SAVING FILENAME
			lFiles[i].filename = strdup(token);
			key_count = 0;
			for(j=0;value[j]!='\0';j++)
			{
				if(value[j]=='|')
				{
					key_count+=1;
				}
				
			}
			//printf("keycount %d\n", key_count);
			//SAVING KEYS
			lFiles[i].keywords = (char **)malloc(key_count+1);
			key = strtok(value,"|");
			for(j=0;key!=NULL;j++)
			{
				lFiles[i].keywords[j] = strdup(key);
				key = strtok(NULL,"|");
				//printf("keyword %s\t",lFiles[i].keywords[j]);
			}
			//printf("\n");
			i++;
		}
	}
	fclose(fp);
}

void printconfig(struct g04_config* g)
{
	printf("%d\n",g->nPort);
	printf("%d\n",g->fPort);
	printf("%d\n",g->nop);
	printf("%d\n",g->ttl);
	printf("%s\n",g->seedTracker);
	printf("%d\n",g->isSeed);
	printf("%s\n",g->localFiles);
	printf("%s\n",g->lfilepath);
}

//response handler for neighbour connects
void *respond_neighbour(void *tsock) {
   int n;
   char buffer[256];
   char sendmsg[] = "GNUTELLA OK\r\n";
   int sock = *(int *)tsock;
   bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) {
      perror("Socket Read Failed!");
      close(sock);
      return 0;
   }
   if((strcmp(buffer,"GNUTELLA CONNECT/0.4\r\n"))==0&&(connected_peers<g04->nop))
   	{
   		n = write(sock,sendmsg,strlen(sendmsg));
   	   	if (n < 0) {
   	      perror("ERROR writing to socket");
   	      exit(1);
   	   }
   	}
   	//close if you have enough peers
   	else
   		close(sock);
   free(tsock);
   return 0;
}

//response handler for file requests
void *respond_files(void *tsock) {
   int n;
   char buffer[256];
   char message[] = "GNUTELLA OK\r\n";
   int sock = *(int *)tsock;
   bzero(buffer,256);
   n = read(sock,buffer,255);
   
   if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }
   
   printf("Here is the message: %s\n",buffer);
   n = write(sock,message,strlen(message));
   
   if (n < 0) {
      perror("ERROR writing to socket");
      exit(1);
   }
   close(sock);
   free(tsock);
   return 0;
}

//Start a tcp server listening on some port
void *create_server(void* s_args)
{
	struct sockaddr_in saddr,caddr;
	struct server_thread_args *args = (struct server_thread_args *)s_args;
	int serversock,clientsock,retval;
	int *newsock;
	pthread_t tid;
	socklen_t clen;

	//create a socket
	serversock = socket(AF_INET,SOCK_STREAM,0);
	//To resolve bind failed and reuse an existing socket
	if (setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("SO_REUSEADDR failed!");

	if(serversock<0)
	{
		perror("Socket opening failed!");
		exit(1);
	}
	//preventing junk values in our structure
	bzero((char *)&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(args->port);

	//bind
	retval = bind(serversock, (struct sockaddr *) &saddr, sizeof(saddr));
	if(retval<0)
	{
		perror("Bind failed!");
		exit(1);
	}

	//listen
	listen(serversock,MAX_CONNECTIONS);
	clen = sizeof(caddr);
	while(1)
	{
		//bzero((char *)&caddr, sizeof(caddr));
		clientsock = accept(serversock, (struct sockaddr *) &caddr, &clen);
		if(clientsock<0)
		{
			perror("Socket opening failed!");
			exit(1);
		}

		newsock = malloc(sizeof(int));
    	*newsock = clientsock;
    	if(args->type == 'n')
    		pthread_create(&tid, NULL, respond_neighbour, newsock);
    	else if(args->type == 'f')
    		pthread_create(&tid, NULL, respond_files, newsock);
    	pthread_detach(tid);
		//close(clientsock);
	}

}

int main(void)
{
	struct server_thread_args npargs,fpargs;
	pthread_t tid1,tid2,tid3;
	char *stat1,*stat2,*stat3;
	g04 = (struct g04_config*)malloc(sizeof(struct g04_config));
	parseConfig(g04);
	printconfig(g04);
	parseSeedFile(seedInf,g04->seedTracker);
	parsefileDB(lFiles,g04->localFiles);
	npargs.port = g04->nPort;
	npargs.type = 'n';
	fpargs.port = g04->fPort;
	fpargs.type = 'f';	
	pthread_create(&tid1, NULL, create_server,(void *) &npargs);
	if(g04->isSeed==0)
		pthread_create(&tid2, NULL, create_server,(void *) &fpargs);	
	pthread_create(&tid3, NULL, create_client,NULL);
	pthread_join(tid1,(void**)&stat1);
	if(g04->isSeed==0)
		pthread_join(tid2,(void**)&stat2);
	pthread_join(tid3,(void**)&stat3);
	free(g04);
	return 0;
}