/*---The following program demonstrates a gnutella like SERVENT-----*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#define MAX_CONNECTIONS 100
#define MAX_FILE_LENGTH 20
#define MAX_LINE_LENGTH 100

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
				strcpy(g->seedTracker,value);
			else if(strcmp(token,"isSeedNode")==0)
			{
				if(strcmp(value,"yes")==0)
					g->isSeed = 1;
				else if(strcmp(value,"no")==0)
					g->isSeed = 0;
				else
					g->isSeed = 0;
			}
			else if(strcmp(token,"localFiles")==0)
				strcpy(g->localFiles,value);
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

void doprocessing (int sock) {
   int n;
   char buffer[256];
   bzero(buffer,256);
   n = read(sock,buffer,255);
   
   if (n < 0) {
      perror("ERROR reading from socket");
      exit(1);
   }
   
   printf("Here is the message: %s\n",buffer);
   n = write(sock,"I got your message",18);
   
   if (n < 0) {
      perror("ERROR writing to socket");
      exit(1);
   }
   
}


//Start a tcp server listening on some port
int create_server(int sport)
{
	struct sockaddr_in saddr,caddr;
	int serversock,clientsock,retval,pid,clen;

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
	saddr.sin_port = htons(sport);

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

		pid = fork();

		if(pid<0)
		{
			perror("Fork Error!");
			exit(1);
		}
		if(pid==0)
		{//client handler
			close(serversock);
			doprocessing(clientsock);
			exit(0);
		}
		else
		{
			close(clientsock);
		}
	}

}

int main(void)
{
	struct g04_config* g04;
	int pd1,pd2,status,n=2,pid;
	g04 = (struct g04_config*)malloc(sizeof(struct g04_config));
	parseConfig(g04);
	printconfig(g04);
	/*pd1 = fork();
	pd2 = fork();
	if(pd1==0)
	{
		create_server(5007);	
	}
	if(pd2==0)
	{
		create_server(5008);
	}

	while (n > 0) {
  		pid = wait(&status);
  		printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
 		 --n;  // TODO(pts): Remove pid from the pids array.
				}*/
	return 0;
}