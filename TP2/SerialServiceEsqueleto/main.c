#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

void* THREAD_RX_puerto_serie();
int desbloquearSign(void);
int bloquearSign(void);
void handler_SIGINT(int signumber);
void handler_SIGTERM(int signumber);

volatile sig_atomic_t received_signal = 0;

int main()
{
//--------------------COnfiguro las senales---------------------------//
	struct sigaction sa1, sa2;
	sa1.sa_handler = handler_SIGINT;
	sa1.sa_flags = 0;
	sigemptyset(&sa1.sa_mask);
	if( sigaction(SIGINT, &sa1, NULL) == -1 )
	{
			perror("ERROR, no se puedo manejar bien la senal SIGINT\r\n");
			return 1;
	}

	sa2.sa_handler = handler_SIGTERM;
	sa2.sa_flags = 0;
	sigemptyset(&sa2.sa_mask);
	if( sigaction(SIGINT, &sa2, NULL) == -1 )
	{
			perror("ERROR, no se puedo manejar bien la senal SIGTERM\r\n");
			return 1;
	}
//---------------------------------------------------------------------//

	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[128];
	int newfd;
	int n;
	pthread_t HANDLER_RX_puerto_serie;
	void* valor_de_retorno_del_handler;
	int thread_created = 0;

	// Creamos socket
	int s = socket(AF_INET,SOCK_STREAM, 0);

	// Cargamos datos de IP:PORT del server
  bzero((char*) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(10000);
  //serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if(inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr))<=0)
  {
	    fprintf(stderr,"ERROR invalid server IP\r\n");
	    return 1;
  }

	// Abrimos puerto con bind()
	if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
	{
			close(s);
			perror("listener: bind");
			return 1;
	}

	// Seteamos socket en modo Listening
	if (listen (s, 10) == -1) // backlog=10
  {
			perror("error en listen");
	  	exit(1);
  }

	// Ejecutamos accept() para recibir la conexion entrante
	addr_len = sizeof(struct sockaddr_in);
	while(1)
	{
		  if ( (newfd = accept(s,
													(struct sockaddr *)&clientaddr,
		                      &addr_len)) == -1 )
		  {
				if( received_signal == SIGINT || received_signal == SIGTERM )
						exit(0);
				perror("error en accept");
				exit(1);
			}

			char ipClient[32];
			inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
			printf ("server:  conexion desde:  %s\n", ipClient);

			//una vez que hay una conexion de socket TCP establecida, creo el thread de
			//recepcion por el puerto serie
			//bloqueo las senales para que no ocurra ningun pseudoestado en la transicion cuando se crea el thread
			if( bloquearSign() )
			{
				perror("ERROR al bloquear las senales antes de crear el thread");
				return 1;
			}
			//creo el thread y verifico que se creo de manera correcta ()

			thread_created = pthread_create(&HANDLER_RX_puerto_serie,
										NULL,
										THREAD_RX_puerto_serie,
										(void *) &newfd
										);
			if( thread_created != 0)
			{
				perror("No se puedo crear el thread de manera correcta, SALIENDO \r\n");
				return 1;
			}

			//Una vez que se ha creado correctamente el thread, desbloqueo senales
			if( desbloquearSign() )
			{
                                perror("ERROR al desbloquear las senales despues de crear el thread");
                                return 1;
			}

		  while(1)
		  {
					n = read(newfd, buffer, 128);

					if( n == -1 )
					{
						if( received_signal == SIGINT || received_signal == SIGTERM )
								break;
							perror("Error leyendo mensaje en socket");
							exit(1);
					}
					if ( n > 0 )
					{
							buffer[n]=0x00;
							printf("Recibi %d bytes.:%s\n",n,buffer);
					}
					if( n == 0 )
					{
							printf("Socket cerrado\r\n");
							break;
					}
			}
			//inicio la cancelacion del thread
			pthread_cancel(HANDLER_RX_puerto_serie);
			//espero hasta que se cancele de manera correcta
			pthread_join(HANDLER_RX_puerto_serie, valor_de_retorno_del_handler);
			close(newfd);
			if( received_signal == SIGINT || received_signal == SIGTERM )
					break;
	}
	return 0;
}


void handler_SIGINT(int signumber)
{
	//escribo la variable global
	received_signal = SIGINT;
}

void handler_SIGTERM(int signumber)
{
	//escribo la variable global
	received_signal = SIGTERM;
}


int bloquearSign(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	//sigaddset(&set, SIGUSR1);
	return pthread_sigmask(SIG_BLOCK,
			&set,
			NULL);
}

int desbloquearSign(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	return pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void* THREAD_RX_puerto_serie(void *pila_FIFO)
{
	char comando_recibido[10];
	while(1)
	{
		scanf("%s", comando_recibido);
		if ( write( *(int *)pila_FIFO,
								comando_recibido,
								sizeof(comando_recibido)
							) == -1 )
		{
			perror("ERROR, NO SE PUEDE ESCRIBIR EN LA PILA FIFO");
			break;
		}
	}
}
