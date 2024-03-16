#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void init_process() {
    char *args[] = {"gnome-terminal", "--", "bash", "-c", "./hello; exec bash", NULL}; 
    execvp("gnome-terminal",args);
}

int main() {
    pid_t cpid;
    cpid = fork();
    if (cpid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (cpid == 0) {
        // Child process
        init_process();
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL);
        printf("Parent Process\n");
        wait(NULL); // Wait for the child process to complete
    }
    return 0;
}
