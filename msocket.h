#ifndef MSOCKET_H
#define MSOCKET_H

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
//Error values 
#define key 1234
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

extern pthread_mutex_t mutex;
extern int mtp_errno;
// extern MTPSocket *shared_memory;
extern int buffer_index;

void printBufferContent(int sockfd);
int m_sendto(int sockfd,char message[MAX_MSG_LEN], char dest_ip[16], int dest_port) ;
int find_free_entry();
int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd,char source_ip[16], int source_port,char dest_ip[16], int dest_port);
int init();

#endif 