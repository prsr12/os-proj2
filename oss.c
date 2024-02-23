#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <stdbool.h>

typedef struct PCB {
    int occupied;           
    pid_t pid;              
    int startSeconds;       
    int startNano;          

} PCB;

PCB processTable[20];

#define NANO_INC 10000000 // represents 10 milliseconds
#define NANO_SEC 1000000000 // 1 second equals a billion nanosec

int main(int argc, char **argv) {
    key_t SH_KEY = ftok(".", 'x');
    int shm_id = shmget(SH_KEY, sizeof(int) * 2, 0777 | IPC_CREAT);
    if (shm_id <= 0) {
        fprintf(stderr, "Shared memory grab failed \n");
        exit(1);
    }
    int *shm_ptr = shmat(shm_id, 0, 0);
    if (shm_ptr == (void *) -1) {
        fprintf(stderr, "Shared memory attach failed \n");
        exit(1);
    }
    int sys_sec = 0;
    int sys_nano = 0;

    int simul = 3; 
    bool stillChildrenToLaunch = true;
    for (int i = 0; i < simul; i++) {
        // Update system clock
        sys_nano += NANO_INC;
        if (sys_nano >= NANO_SEC) {
            sys_sec++;
            sys_nano -= NANO_SEC;
        }

        pid_t new_pid = fork();
        if (new_pid == 0) {
            char sec_str[10], nano_str[10];
            sprintf(sec_str, "%d", sys_sec + 5);
            sprintf(nano_str, "%d", sys_nano);
            execl("./worker", "./worker", sec_str, nano_str, NULL);
            perror("Execl() failed");
            exit(0); 
        } else if (new_pid > 0) {
            for (int j = 0; j < 20; j++) {
                if (!processTable[j].occupied) {
                    processTable[j].occupied = 1;
                    processTable[j].pid = new_pid;
                    processTable[j].startSeconds = sys_sec;
                    processTable[j].startNano = sys_nano;
                    break;
                }
            }
        } else {
            fprintf(stderr, "Fork failed.\n");
            exit(1);
        }
    }
    while (stillChildrenToLaunch == true) {
        sys_nano += NANO_INC;
        if (sys_nano >= NANO_SEC) {
            sys_sec++;
            sys_nano -= NANO_SEC;
        }

        shm_ptr[0] = sys_sec; 
        shm_ptr[1] = sys_nano; 

        usleep(10000);
        if (sys_nano % 500000000 == 0) { 
            printf("Entry  Occupied  PID      StartS  StartN\n");
            for (int i = 0; i < 20; i++) {
                printf("%-6d %-9d %-8d %-7d %d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
            }
        }

        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) { 
            printf("Child process with PID %d has terminated.\n", pid);
            for (int i = 0; i < 20; i++) {
                if (processTable[i].pid == pid) {
                    processTable[i].occupied = 0;
                    break;
                }
            }
            if (--simul > 0) { 
                pid_t new_pid = fork();
                if (new_pid == 0) {
                
                    char sec_str[10], nano_str[10];
                    sprintf(sec_str, "%d", sys_sec + 5);
                    sprintf(nano_str, "%d", sys_nano);
                    execl("./worker", "./worker", sec_str, nano_str, NULL);
                    perror("Execl() failed");
                    exit(0); 
                } else if (new_pid > 0) {
                    printf("Parent process has launched a child with PID %d.\n", new_pid);
                    for (int i = 0; i < 20; i++) {
                        if (!processTable[i].occupied) {
                            processTable[i].occupied = 1;
                            processTable[i].pid = new_pid;
                            processTable[i].startSeconds = sys_sec;
                            processTable[i].startNano = sys_nano;
                            break;
                        }
                    }
                } else {
                    fprintf(stderr, "Fork failed.\n");
                    exit(1);
                }
            } else {
                stillChildrenToLaunch = false;
            }
        }
    }
    shmdt(shm_ptr);

    return 0;
}
