#ifndef MSOCKET_H
#define MSOCKET_H
#define KEY_PATH "/Test_files/p3.c"
#define KEY_ID 'A'
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
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_SOCKETS 1
#define MAX_MSG_LEN 1000
#define MAX_SEND_BUFFER 10
#define MAX_RECV_BUFFER 5
#define MAX_WINDOW_SIZE 5
#define DATA_MESSAGE 0
#define ACK_MESSAGE 1
#define SOCK_MTP 1
//Error values 
#define EINVAL 1
#define ENOBUFS 2
#define UDPSOCK 3
#define ENOTBOUND 4
#define ENOMSG 5
#define TIME_OUT 1000 //milliseconds
#define msec 1000

#define P(s) semop(s, &pop, 1)   // wait(s)
#define V(s) semop(s, &vop, 1)   // signal(s)
extern struct sembuf pop, vop ;

// Structure for MTP socket information
typedef struct {
    int is_free;
    pid_t pid;
    int udp_socket_id;
    char source_ip[16];
    int source_port;
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
#define sharedMemKey 1234
#define sharedVariables 12
#define semKey1 123
#define semKey2 124
#define semKey3 125
#define semKey4 126

extern const char* mutexName;
extern int mtp_errno;
extern int buffer_index;
extern pthread_mutex_t mutex;
extern pthread_mutex_t mutex_sockid;
typedef struct{
    int currSockId;
}SharedVar;


void printBufferContent(int sockfd);
int m_sendto(int sockfd,char message[MAX_MSG_LEN], char dest_ip[16], int dest_port) ;
int find_free_entry();
int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd,char source_ip[16], int source_port,char dest_ip[16], int dest_port);
int init();

#endif 