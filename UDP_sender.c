#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void){
    int client_sock;
    struct sockaddr_in server_addr,client_addr;
    char server_message[2000], client_message[2000];
    // int server_struct_length = sizeof(server_addr);
    // struct sockaddr_in client_addr;
    // int client_struct_length = sizeof(server_addr);
    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    
    // Create socket:
    client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(client_sock < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(2000);
    client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(bind(client_sock,(struct sockaddr* ) &client_addr,sizeof(client_addr))<0)
       perror("Binding failed");
    // Send the message to server:
    strcpy(client_message,"104");
    if(sendto(client_sock, client_message, strlen(client_message), 0,
         (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        printf("Unable to send message\n");
        return -1;
    }
    // Close the socket:
    close(client_sock);
    
    return 0;
}
