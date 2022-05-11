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
char *name;
int in_chat = 0;
char *status;

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
	
		if(strcmp(buffer, "*") == 0){
			in_chat = 1;
			break;
		}else{
			sprintf(message, "%s: %s\n", name, buffer); //enviar el mensaje por cada cliente que haya enviado
			send(sockfd, message, strlen(message), 0);
		}
		
		bzero(buffer, BUFFER_SZ);
		bzero(message, BUFFER_SZ + NAME_LEN);
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
			printf("Regresando al Menu Principal...\n");
			break;
		}
	}
}
//change status
void change_status(){
	char buffer[BUFFER_SZ] = {}, status[BUFFER_SZ + NAME_LEN] = {};
	
	while(1){
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin); //recibidor de mensajes del cliente
		str_trim_lf(buffer, BUFFER_SZ);
		
		if(strcmp(buffer, "ACTIVO") == 0){
			printf("%s, tu nuevo estado es: ACTIVO\n\n", name);
			send(sockfd, status, strlen(status), 0);
			break;
		}else if(strcmp(buffer, "OCUPADO") == 0){
			printf("%s, tu nuevo estado es: OCUPADO\n\n", name);
			send(sockfd, status, strlen(status), 0);
			break;
		}else if(strcmp(buffer, "INACTIVO") == 0){
			printf("%s, tu nuevo estado es: INACTIVO\n\n", name);
			send(sockfd, status, strlen(status), 0);
			break;
		}
		
	}
}

//print usuarios conectados
void list_clients(){
	char buffer[BUFFER_SZ] = {}, message[BUFFER_SZ + NAME_LEN] = {};
	
	while(1){
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin); //recibidor de mensajes del cliente
		str_trim_lf(buffer, BUFFER_SZ);
	
		if(strcmp(buffer, "4") == 0){
			send(sockfd, message, strlen(message), 0);
			break;
		}
		
	}
}


//MENU
void menu(){
	printf("******************WELCOME TO THE CHATROOM******************\n");
	printf(">	Para chatear con todos los usuarios, ingrese 1\n");
	printf(">	Para chatear con un usuario por privado, ingrese 2\n");
	printf(">	Si desea cambiar su STATUS, ingrese 3\n");
	printf(">	Si desea listar todos los clientes conectados, ingrese 4\n");
	printf(">	Si desea la informacion de un usuario en especifico, ingrese 5\n");
	printf(">	Si necesita ayuda, ingrese 6\n");
	printf(">	Si desea salir, ingrese 7\n");
}

void help(){
	printf("***********Manual de ayuda************\n");
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
	
	//printf("Enter your name: ");
	//fgets(name, NAME_LEN, stdin); //get client enter
	//str_trim_lf(name, strlen(name)); 
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
	
	/*//thread para enviar mensajes
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para enviar los mensajes y null)
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}
	
	//thread para recibir mensajes
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){ //si no es igual a 0 hay un error (pt_c recibe la address del thread del mensaje, null, la funcion para recibir los mensajes y null)
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}*/
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
			break;
			
			case 6:
			help();
			break;
			
			case 7:
			catch_ctrl_c_and_exit();
			break;
			
			default:
			printf("Ingrese solamente un numero del 1-7.");
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