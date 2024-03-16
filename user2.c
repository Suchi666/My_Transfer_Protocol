#include "msocket.h"

int main() {
    init();
    char source_ip[16]="192.168.1.100";
    int source_port=8000;
    char dest_ip[16]="172.16.0.2";
    int dest_port=5000;
    int mtp_socket = m_socket(AF_INET, SOCK_MTP, 0);
    if(mtp_socket==-1){
        printf("Error creating socket\n");
    }
    else{printf("Socket created successfully\n");}
    
    if(m_bind(mtp_socket,source_ip,source_port,dest_ip,dest_port)==-1){
        printf("Error binding\n");
    }
    else{printf("Socket bind successfull\n");}
    return 0;
}