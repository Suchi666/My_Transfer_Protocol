#include "msocket.h"

int mtp_errno=0;
int buffer_index=0;
 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_sockid = PTHREAD_MUTEX_INITIALIZER;
struct sembuf pop ={0,-1,0};
struct sembuf vop ={0,1,0};
const char* mutexName = "/my_named_mutex";

