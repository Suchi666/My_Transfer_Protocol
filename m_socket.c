#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_SOCKETS 1
#define MAX_BUFFER_SIZE 1024
#define MAX_WINDOW_SIZE 5
#define DATA_MESSAGE 0
#define ACK_MESSAGE 1
#define SOCK_MTP 1

#define EINVAL 1
#define ENOBUFS 2
#define UDPSOCK 3
#define ENOTBOUND 4
#define ENOMSG 5
// Structure for MTP socket information
typedef struct {
    int is_free;
    pid_t pid;
    int udp_socket_id; // Socket ID assigned by the init process
    char dest_ip[16]; // Assuming IPv4
    int dest_port;
    char send_buffer[10][MAX_BUFFER_SIZE];
    char recv_buffer[5][MAX_BUFFER_SIZE];
    int swnd[MAX_WINDOW_SIZE];
    int rwnd[MAX_WINDOW_SIZE];
} MTPSocket;

int mtp_errno = 0;
// Global shared memory
MTPSocket *shared_memory;

// Mutex for synchronization
pthread_mutex_t mutex;

// Thread function for receiving messages
void *receive_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    printf("Receive thread\n");
    sleep(1);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}
void *send_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    printf("Sender thread\n");
    sleep(1);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
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
// Initialization function
void init() {
    // Create shared memory segment
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int shmid = shmget(IPC_PRIVATE, MAX_SOCKETS * sizeof(MTPSocket), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (MTPSocket *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        shared_memory[i].is_free = 1; // Mark as free
        shared_memory[i].pid = -1; // Initialize to invalid PID
        shared_memory[i].udp_socket_id = -1; // Initialize to invalid socket ID
        shared_memory[i].dest_ip[0] = '\0'; // Initialize to empty string
        shared_memory[i].dest_port = 0; // Initialize to 0
        for(int j=0;j<10;j++){
            strcpy(shared_memory[i].send_buffer[0], "\0");
        }
        for(int j=0;j<5;j++){
            strcpy(shared_memory[i].recv_buffer[0], "\0");
        }
        for (int j = 0; j < MAX_WINDOW_SIZE; j++) {
            shared_memory[i].swnd[j] = -1; // Initialize to -1
            shared_memory[i].rwnd[j] = -1; // Initialize to -1
        }
    }
    // Create threads R and S
    pthread_t thread_R, thread_S;
    if (pthread_create(&thread_R, NULL, receive_thread, NULL) != 0) {
        perror("pthread_create: receive_thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&thread_S, NULL, send_thread, NULL) != 0) {
        perror("pthread_create: send_thread");
        exit(EXIT_FAILURE);
    }
    pthread_join(thread_R, NULL);
    pthread_join(thread_S, NULL);
}



int main() {
    init();
    char source_ip[16]="127.0.0.1";
    int source_port=5000;
    char dest_ip[16]="192.168.1.100";
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
    return 0;
}