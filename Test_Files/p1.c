#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>


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
int main() {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    usleep(100*1000);
    usleep(200*1000);
    gettimeofday(&end_time, NULL);
    // Compute the time difference
    long time_diff_ms = timeval_diff(&start_time, &end_time);
    printf("Time difference: %ld milliseconds\n", time_diff_ms);
    return 0;
}