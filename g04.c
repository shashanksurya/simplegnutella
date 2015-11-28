#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#define MAX_CONNECTIONS 100


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
		{
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
	int pd,status;
	pd = fork();
	if(pd==0)
	{
		create_server(5007);	
	}
	else
	{
		waitpid(pd,&status,0);
	}
	return 0;
}