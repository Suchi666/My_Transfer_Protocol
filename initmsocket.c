#include "msocket.h"


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
              if (sendto(shared_memory[i].udp_socket_id,mess, strlen(mess), 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                printf("Send failed");
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
    // Create shared memory segment
    
    int shmid = shmget(key, MAX_SOCKETS * sizeof(MTPSocket), IPC_CREAT | 0666);
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
        shared_memory[i].dest_port = 0;
        shared_memory[i].recvWindSize = MAX_RECV_BUFFER; // Initialize to 0
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
    pthread_join(thread_S,NULL);
    pthread_join(thread_R, NULL);  
}
int main(){
  init_process();
  return 0;
}

