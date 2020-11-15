#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define LOG_FIFO "logger_FIFO"
#define BUFFER_SIZE 300

int32_t fd;

void SIG1_handler(int sig)
{
    write(fd, "SIGN:1", 6);
}

void SIG2_handler(int sig)
{
    write(fd, "SIGN:2", 6);
}

int main(void)
{
   
    char outputBuffer[BUFFER_SIZE];
    char aux[BUFFER_SIZE];
    uint32_t bytesWrote;
    int32_t returnCode;
    struct sigaction SIGNAL_1, SIGNAL_2;

    SIGNAL_1.sa_handler = SIG1_handler;
    SIGNAL_1.sa_flags = 0; //SA_RESTART;
    sigemptyset(&SIGNAL_1.sa_mask);
    sigaction(SIGUSR1, &SIGNAL_1, NULL);

    SIGNAL_2.sa_handler = SIG2_handler;
    SIGNAL_2.sa_flags = 0; //SA_RESTART;
    sigemptyset(&SIGNAL_2.sa_mask);
    sigaction(SIGUSR2, &SIGNAL_2, NULL);

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(LOG_FIFO, S_IFIFO | 0666, 0) ) < -1 )
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
	printf("waiting for readers...\n");
	if ( (fd = open(LOG_FIFO, O_WRONLY) ) < 0 )
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
	if(fgets(aux, BUFFER_SIZE, stdin)>0)
	{
		sprintf(outputBuffer, "DATA: %s", aux);
       
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
    }
return 0;
}
