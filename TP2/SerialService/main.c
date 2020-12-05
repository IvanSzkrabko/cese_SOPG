#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define Puerto_Serie 1
#define Baudrate 115200
#define BufferSize 100

#define Puerto_TCP 10000
#define IP_TCP "127.0.0.1"

static pthread_t th_TCP;
static int newfd;

//Signal
void sigint_handler(int sig)
{
	pthread_cancel(th_TCP);
	exit(1);
}

//Thread: Leo de la educia y mando al server
void *start_thread(void *aux)
{
	char bufferIn[9];
	while (1)
	{
		//Leo mensajes de la EDUCIA
		usleep(10000);
		if ((serial_receive(bufferIn, sizeof(bufferIn))) != 0)
		{
			//printf("read: %s\r\n", bufferIn);
			// Enviamos mensaje a server
			if (write(newfd, bufferIn, sizeof(bufferIn)) == -1)
			{
				perror("Error escribiendo mensaje en socket");
				// Cerramos conexion con cliente
				close(newfd);
				exit(1);
			}
		}
	}
}

int main(void)
{
	//Signals---------------
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	if (sigaction(SIGTERM, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	//TCP---------------
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[128];
	int n;

	//Serial---------------
	serial_open(Puerto_Serie, Baudrate);
	int serialstatus = 0;

	// Creamos socket
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s==-1)
	{
		perror("error en socket");
		exit(1);
	}

	// Cargamos datos de IP:PORT del server
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(10000);
	
	if (inet_pton(AF_INET, IP_TCP, &(serveraddr.sin_addr)) <= 0)
	{
		fprintf(stderr, "ERROR invalid server IP\r\n");
		return 1;
	}

	// Abrimos puerto con bind()
	if (bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
		close(s);
		perror("listener: bind");
		return 1;
	}

	// Seteamos socket en modo Listening
	if (listen(s, 10) == -1) // backlog=10
	{
		perror("error en listen");
		exit(1);
	}

	//Evito que las señales afecten al thread
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set,NULL);

	//Creo Thread secundario
	char *aux;
	pthread_create(&th_TCP, NULL, start_thread, (void *)aux);

	//Activo señales en el thread ppal
	pthread_sigmask(SIG_UNBLOCK, &set,NULL);

	while (1)
	{
		// Ejecutamos accept() para recibir conexiones entrantes
		addr_len = sizeof(struct sockaddr_in);
		if ((newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
		{
			perror("error en accept");
			exit(1);
		}

		char ipClient[32];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf("server:  conexion desde:  %s\n", ipClient);
		
		while(1)
		{
			// Leemos mensaje de server y enviamos a la educia - BLOQUEANTE
			if( (n = recv(newfd,buffer,128,0)) == -1 )
			{
				perror("Error leyendo mensaje en socket");
				// Cerramos conexion con cliente
				close(newfd);
				break;
			}
			else if (n > 0)
			{
				buffer[n] = 0x00;
				serial_send(buffer, n);
				//printf("Recibi %d bytes.:%s\n", n, buffer);
			}
		}
	}

	exit(EXIT_SUCCESS);
	return 0;
}
