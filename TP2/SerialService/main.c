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
#include <stdbool.h>

#define Puerto_Serie 1
#define Baudrate 115200
#define BufferSize 100

#define Puerto_TCP 10000
#define IP_TCP "127.0.0.1"

static pthread_t th_TCP;
static int newfd;
static bool TCPok,endProgram;
static pthread_mutex_t mutexData = PTHREAD_MUTEX_INITIALIZER;

//Signal
void sigint_handler(int sig)
{
	endProgram=true;
}

//Thread: Leo de la educia y mando al server
void *start_thread(void *aux)
{
	char bufferIn[9];
	while (1)
	{
		//Leo mensajes de la EDUCIA
		usleep(10000);
		pthread_mutex_lock(&mutexData);
		if (TCPok)
		{
			//perror("Serial Enable\n");
			pthread_mutex_unlock(&mutexData);
			if ((serial_receive(bufferIn, sizeof(bufferIn))) != 0)
			{
				//printf("read: %s\r\n", bufferIn);
				// Enviamos mensaje a server
				if (write(newfd, bufferIn, sizeof(bufferIn)) <= 0)
				{
					perror("Error escribiendo mensaje en socket");
					// Cerramos conexion con cliente
					close(newfd);
					exit(1);
				}
			}
		}
		else
		{
			pthread_mutex_unlock(&mutexData);
			//perror("Serial Blocked\n");
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

	endProgram=false;

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

	if (sigemptyset(&set)|sigaddset(&set, SIGTERM)|sigaddset(&set, SIGINT) != 0)
	{
		perror("error en sigset_t");
		exit(1);
	}
	

	if 	(pthread_sigmask(SIG_BLOCK, &set,NULL) != 0)
	{
		perror("error en pthread_sigmask");
		exit(1);
	}

	//Creo Thread secundario
	char *aux;
	if (pthread_create(&th_TCP, NULL, start_thread, (void *)aux)!= 0)
	{
		perror("error en pthread_create");
		exit(1);
	}

	//Activo señales en el thread ppal
	if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0)
	{
		perror("error en pthread_sigmask");
		exit(1);
	}

	while (1)
	{

		// Ejecutamos accept() para recibir conexiones entrantes
		addr_len = sizeof(struct sockaddr_in);
		if (((newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1) || endProgram)
		{
			perror("error en accept");
			break;
		}

		char ipClient[32];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf("Server:  conexion desde:  %s\n", ipClient);
		
		pthread_mutex_lock (&mutexData);
		TCPok=true;
		pthread_mutex_unlock (&mutexData);

		while(1)
		{
			// Leemos mensaje de server y enviamos a la educia - BLOQUEANTE
			if( ((n = recv(newfd,buffer,128,0)) < 0) || endProgram)
			{
				perror("Error leyendo mensaje en socket\n");
				close(newfd);
				break;
			}
			else if (n > 0)
			{
				buffer[n] = 0x00;
				serial_send(buffer, n);
				//printf("Recibi %d bytes.:%s\n", n, buffer);
			}
			else
			{
				pthread_mutex_lock (&mutexData);
				TCPok=false;
				pthread_mutex_unlock (&mutexData);
				close(newfd);
				printf("Socket Cerrado\n");
				break;	
			}
		}
		if (endProgram)
		{
			break;
		}
	}
	
	printf("FinDePrograma!\n");
	pthread_mutex_lock(&mutexData);
	TCPok = false;
	pthread_mutex_unlock(&mutexData);
	pthread_cancel(th_TCP);
	pthread_join(th_TCP, NULL);
	serial_close();
	exit(EXIT_SUCCESS);
	return 0;
}
