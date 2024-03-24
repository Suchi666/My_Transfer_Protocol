#include "msocket.h"
MTPSocket *shared_memory;
pid_t cpid;
SharedVar *sockId;
// sem_t* mutex;
int semId1,semId2;
int semId3,semId4;

void printBufferContent(int sockfd) {
    
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS;i++) {
        if (shared_memory[i].udp_socket_id == sockfd) {entry_index = i;break;}
    }
    if (entry_index == -1) {printf("Socket ID %d not found!\n", sockfd);}
    MTPSocket mtpsockfd = shared_memory[entry_index];
    printf("------------Buffer Content-------------\n");
    for (int i = 0; i < mtpsockfd.sendBuffSize; i++) {
    printf("    Message %d: %s\n", i+1, mtpsockfd.send_buffer[i]);
    }
}
int find_free_entry() {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (shared_memory[i].is_free) {
            return i;
        }
    }
    return -1; // No free entry found
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

int m_socket(int domain, int type, int protocol) {
    if (type != SOCK_MTP) {mtp_errno = EINVAL;return -1;}
    // Check for a free entry in shared memory
    pthread_mutex_lock(&mutex);
    int free_entry = find_free_entry();
    if (free_entry == -1) {mtp_errno = ENOBUFS;return -1;}
    pthread_mutex_unlock(&mutex);
    // Create UDP socket
    
    printf("Creating socket.......................\n");
    sleep(1);
    V(semId1);
    P(semId2);
    pthread_mutex_lock(&mutex_sockid);
    int udp_socket=sockId->currSockId;
    sockId->currSockId=-1;
    pthread_mutex_unlock(&mutex_sockid);
    printf("sock created :- %d\n",udp_socket);
    semctl(semId1, 0, SETVAL, 0);
	semctl(semId2, 0, SETVAL, 0);
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
    strcpy(shared_memory[entry_index].source_ip, source_ip);
    shared_memory[entry_index].source_port = source_port;
    printf("dest ip = %s\ndest port = %d\n",shared_memory[entry_index].dest_ip,shared_memory[entry_index].dest_port);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_lock(&mutex_sockid);
    sockId->currSockId=sockfd;
    pthread_mutex_unlock(&mutex_sockid);
    V(semId3);
    P(semId4);
    semctl(semId3, 0, SETVAL, 0);
	semctl(semId4, 0, SETVAL, 0);
    int return_value;
    pthread_mutex_lock(&mutex_sockid);
    if(sockId->currSockId==sockfd){
        sockId->currSockId=-1;
        return_value=0;
    }
    else{
        return_value=-1;
    }
    pthread_mutex_unlock(&mutex_sockid);
    return return_value;
}
int init(){
    
  // // SEMAPHORES AND MUTEXES INITIALIZATION-----------------------------------------------------------
    // mutex = sem_open(mutexName, O_CREAT, S_IRUSR | S_IWUSR, 1);
    // if (mutex == SEM_FAILED) {
    //     perror("Failed to create/open mutex");
    //     return 1;
    // }
    semId1 = semget(semKey1, 1, 0777|IPC_CREAT);
	semId2 = semget(semKey2, 1, 0777|IPC_CREAT);
    semctl(semId1, 0, SETVAL, 0);
	semctl(semId2, 0, SETVAL, 0);
    semId3 = semget(semKey3, 1, 0777|IPC_CREAT);
	semId4 = semget(semKey4, 1, 0777|IPC_CREAT);
    semctl(semId3, 0, SETVAL, 0);
	semctl(semId4, 0, SETVAL, 0);
    printf("%d",sharedMemKey);
    int shmid = shmget(sharedMemKey, MAX_SOCKETS * sizeof(MTPSocket), IPC_CREAT | 0666);
    if (shmid < 0) {
      printf("Error ----------");
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
        shared_memory[i].source_ip[0] = '\0'; // Initialize to empty string
        shared_memory[i].source_port = 0;
        shared_memory[i].dest_ip[0] = '\0'; // Initialize to empty string
        shared_memory[i].dest_port = 0;
        shared_memory[i].recvIndex = 0; // Initialize to 0
        shared_memory[i].maxSend = 1;
        shared_memory[i].sendBuffSize = 0;
        for(int j=0;j<10;j++){
            strcpy(shared_memory[i].send_buffer[j], "\0");
        }
        for(int j=0;j<5;j++){
            strcpy(shared_memory[i].recv_buffer[j], "\0");
        }
        for (int j = 0; j < MAX_WINDOW_SIZE; j++) {
            shared_memory[i].swnd[j] = -1; // Initialize to -1
            shared_memory[i].rwnd[j] = j; // Initialize to -1
        }
    }
    printf("%d",sharedVariables);
    int shmid1 = shmget(sharedVariables, sizeof(SharedVar), IPC_CREAT | 0666);
    if (shmid1 < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    sockId = shmat(shmid1, NULL, 0);
    if (sockId == (SharedVar *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    } 
    sockId->currSockId=-1;
    //-------------------------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------------------------
    cpid=fork();
    if(cpid==0){
    char *args[] = {"gnome-terminal", "--", "bash", "-c", "./run_threads; exec bash", NULL}; 
    execvp("gnome-terminal",args);
    }
    else{
        printf("Initializing ....\n");
        sleep(5);
        printf("Initialization Done\n");
        return 0;
    }
}


