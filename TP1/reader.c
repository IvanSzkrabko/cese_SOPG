#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "myfifo"
#define BUFFER_SIZE 300

uint8_t parseInput(char* input);
void writeToFile(char archivo[10], char* mensaje);

int main(void)
{
	uint8_t inputBuffer[BUFFER_SIZE];
    uint8_t stringToFile[BUFFER_SIZE];
	int32_t bytesRead, returnCode, fd;
   
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1  )
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }
    
	printf("waiting for writers...\n");
	if ( (fd = open(FIFO_NAME, O_RDONLY) ) < 0 )
    {
        printf("Error opening named fifo file: %d\n", fd);
        exit(1);
    }
    
	printf("got a writer\n");

	do
	{
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
			perror("read");
        }
        else
		{
			inputBuffer[bytesRead] = '\0';
            uint8_t result=parseInput(inputBuffer);
			if(result!=0)
            printf("Resultado: %d Mensaje: \"%s\"\n", result, inputBuffer);
            result=0;
		}
	}
	while (bytesRead > 0);
    
	return 0;
}

uint8_t parseInput(char* input){
    uint8_t header[4];
    strncpy(header,input,4);

    if(strcmp(header,"DATA")==0)
    {
        writeToFile("Log.txt",input);
        return 1;
    }
    else if(strcmp(header,"SIGN")==0)
    {
        writeToFile("Sign.txt",input);
        return 2;
    }
    return 0;
}

void writeToFile(char archivo[10], char* mensaje){
    FILE* fp;
    fp = fopen(archivo ,"a");
    if (fp != NULL) {   
        fprintf(fp,"%s \n", mensaje);
        fclose(fp);
    }
}
