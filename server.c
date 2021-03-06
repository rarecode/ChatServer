#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <fcntl.h>
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#define TRUE   1 
#define FALSE  0 

typedef struct SocketTable {
     int socket;
     char clientName[10];
}SockTab;

char * getPeerName(char packet[], char buff[]) {
    int i, j=0;
    for (i = 0; i < strlen(packet); i++) {
        if (j >= 2) {     
           if (packet [i] == '#') {
               buff[j - 2] = '\0';
               break;
           } else {
               buff[j - 2] = packet[i];
               j++;
           }
        }
        if (packet[i] == '#' && j < 2)
           j++;
        
    }
    return buff;
}

int getDestSocket(char packet[], char buff[], SockTab Table[]) {
    int i, j=0;
    for (i = 0; i < strlen(packet); i++) {
        if (j >= 4) {     
           if (packet [i] == '#') {
               buff[j-4] = '\0';
               break;
           } else {
               buff[j - 4] = packet[i];
               j++;
           }
        }
        if (packet[i] == '#' && j < 4)
           j++;
    }
    for (i = 0; i < 30 ; i++) {
        if (!strcmp (buff, Table[i].clientName)) {
            return Table[i].socket;
        }
    }
    return -1;
}

int server(char *Address, int Port)  {  
    int opt = TRUE, listen_socket , addrlen , new_socket , client_socket[30]  = {0}, max_clients = 30 , activity, i , valread , sd, j, k, max_sd;  
    SockTab Table[100];
    struct sockaddr_in address;  
    char buffer[1025], PeerName[10] = {'\0'}, DestName[100] = {'\0'};
    fd_set readfds, masterfds;  
    
    for (i = 0; i < 100; i++)
        Table[i].socket = 0;
    i = 0;
    
    if( (listen_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }
    if( setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = inet_addr(Address);  
    address.sin_port = htons(Port);  
    if (bind(listen_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", Port);
    
    if (listen(listen_socket, 15) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    FD_ZERO(&readfds);
    FD_ZERO(&masterfds);
    FD_SET(listen_socket, &masterfds);
    FD_SET(listen_socket, &readfds);
    max_sd = listen_socket; 
    while(TRUE) {
        //copy master fd set to read fd every time, because select modifies set after every call
        readfds = masterfds;
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }
        printf("activity :%d\n",activity);
        if (FD_ISSET(listen_socket, &readfds)) {
            //if activity on listening socket then its incoming request.
            if ((new_socket = accept(listen_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  {  
                perror("accept");
                exit(EXIT_FAILURE);
            }

            valread = read( new_socket , buffer, 1024);
            //read the buffer first whenever client connects to server to get client information 
            FD_SET(new_socket, &masterfds);
            readfds = masterfds;
            if (new_socket > max_sd) 
                max_sd = new_socket;
           
            for (i = 0; i < 100; i++) {
                if (Table[i].socket == 0) {
                    k = i;
                    break;
                }
            }
            //copy client info into table.
            strcpy(Table[k].clientName,getPeerName(buffer, Table[k].clientName));
            Table[k].socket = new_socket;
            
            printf("Socket , Client Name :%d, %s\n",Table[k].socket,Table[k].clientName);
            printf("New connection, socket fd is %d, ip is : %s, port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            
            for (i = 0; i < max_clients; i++) {
                //if position is empty 
                if( client_socket[i] == 0 ) {
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                    break;  
                }
            }
        } else {
            //activity on connected socket i.e some client sent a message.
            for (i = 0; i < max_clients; i++) {  
                sd = client_socket[i];
                if (FD_ISSET( sd , &readfds)) {
                    if ((valread = read( sd , buffer, 1024)) == 0) {
                        // if valread == 0 then client disconnected.
                        getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);
                        FD_CLR(sd, &masterfds);
                        FD_CLR(sd, &readfds);
                       
                        // clean up routing table. 
                        for (k = 0; k< 100; k++) {
                            if (sd == Table[k].socket) {
                                Table[k].socket = 0;
                                Table[k].clientName[0] = '\0';
                                break;
                            }
                        }
                        printf("Host disconnected , ip %s , port %d \n" ,inet_ntoa(address.sin_addr), ntohs(address.sin_port)); 
                        // make client socket entry empty 
                        client_socket[i] = 0; 
                        if (max_sd == sd) {
                            max_sd = listen_socket;
                            for (k = 0; k < max_clients; k++) {
                                if (client_socket[k] > max_sd)
                                    max_sd = client_socket[k];
                            }
                        }
                        //Close the socket 
                        close(sd);
                         
                    } else {
                        buffer[valread] = '\0';
                        printf("%s\n",buffer);
                        j = getDestSocket(buffer,DestName, Table);
	    	        if (j < 0) {
	    	            printf("Error : No destination found");
                               //TODO :Error checks and further functionality here 
                               // send this to another server cause dest client is not connected to this server.
	    	        } else {
                            write(j , buffer,strlen(buffer));  
                        }
                    }
                }
            }
        }
    }
    return 0; 
}

int main(int argc, char *argv[]) {
    int i=0;
    int child[3] = {0}, pid;
    int port = 49152;
    server("127.0.0.1",49152);
} 
