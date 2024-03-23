#include "msocket.h"
MTPSocket *shared_memory;
pid_t cpid;
SharedVar *sockId;
// sem_t* mutex;
int semId1,semId2;
int semId3,semId4;

void *createSocket(){
  P(semId1);
  printf("creating socket\n");
  pthread_mutex_lock(&mutex_sockid);
  printf("sock value = %d",sockId->currSockId);
  int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_socket == -1) {mtp_errno = UDPSOCK;return -1;}
  sockId->currSockId=udp_socket;
  printf("sock value = %d\n",sockId->currSockId);
  pthread_mutex_unlock(&mutex_sockid);
  V(semId2);
}

void *bind_thread(){
  P(semId3);
  pthread_mutex_lock(&mutex_sockid);
  int sockfd=sockId->currSockId;
  // pthread_mutex_unlock(&mutex_sockid);
  pthread_mutex_lock(&mutex);
  int entry_index = -1;
  for (int i = 0; i < SOCK_MTP;i++) {
    if (shared_memory[i].udp_socket_id == sockfd) {entry_index = i;break;}
  }
  if (entry_index == -1) {
    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&mutex_sockid);
    sockId->currSockId=-1;
    mtp_errno = EINVAL;return;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(shared_memory[entry_index].source_port);
  server_addr.sin_addr.s_addr =inet_addr(shared_memory[entry_index].source_ip);
  pthread_mutex_unlock(&mutex);
  // pthread_mutex_lock(&mutex_sockid);
  if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    printf("Couldn't bind to the port\n");
    sockId->currSockId=-1;
    return -1;
  }
  pthread_mutex_unlock(&mutex_sockid);
  V(semId4);
}

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
  while (1)
  {
    usleep(20 * msec);
    struct timeval current_time;
    
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_SOCKETS; i++){
      if (!shared_memory[i].is_free){
        printf("Sender thread\n");
        int wind_size=0;
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr(shared_memory[i].dest_ip);
        client_addr.sin_port = htons(shared_memory[i].dest_port);
        printf("Client address :- %s\n",shared_memory[i].dest_ip);
        printf("Client port :- %d\n",shared_memory[i].dest_port);
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
                printf("Send failed 1");
                pthread_exit(NULL);
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
              printf("Message to send :- %s\n",mess);
              struct sockaddr_in addr;
              // if (getsockname(shared_memory[i].udp_socket_id, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) == -1) {
              //   perror("getsockname");exit(EXIT_FAILURE);}
              // char ip_address[INET_ADDRSTRLEN];
              // inet_ntop(AF_INET, &(addr.sin_addr), ip_address, INET_ADDRSTRLEN);
              // printf("Local IP Address: %s\n", ip_address);

              // // Convert port number to host byte order and print
              // int port_number = ntohs(addr.sin_port);
              // printf("Local Port Number: %d\n", port_number);
              if (sendto(shared_memory[i].udp_socket_id,mess, strlen(mess), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                pthread_mutex_unlock(&mutex);
                printf("Send failed 2");
                pthread_exit(NULL);
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
void init_process() {
    printf("*********************************************");
    pthread_t thread_R, thread_S;
    if (pthread_create(&thread_R, NULL, receive_thread, NULL) != 0) {
        perror("pthread_create: receive_thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&thread_S, NULL, send_thread, NULL) != 0) {
        perror("pthread_create: send_thread");
        exit(EXIT_FAILURE);
    }
    pthread_t createSock;
    if (pthread_create(&createSock, NULL, createSocket, NULL) != 0) {
        perror("pthread_create: create socket");
        exit(EXIT_FAILURE);
    }
    pthread_t bind_th;
    if (pthread_create(&bind_th, NULL, bind_thread, NULL) != 0) {
        perror("pthread_create: bind thread");
        exit(EXIT_FAILURE);
    }
    pthread_join(thread_S,NULL);
    pthread_join(thread_R, NULL);  
    pthread_join(createSock, NULL); 
    pthread_join(bind_th, NULL);  
}
int main(){
  // mutex = sem_open(mutexName, 0);
  // if (mutex == SEM_FAILED) {
  // perror("Failed to open mutex");
  // return 1;
  // }
  semId1 = semget(semKey1, 1, 0);
  semId2 = semget(semKey2, 1, 0);
  if (semId1 == -1 || semId2 == -1) {
  perror("semget");
  return 1;
  }
  semId3 = semget(semKey3, 1, 0);
  semId4 = semget(semKey4, 1, 0);
  if (semId3 == -1 || semId4 == -1) {
  perror("semget");
  return 1;
  }
  pthread_mutex_lock(&mutex);
  int shmid = shmget(sharedMemKey, MAX_SOCKETS * sizeof(MTPSocket),0666);
  if (shmid < 0) {perror("shmget");printf("ERROR in creating socket\n");exit(EXIT_FAILURE);}
  // Attach to shared memory
  shared_memory = shmat(shmid, NULL, 0);
  if (shared_memory == (MTPSocket *) -1) {perror("shmat");exit(EXIT_FAILURE);}
  pthread_mutex_unlock(&mutex);

  pthread_mutex_lock(&mutex_sockid);
  int shmid1 = shmget(sharedVariables,sizeof(SharedVar),0666);
  if (shmid1 < 0) {perror("shmget");printf("ERROR in creating socket\n");exit(EXIT_FAILURE);}
  // Attach to shared memory
  sockId = shmat(shmid1, NULL, 0);
  if (sockId == (SharedVar*) -1) {perror("shmat");exit(EXIT_FAILURE);}
  printf("sockfd initial============= %d\n",sockId->currSockId);
  pthread_mutex_unlock(&mutex_sockid);
  printf("semaphores initialization done\n");
  init_process();
  return 0;
}


