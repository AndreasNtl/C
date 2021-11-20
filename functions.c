#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include "interfaces.h"

#define FILENAME "file.txt"
#define MEMORY_SIZE (2*sizeof(Line))

void produce_in(int semid1, char * mem, Line line) {
    struct sembuf downempty;
    struct sembuf upfull;

    char *tempmem = mem;

    // prepare semaphores
    downempty.sem_num = 1;
    downempty.sem_op = -1;
    downempty.sem_flg = 0;

    upfull.sem_num = 0;
    upfull.sem_op = 1;
    upfull.sem_flg = 0;

    // lock
    if (semop(semid1, &downempty, 1) == -1) {
        perror("Semop");
        exit(0);
    }

    memcpy(tempmem, &line, sizeof (Line));
    
//    printf("p %d produced message %d-%s \n", getpid(), line.pid, line.line);

    if (semop(semid1, &upfull, 1) == -1) {
        perror("Semop");
        exit(0);
    }
}

void produce_out(int semid2, char * mem, Line line) {

    struct sembuf upfull;
    struct sembuf downempty;

    char *tempmem = mem;

    // prepare semaphores
    downempty.sem_num = 1;
    downempty.sem_op = -1;
    downempty.sem_flg = 0;

    upfull.sem_num = 0;
    upfull.sem_op = 1;
    upfull.sem_flg = 0;

    // lock
    if (semop(semid2, &downempty, 1) == -1) {
        perror("Semop");
        exit(0);
    }

    tempmem = tempmem + (MEMORY_SIZE / 2);
    memcpy(tempmem, &line, sizeof (Line));

    printf("c %d produced message %d-%s \n", getpid(), line.pid, line.line);
    
    // unlock
    if (semop(semid2, &upfull, 1) == -1) {
        perror("Semop");
        exit(0);
    }
}

Line consume_in(int semid, char * mem) {
    struct sembuf downfull;
    struct sembuf upempty;

    char * tempmem = mem;

    Line line;

    // prepare semaphores
    downfull.sem_num = 0;
    downfull.sem_op = -1;
    downfull.sem_flg = 0;

    upempty.sem_num = 1;
    upempty.sem_op = 1;
    upempty.sem_flg = 0;

    // lock
    if (semop(semid, &downfull, 1) == -1) {
        perror("semop");
        exit(1);
    }

    memcpy(&line, tempmem, sizeof (Line));
    
//    printf("c %d consumed message %d-%s \n", getpid(), line.pid, line.line);

    // unlock
    if (semop(semid, &upempty, 1) == -1) {
        perror("Semop");
        exit(0);
    }

    return line;
}

Line consume_out(int semid, char * mem) {

    struct sembuf downfull;
    struct sembuf upempty;

    char * tempmem = mem;

    Line line;

    // prepare semaphores
    downfull.sem_num = 0;
    downfull.sem_op = -1;
    downfull.sem_flg = 0;

    upempty.sem_num = 1;
    upempty.sem_op = 1;
    upempty.sem_flg = 0;

    // lock
    if (semop(semid, &downfull, 1) == -1) {
        perror("semop");
        exit(1);
    }

    // consume out
    tempmem = tempmem + (MEMORY_SIZE / 2);
    memcpy(&line, tempmem, sizeof (Line));

   // printf("p %d consumed message %d-%s \n", getpid(), line.pid, line.line);
    
    if (semop(semid, &upempty, 1) == -1) {
        perror("Semop");
        exit(0);
    }

    return line;
}

void c(int count_c, int no_of_p, int sem_id_in_ds, int sem_id_out_ds, char * mem) {
    int i, j;
    Line l;
    int mypid = getpid();

    // work
    for (i=0;i<count_c;i++) {
        l = consume_in(sem_id_in_ds, mem);

        for (j = 0; j < strlen(l.line); j++) {
            l.line[j] = toupper(l.line[j]);
        }

        produce_out(sem_id_out_ds, mem, l);
    }
    
    // shutdown
    for (i=0;i<no_of_p;i++) {
        printf("C waiting for P (i=%d) \n", i);
        consume_in(sem_id_in_ds, mem);
        l.pid = -1;
        produce_out(sem_id_out_ds, mem, l);
    }

    printf("c %d finished. \n", mypid);
    
    exit(0);
}

