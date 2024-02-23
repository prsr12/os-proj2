#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char **argv) {
        int sec  = atoi(argv[1]);               
        int nano = atoi(argv[2]);       
        key_t SH_KEY = ftok(".", 'x');
        if (argc < 3) {
                fprintf(stderr, "Usage: %s seconds nanoseconds\n", argv[0]);
                exit(1);
        }
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

        int sys_sec = shm_ptr[0];
        int sys_nano = shm_ptr[1];       
        int terminate_sec = sec + sys_sec;
        int terminate_nano = nano + sys_nano;
         int terminate = sys_sec;

        printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n -- Just Starting\n", getpid(), getppid(), sys_sec, sys_nano, terminate_sec, terminate_nano);
        while (sys_sec < terminate_sec ||(sys_sec==terminate_sec && sys_nano < terminate_nano)) {
                usleep(10000);
                sys_sec = shm_ptr[0];
                sys_nano = shm_ptr[1];

        if (sys_sec == terminate_sec && sys_nano >= terminate_nano) {
                break;
        }
                if (sys_sec != terminate) {
                        printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n -- %d seconds have passed since starting\n", getpid(), getppid(), sys_sec, sys_nano, terminate_sec, terminate_nano, sys_sec - terminate);
                terminate = sys_sec;
                }
        }

                printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n -- Terminating\n", getpid(), getppid(), sys_sec, sys_nano, terminate_sec, terminate_nano);
        shm_ptr[2] = 1;
        shmdt(shm_ptr);
        return 0;
}
