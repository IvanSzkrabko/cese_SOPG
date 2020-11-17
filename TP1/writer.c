#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "myfifo"
#define BUFFER_SIZE 300


void sigint_handler(int sig);

volatile sig_atomic_t inputSignal=0;

int main(void)
{
    char outputBuffer[BUFFER_SIZE];
	uint32_t bytesWrote,signalResult;
	int32_t returnCode, fd;
    struct sigaction sa;


    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
     if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }



    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1 )
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
	printf("waiting for readers...\n");
	if ( (fd = open(FIFO_NAME, O_WRONLY) ) < 0 )
    {
        printf("Error opening named fifo file: %d\n", fd);
        exit(1);
    }
    
    /* open syscalls returned without error -> other process attached to named fifo */
	printf("got a reader--type some stuff\n");

    /* Loop forever */
	while (1)
	{
        /* Get some text from console */
		fgets(outputBuffer, BUFFER_SIZE, stdin);

         if(inputSignal==0){
        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
		    if ((bytesWrote = write(fd, outputBuffer, strlen(outputBuffer)-1)) == -1)
            {
    			perror("write");
            }
            else
            {
    			printf("writer: wrote %d bytes\n", bytesWrote);
            }
         }  
        if(inputSignal==10){
            if ((signalResult=write(fd, "SIGN:1", strlen("SIGN:1"))) == -1)
                perror("write");
            else
                inputSignal=0;
        }
        else if(inputSignal==12){
            if ((signalResult=write(fd, "SIGN:2", strlen("SIGN:2"))) == -1)
                perror("write");
            else
                inputSignal=0;
        }        
	}
	return 0;
}


void sigint_handler(int sig) {
    inputSignal=sig;
}