void p(int sem_id_in_ds, int sem_id_out_ds, char * mem) {
    Line l;
    int mypid = getpid();
    int pid_match = 0;
    int samples = 0;
    
    FILE * file = fopen(FILENAME, "rt");
    int no_of_lines = 0;
    int x;
    int i;
    srand(mypid);
    
    while (fgets(l.line, sizeof(l.line), file) > 0) {
        no_of_lines++;        
    }
    
    fclose(file);
    
    file = fopen(FILENAME, "rt");
    
    while (1) {
        l.pid = mypid;    
        
        x = rand()%no_of_lines;
        
        // skip x lines
        fseek(file, 0, SEEK_SET);
        for (i=0;i<x;i++) {
            fgets(l.line, sizeof(l.line), file);
        }
        
        // read desired line
        fgets(l.line, sizeof(l.line), file);
                
        produce_in(sem_id_in_ds, mem, l);
        
        l = consume_out(sem_id_out_ds, mem);
                
        if (l.pid == mypid) {
//            printf("p %d consumed message %d-%s \n", getpid(), l.pid, l.line);
            pid_match++;
        } else if (l.pid == -1) {
            break;
        } 
        samples++;        
    }
    
    printf("p %d finished with pid_match: %d out of %d samples \n", mypid, pid_match, samples);
    
    fclose(file);

    exit(pid_match);
}

int m(int no_of_p, int count_c) {
    int i;
    int total_match= 0;

    // create SHM
    int id;
    id = shmget(IPC_PRIVATE, MEMORY_SIZE, 0600);
    if (id == -1) {
        perror("CREATION");
    }

    // attach SHM
    char *mem;
    mem = (char *) shmat(id, (void *) 0, 0);
    if ((int*) mem == (int*) - 1) {
        perror("Attachment.");
    }

    /* Create a new semaphore id */
    int sem_id_in_ds = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (sem_id_in_ds == -1) {
        perror("Semaphore creation ");
    }

    /* Create a new semaphore id */
    int sem_id_out_ds = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (sem_id_out_ds == -1) {
        perror("Semaphore creation ");
    }

    // Initialize semaphore values
    
    // full
    arg.val = 0;
    if (semctl(sem_id_in_ds, 0, SETVAL, arg) == -1) {
        perror("# Semaphore setting value ");
    }

    if (semctl(sem_id_out_ds, 0, SETVAL, arg) == -1) {
        perror("# Semaphore setting value ");
    }

    // empty
    arg.val = 1;
    if (semctl(sem_id_in_ds, 1, SETVAL, arg) == -1) {
        perror("# Semaphore setting value ");
    }

    if (semctl(sem_id_out_ds, 1, SETVAL, arg) == -1) {
        perror("# Semaphore setting value ");
    }

    // create processes
    for (i = 0; i < no_of_p; i++) {
        int k = fork();
        if (k == 0) {
            p(sem_id_in_ds, sem_id_out_ds, mem);
        }
    }

    int k = fork();
    if (k == 0) {
        c(count_c, no_of_p, sem_id_in_ds, sem_id_out_ds, mem);
    }

    // wait for children ...
    int status;

    for (i = 0; i < no_of_p + 1; i++) {
        wait(&status);
        
        int match = WEXITSTATUS(status) ;
        total_match = total_match + match;
    }

    // detach SHM
    int err;
    err = shmdt((void *) mem);
    if (err == -1) {
        perror("Detachment");
    }

    // REMOVE shm
    err = shmctl(id, IPC_RMID, 0);
    if (err == -1) {
        perror("Removal");
    }

    // REMOVE Semaphores
    semctl(sem_id_in_ds, 0, IPC_RMID, 0);

    semctl(sem_id_out_ds, 0, IPC_RMID, 0);
    
    return total_match;
}

