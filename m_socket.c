#include "msocket.h"
#include "mutex.h"

void printBufferContent(int sockfd) {
    
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS;i++) {
        if (shared_memory[i].udp_socket_id == sockfd) {entry_index = i;break;}
    }
    if (entry_index == -1) {pthread_mutex_unlock(&mutex);printf("Socket ID %d not found!\n", sockfd);}
    MTPSocket mtpsockfd = shared_memory[entry_index];
    printf("------------Buffer Content-------------\n");
    for (int i = 0; i < mtpsockfd.sendBuffSize; i++) {
    printf("    Message %d: %s\n", i+1, mtpsockfd.send_buffer[i]);
    }
}
int m_sendto(int sockfd,char message[MAX_MSG_LEN], char dest_ip[16], int dest_port) {
    // Check if sockfd is a valid MTP socket
    if(sockfd<0){mtp_errno = EINVAL;return -1;}
    pthread_mutex_lock(&mutex);
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS;i++) {
        if (shared_memory[i].udp_socket_id == sockfd) {entry_index = i;break;}
    }
    if (entry_index == -1) {pthread_mutex_unlock(&mutex);mtp_errno = EINVAL;return -1;}
    MTPSocket mtpsock = shared_memory[entry_index];
    // Check if the destination IP and port match the bound IP and port
    if (strcmp(mtpsock.dest_ip, dest_ip) != 0 || mtpsock.dest_port != dest_port) {
        mtp_errno = ENOTBOUND;
        return -1;
    }
    // Check if there is enough space in the send buffer
    if (mtpsock.sendBuffSize>=MAX_SEND_BUFFER) {mtp_errno = ENOBUFS;return -1;}
    // printf("ERROR\n");
    // printf("message to be sent = %s\n",message);
    // printf("Initial Buffer Size = %d\n",mtpsock.sendBuffSize);
    strcpy(shared_memory[entry_index].send_buffer[buffer_index], message);
    printf("%s\n",shared_memory[entry_index].send_buffer[mtpsock.sendBuffSize]);
    shared_memory[entry_index].sendBuffSize++;
    buffer_index++;
    buffer_index=buffer_index%10;
    printBufferContent(mtpsock.udp_socket_id);
    pthread_mutex_unlock(&mutex);
    return strlen(message);
}

int find_free_entry() {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (shared_memory[i].is_free) {
            return i;
        }
    }
    return -1; // No free entry found
}
int m_socket(int domain, int type, int protocol) {
    if (type != SOCK_MTP) {mtp_errno = EINVAL;return -1;}
    // Check for a free entry in shared memory
    int free_entry = find_free_entry();
    if (free_entry == -1) {mtp_errno = ENOBUFS;return -1;}

    // Create UDP socket
    int udp_socket = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == -1) {mtp_errno = UDPSOCK;return -1;}
    pthread_mutex_lock(&mutex);
    // Initialize shared memory entry
    shared_memory[free_entry].is_free = 0;
    shared_memory[free_entry].pid = getpid();
    shared_memory[free_entry].udp_socket_id = udp_socket;
    // Initialize other fields as needed
    pthread_mutex_unlock(&mutex);
    return udp_socket;
}

int m_bind(int sockfd,char source_ip[16], int source_port,char dest_ip[16], int dest_port) {
    if(sockfd < 0) {mtp_errno = EINVAL;return -1;}
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(source_port);
    server_addr.sin_addr.s_addr =inet_addr(source_ip);
    
    // Bind to the set port and IP:
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    // Find the corresponding entry in shared memory
    pthread_mutex_lock(&mutex);
    int entry_index = -1;
    for (int i = 0; i < SOCK_MTP;i++) {
        if (shared_memory[i].udp_socket_id == sockfd) {entry_index = i;break;}
    }
    if (entry_index == -1) {pthread_mutex_unlock(&mutex);mtp_errno = EINVAL;return -1;}

    // Update shared memory with destination IP and port
    strcpy(shared_memory[entry_index].dest_ip, dest_ip);
    shared_memory[entry_index].dest_port = dest_port;
    printf("dest ip = %s\n dest port = %d\n",shared_memory[entry_index].dest_ip,shared_memory[entry_index].dest_port);

    pthread_mutex_unlock(&mutex);
    return 0;
}

int init(){
    pid_t cpid;
    cpid=fork();
    if(cpid==0){
        char *args[] = {"gnome-terminal", "--", "bash", "-c", "./init; exec bash", NULL}; 
        execvp("gnome-terminal",args);
        exit(EXIT_SUCCESS);
    }else{
        return 1;
    }
}

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
    if(m_sendto(mtp_socket,mess,dest_ip,dest_port)<0){
        printf("Error sending message\n");
    }
    else{printf("Message sent Successfully\n");}
    return 0;
}