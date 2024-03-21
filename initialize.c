#include "msocket.h"

mtp_errno=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
buffer_index=0; 
MTPSocket *shared_memory;
