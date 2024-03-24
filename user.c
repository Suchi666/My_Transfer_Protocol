#include "msocket.h"

int main() {
    init();
    char source_ip[16]="127.0.0.1";
    int source_port=5000;
    char dest_ip[16]="127.0.0.1";
    int dest_port=8000;
    int mtp_socket = m_socket(AF_INET, SOCK_MTP, 0);
    if(mtp_socket==-1){
        printf("Error creating socket\n");
    }
    else{printf("Socket created successfully\n");}
    
    if(m_bind(mtp_socket,source_ip,source_port,dest_ip,dest_port)==-1){
        printf("Error binding\n");
    }
    else{printf("Socket bind successfull\n");}
    char mess[1000]="Hello this is the first message";
    // if(m_sendto(mtp_socket,mess,dest_ip,dest_port)<0){
    //     printf("Error sending message\n");
    // }
    // else{printf("Message sent Successfully\n");}
    return 0;
}