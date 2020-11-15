#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define LOG_FIFO "logger_FIFO"


#define BUFFER_SIZE 300

int main(void)
{
    uint8_t inputBuffer[BUFFER_SIZE];
    int32_t bytesRead, returnCode, fd;
    FILE *record_LOG, *record_SIGN;

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(LOG_FIFO, S_IFIFO | 0666, 0) ) < -1  )
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }
    
    /* Open named fifo. Blocks until other process opens it */
	printf("waiting for writers...\n");
	if ( (fd = open(LOG_FIFO, O_RDONLY) ) < 0 )
    {
        printf("ERROR! NO SE PUDO ABRIR 'LOG_FIFO': %d\n", fd);
        exit(1);
    }

    /* open syscalls returned without error -> other process attached to named fifo */
	printf("got a writer\n");

    /* Loop until read syscall returns a value <= 0 */
	do
	{
        /* read data into local buffer */
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        	{
			perror("read");
        	}
        	else
		{
			inputBuffer[bytesRead] = '\0';
			//printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);
			if(strstr(inputBuffer, "DATA"))//reviso si el string "DATA" esta dentro de lo que viene en la FIFO 
			{
				record_LOG = fopen("Log.txt", "a");
				fprintf(record_LOG, "%s\n", inputBuffer);
				fclose(record_LOG);
			}else if(strstr(inputBuffer, "SIGN"))
			{
				record_SIGN = fopen("Sign.txt", "a");
				fprintf(record_SIGN, "%s\n", inputBuffer);
				fclose(record_SIGN);
			}
		}
	}
	while (bytesRead > 0);
	return 0;
}
