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

//variables globales
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char *name, *status;
int in_chat = 0;
char new_status[BUFFER_SZ + NAME_LEN] = {};

void str_overwrite_stdout(){
	printf("\r%s", "> ");
	fflush(stdout); //flush standard output
}

void str_trim_lf(char* arr, int length){ //function for the trimming of the character
	for(int i=0; i<length; i++){
		if(arr[i] == '\n'){
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(){ //controla los mensajes para salir, ya sea ctrl+c o "exit"
	flag = 1;
}

//handler de recibir mensajes
void recv_msg_handler(){
	char message[BUFFER_SZ] = {};
	
	while(1){
		int recieve = recv(sockfd, message, BUFFER_SZ, 0);
		
		if(recieve > 0){ //si recibe algo
			printf("%s ", message);
			str_overwrite_stdout();
		}else if(recieve == 0){ //si no hay mensajes
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

//handler para enviar mensajes
void send_msg_handler(){
	char buffer[BUFFER_SZ] = {}, message[BUFFER_SZ + NAME_LEN] = {};
	
	while(1){
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin); //recibidor de mensajes del cliente
		str_trim_lf(buffer, BUFFER_SZ);
		
		status = new_status;
		
		if(strcmp(buffer, "*") == 0){
			in_chat = 1;
			break;
		}
		//change status
		else if(strcmp(buffer, "ACTIVO") == 0 || strcmp(buffer, "OCUPADO") == 0 || strcmp(buffer, "INACTIVO") == 0){
			strcpy(new_status, buffer);
			sprintf(message, "%s, cambio su status a %s\n", name, status); //enviar el mensaje por cada cliente que haya enviado
			send(sockfd, message, strlen(message), 0);
			//printf("%s tu nuevo status es %s\n", name, status);
			in_chat = 1;
		}else if(strcmp(buffer, "4") == 0){
			strcpy(message, "Lista de Usuarios");
			printf(">>Jorge\n");
			printf(">>Daniela\n");
			printf(">>Daniela2\n");
			printf(">>Jorge2\n");
			in_chat = 1;
			break;
		}else{
			sprintf(message, "%s [%s]: %s\n", name, new_status, buffer); //enviar el mensaje por cada cliente que haya enviado
			send(sockfd, message, strlen(message), 0);
		}
		
		bzero(buffer, BUFFER_SZ);
		bzero(message, BUFFER_SZ + NAME_LEN);
	}
	
}

//handler para enviar mensajes privados
void send_privmsg_handler(){
	char buffer[BUFFER_SZ] = {}, priv_message[BUFFER_SZ + NAME_LEN] = {}, priv_name[NAME_LEN] = {};
	
	printf("\nA quien deseas mandar el mensaje privado?:");
	scanf("%d", priv_name);
	send(sockfd, priv_name, strlen(priv_name), 0);
	
	while(1){
		
		printf("Cual es el mensaje?: \n");
		strcpy(buffer, priv_message);
		send(sockfd, priv_message, strlen(priv_message), 0);
		
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin); //recibidor de mensajes del cliente
		str_trim_lf(buffer, BUFFER_SZ);
		if(strcmp(buffer, "*") == 0){
			in_chat = 1;
			break;
		}else{
			sprintf(priv_message, "%s [Privado]: %s\n", name, buffer);
			send(sockfd, priv_message, strlen(priv_message), 0);
		}
		
		
		bzero(buffer, BUFFER_SZ);
		bzero(priv_message, BUFFER_SZ + NAME_LEN);
		bzero(priv_name, NAME_LEN);
	}
	
}

//get all chats
void get_all_chats(){
	//thread para enviar mensajes
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para enviar los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	//thread para recibir mensajes
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para recibir los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	while(1){
		
		if(in_chat){
			pthread_cancel(send_msg_thread);
			pthread_cancel(recv_msg_thread);
			printf("\nRegresando al Menu Principal...\n");
			in_chat = 0;
			break;
		}
	}
}

//change status
void change_status(){
	//thread para enviar mensajes
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para enviar los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	while(1){
		if (in_chat){
			pthread_cancel(send_msg_thread);
			in_chat = 0;
			printf("%s tu nuevo status es %s\n", name, status);
			break;
		}
	}
}

//private chat
void private_chat(){
	//thread para enviar mensajes
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_privmsg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para enviar los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	//thread para recibir mensajes
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para recibir los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	while(1){
		if (in_chat){
			pthread_cancel(send_msg_thread);
			pthread_cancel(recv_msg_thread);
			in_chat = 0;
			break;
		}
	}
}

//print usuarios conectados
void list_clients(){
	char buffer[BUFFER_SZ] = {}, message[BUFFER_SZ + NAME_LEN] = {};
	
	//thread para enviar mensajes
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para enviar los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	//thread para recibir mensajes
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para recibir los mensajes y null)
		printf("ERROR: pthread\n");
		//return EXIT_FAILURE;
		in_chat = 1;
	}
	
	while(1){
		
		if(in_chat){
			pthread_cancel(send_msg_thread);
			pthread_cancel(recv_msg_thread);
			in_chat = 0;
			break;
		}
		
	}
}

//user info
void user_info(){
	char buffer[BUFFER_SZ] = {}, message[BUFFER_SZ + NAME_LEN] = {};
	
	str_overwrite_stdout();
	fgets(buffer, BUFFER_SZ, stdin); //recibidor de mensajes del cliente
	str_trim_lf(buffer, BUFFER_SZ);
	while(1){
		if(strcmp(buffer, "5") == 0){
			if(strcmp(buffer, "Daniela") == 0){
				printf(">Name: Daniela");
				printf(">Status: %s", status);
				break;
			}else if(strcmp(buffer, "Jorge") == 0){
				printf(">Name: Jorge");
				printf(">Status: %s", status);
				break;
			}else if(strcmp(buffer, "Daniela2") == 0){
				printf(">Name: Daniela2");
				printf(">Status: %s", status);
				break;
			}else if(strcmp(buffer, "Jorge2") == 0){
				printf(">Name: Jorge2");
				printf(">Status: %s", status);
				break;
			}
		}
	}
}

//MENU
void menu(){
	printf("\n******************WELCOME TO THE CHATROOM******************\n");
	printf(">	Para chatear con todos los usuarios, ingrese 1\n");
	printf(">	Para chatear con un usuario por privado, ingrese 2\n");
	printf(">	Si desea cambiar su STATUS, ingrese 3\n");
	printf(">	Si desea listar todos los clientes conectados, ingrese 4\n");
	printf(">	Si desea la informacion de un usuario en especifico, ingrese 5\n");
	printf(">	Si necesita ayuda, ingrese 6\n");
	printf(">	Si desea salir, ingrese 7\n");
}

void help(){
	printf("\n***********Manual de ayuda************\n");
	printf("Si eliges 1, podras chatear con todos los usuarios conectados!\n");
	printf("Si eliges 2, puedes hablarle al usuario que desees sin que los demas lo vean.\n");
	printf("Si eliges 3, podras cambiar tu STATUS entre ACTIVO, OCUPADO e INACITIVO\n");
	printf("Si eliges 4, puedes observar a todos los usuarios conectados\n");
	printf("Si eliges 5, puedes obtener la informacion de un usuario de tu eleccion!\n");
	printf("Si eliges 6, obtendras el Manual de ayuda de nuevo.\n");
	printf("Si deseas salir del chat, solo ingresa 7.\n");
}

int main (int argc, char **argv){
	int choice;	
	
	if(argc != 4){
		printf("Usage: %s <Username> <IP> <port>", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *ip = argv[2];
	int port = atoi(argv[3]);
	
	signal(SIGINT, catch_ctrl_c_and_exit); //definir la senal para salir
	
	name = (argv[1]);
	
	if(strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
		printf("Enter name Correctly\n");
		return EXIT_FAILURE;
	}
	
	struct sockaddr_in server_addr;
	
	//socket settings
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);
	
	//conectar client al servidor
	int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err == -1){
		printf("ERROR: can't connect\n");
		return EXIT_FAILURE;
	}
	
	//mandar el nombre del client
	send(sockfd, name, NAME_LEN, 0);
	
	menu();
	
	while(1){
		printf("%s, que deseas hacer?: ", name);
		scanf("%d", &choice);
		
		switch(choice){
			case 1:
			printf("\nIngrese * para salir al menu principal o escriba su mensaje:\n");
			get_all_chats();
			break;
			
			case 2:
			private_chat();
			break;
			
			case 3:
			printf("\n 1. ACTIVO");
			printf("\n 2. OCUPADO");
			printf("\n 3. INACTIVO");
			printf("\nIngrese el estado que desea para cambiar su estado: \n");
			change_status();
			break;
			
			case 4:
			list_clients();
			break;
			
			case 5:
			user_info();
			break;
			
			case 6:
			help();
			break;
			
			case 7:
			catch_ctrl_c_and_exit();
			break;
			
			default:
			printf("\nIngrese solamente un numero del 1-7.\n");
			break;
		}
		if(flag){
			printf("\nBye\n");
			break;
		}
		
		menu();
	}
	
	close(sockfd);
	

	return EXIT_SUCCESS;
}