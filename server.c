#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#define MAX_CLIENTS 20
#define BUFFER_SZ 2048
#define NAME_LEN 32

static _Atomic unsigned int cli_count = 0; //los objetos de tipo atomico pueden ser modificados por 2 threads al mismo tiempo o que uno modifique y otro lea
static int uid = 10;

//client structure, stores addresses, socket descriptors, user id, status and name
typedef struct{
	struct sockaddr_in address;
	int sockfd, uid;
	char name[NAME_LEN], status[NAME_LEN];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; //require mutex when we are going to send the message between the clients

void str_overwrite_stdout(){
	printf("\r%s", "> ");
	fflush(stdout); //flush standard output
}

void str_trim_lf(char* arr, int length){ //function for the trimming of the character, osea que imprima todo el mensaje y al final, donde esta el enter (\n) se transforma en \0, que finaliza el string
	for(int i=0; i<length; i++){
		if(arr[i] == '\n'){ //si el mensaje va a terminar, termina el array donde esta el mensaje
			arr[i] = '\0';
			break;
		}
	}
}

//function to add clients to queue
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	
	for(int i=0;i<MAX_CLIENTS; i++){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

//remove client from array of clients
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);
	
	for(int i=0;i<MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

void print_ip_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d", 
		addr.sin_addr.s_addr & 0xff,
		(addr.sin_addr.s_addr & 0xff00) >> 8,
		(addr.sin_addr.s_addr & 0xff0000) >> 16,
		(addr.sin_addr.s_addr & 0xff000000) >> 24);
}

//function to send messages to everyone except the sender
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	
	for(int i=0;i<MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i] -> uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					printf("ERROR: write descriptor failed\n");
					break;
				}
			}
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

//send private message
void send_priv_msg(char *s, char *name){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            if(strcmp(clients[i]->name, name) == 0){
                if(write(clients[i]-> sockfd, s, strlen(s)) < 0){
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

//send clients
void send_clients(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	
	printf("Los clientes conectados son: \n");
	for(int i=0;i<MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i] -> uid != uid){
				printf("%s", clients[i]->name);
			}
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
	char buffer[BUFFER_SZ], name[NAME_LEN], message[BUFFER_SZ + NAME_LEN];
	int leave_flag = 0; //flag que indica si esta conectado el cliente o si hay un error para salir del loop
	cli_count++;
	
	client_t *cli = (client_t*)arg;
	
	
	//name of client
	if(recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1){
		printf("*************ENTER NAME CORRECTLY*************\n");
		leave_flag = 1;
	}else{ //si se recibe correctamente el nombre
		strcpy(cli->name, name); //copiar nombre en el cli->name
		sprintf(buffer, "%s has joined the chat, status [ACTIVO]\n", cli->name); //como se unio el cliente, imprimir en el server y a todos los usuarios en el buffer
		printf("%s", buffer); //print on the server
		send_message(buffer, cli->uid);//send to all the clients excepto al nuevo
	}
	
	bzero(buffer, BUFFER_SZ); //set the buffer to 0
	
	while(1){
		if(leave_flag){
			break;
		}
		//recibir mensajes de los clientes
		int recieve = recv(cli->sockfd, buffer, BUFFER_SZ, 0); //recibe socket del cliente, buffer vacio, el tamano del buffer y 0
		
		
		//condiciones para los mensajes
		if(recieve > 0){ 
			if(strlen(buffer) > 0){ //si hay mensajes, se revisa el tamano del buffer que sea mas que 0
				send_message(buffer, cli->uid); //mandar mensaje a los demas clientes
				str_trim_lf(buffer, strlen(buffer)); //trimm el buffer para imprimir el mensaje en el server
				printf("%s\n", buffer); //se imprime el mensaje y muestra que cliente ha mandado un mensaje
			}
		}else if(recieve == 0 || strcmp(buffer, "7") == 0){ //si el cliente envio "exit" o se recibe 0, se desconecta al cliente y se imprime el mensaje
			sprintf(buffer, "%s has left \n", cli->name);
			printf("%s\n", buffer);
			send_message(buffer, cli->uid); //mostrar a todos los clientes que se salio un cliente
			leave_flag = 1;
		}else if(strcmp(buffer, "4") == 0){
			send_clients(cli->name, cli->uid);
		}else if(strcmp(buffer, "2") == 0){
			//comparar para ver si mandaron un nombre
			for(int i = 0; i<MAX_CLIENTS; i++){
				if(strcmp(buffer, clients[i]->name) == 0){
					str_trim_lf(buffer, strlen(buffer));
					if(strlen(buffer) > 0){
						send_priv_msg(buffer, clients[i]->name);
						str_trim_lf(buffer, strlen(buffer));
						printf("%s", buffer);
					}
				}
			}
		}else{ //cualquier otra opcion es un error
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
		
		
		bzero(buffer, BUFFER_SZ); //se vuelve a vaciar el buffer
	
	}
	//si se sale del loop (osea que se sale un cliente del chat) se cierra su socketfd, se quita del queue, se libera el espacio que ocupaba y se reduce el numero de clientes
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self()); //se libera el thread ocupado por el usuario
	
	return NULL;
}

int main (int argc, char **argv){
	
	if (argc != 2){
		printf("Usage %s <port>\n", argv[0]);
		return EXIT_FAILURE;	
	}
	
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t tid;
	
	//socket settings
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);
	
	//Pipe Signals
	signal(SIGPIPE, SIG_IGN);
	
	//set snaps
	if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
		printf("ERROR setsockopt\n");
		return EXIT_FAILURE;
	}
	
	//bind function
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("ERROR binding\n");
		return EXIT_FAILURE;
	}
	
	//Listen
	if(listen(listenfd, 10) < 0){
		printf("ERROR listening\n");
		return EXIT_FAILURE;
	}
	
	printf("******************WELCOME TO THE CHATROOM******************\n");
	
	//Loop para cominicarse con los clients
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		
		//check for max clients
		if((cli_count + 1) == MAX_CLIENTS){
			printf("MAX Clients connected. REJECTED\n");
			print_ip_addr(cli_addr);
			close(connfd);
			continue;
		}
		
		//Client settings -> recibir data de los clientes
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		
		//add client to queue
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);
		
		//reduce CPU usage
		sleep(1);
	}
	
	
	return EXIT_SUCCESS;
}

