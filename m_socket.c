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
#include <sys/time.h>

#define MAX_SOCKETS 1
#define MAX_MSG_LEN 1000
#define MAX_SEND_BUFFER 10
#define MAX_RECV_BUFFER 5
#define MAX_WINDOW_SIZE 5
#define DATA_MESSAGE 0
#define ACK_MESSAGE 1
#define SOCK_MTP 1
//  Error values 
#define EINVAL 1
#define ENOBUFS 2
#define UDPSOCK 3
#define ENOTBOUND 4
#define ENOMSG 5
#define TIME_OUT 1000 //milliseconds
#define msec 1000


// Structure for MTP socket information
typedef struct {
    int is_free;
    pid_t pid;
    int udp_socket_id; // Socket ID assigned by the init process
    char dest_ip[16]; // Assuming IPv4
    int dest_port;
    char send_buffer[10][MAX_MSG_LEN];
    char recv_buffer[5][MAX_MSG_LEN];
    int swnd[MAX_WINDOW_SIZE];
    int rwnd[MAX_WINDOW_SIZE];
    int recvWindSize;
    int sendBuffSize;
    struct timeval last_send_time;
} MTPSocket;


int mtp_errno = 0;
MTPSocket *shared_memory;
pthread_mutex_t mutex;
int buffer_index;

// Thread function for receiving messages
void *receive_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    printf("Receive thread\n");
    sleep(1);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}
long timeval_diff(struct timeval *start, struct timeval *end) {
    long seconds_diff = end->tv_sec - start->tv_sec;
    long microseconds_diff = end->tv_usec - start->tv_usec;
    // Handle the carry-over from microseconds to seconds
    if (microseconds_diff < 0) {
        seconds_diff--;
        microseconds_diff += 1000000;
    }
    // Convert time difference to milliseconds
    return seconds_diff * 1000 + microseconds_diff / 1000;
}
void *send_thread(void *arg)
{
  printf("Sender thread\n");
  while (1)
  {
    usleep(20 * msec);
    struct timeval current_time;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_SOCKETS; i++){
      if (!shared_memory[i].is_free){
        int wind_size=0;
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr(shared_memory[i].dest_ip);
        client_addr.sin_port = htons(shared_memory[i].dest_port);
        gettimeofday(&current_time, NULL);
        int time_diff = timeval_diff(&current_time, &shared_memory[i].last_send_time);
        if (time_diff > TIME_OUT){
          for (int j = 0; j < MAX_WINDOW_SIZE; j++){
            if (shared_memory[i].swnd[j] != -1){
              wind_size++;
              char mess[1001];
              sprintf(mess, "%d", shared_memory[i].swnd[j]);
              strcat(mess,shared_memory[i].send_buffer[shared_memory[i].swnd[j]]);
              if (sendto(shared_memory[i].udp_socket_id,mess, strlen(mess), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                printf("Send failed");
                return;
              }
            }
          }
        }
        if(wind_size<MAX_WINDOW_SIZE && shared_memory[i].recvWindSize>wind_size){
          int idx=MAX_WINDOW_SIZE-1;
          while(shared_memory[i].swnd[idx]==-1 && idx>=0)idx--;
          if(idx<0)idx=0;
          idx=(shared_memory[i].swnd[idx]+1)%MAX_SEND_BUFFER;
          // int send_num=shared_memory[i].recvWindSize-wind_size;
          int end_idx=(idx-1)>0?(idx-1):MAX_SEND_BUFFER-1;
          while(idx!=end_idx && wind_size<shared_memory[i].recvWindSize){
            if(strlen(shared_memory[i].send_buffer[idx])!=0){
              shared_memory[i].swnd[wind_size]=idx;
              wind_size++;
              char mess[1001];
              sprintf(mess, "%d", idx);
              strcat(mess,shared_memory[i].send_buffer[idx]);
              if (sendto(shared_memory[i].udp_socket_id,mess, strlen(mess), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                printf("Send failed");
                return;
              }
            }
            idx=(idx+1)%MAX_SEND_BUFFER; 
          }
        }
      }
    }
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}
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
    buffer_index=0;    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        shared_memory[i].is_free = 1; // Mark as free
        shared_memory[i].pid = -1; // Initialize to invalid PID
        shared_memory[i].udp_socket_id = -1; // Initialize to invalid socket ID
        shared_memory[i].dest_ip[0] = '\0'; // Initialize to empty string
        shared_memory[i].dest_port = 0; // Initialize to 0
        shared_memory[i].sendBuffSize = 0;
        for(int j=0;j<10;j++){
            strcpy(shared_memory[i].send_buffer[j], "\0");
        }
        for(int j=0;j<5;j++){
            strcpy(shared_memory[i].recv_buffer[j], "\0");
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