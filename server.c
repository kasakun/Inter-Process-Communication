//
// Created by Zeyu Chen, Yaohong Wu on 2/20/18.
//

#define DEBUG 0
#define LOG 1
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <memory.h>

#include "server.h"

#define SIZE 4096

server* server_init(int pid) {
    server* s;
    s = (server *)malloc(sizeof(server));
    s->pid = pid;


    // high priority queue
    if ((s->higqid = msgget(HIGKEY, IPC_CREAT | 0666)) == -1) {
        printf("msg queue created error!\n");
        exit(2);
    }
    if ((s->higsid = sem_init(HIGKEY, 1)) == -1) {
        printf("sem for msg queue created error!\n");
        exit(3);
    }

    // medium priority queue
    if ((s->medqid = msgget(MEDKEY, IPC_CREAT | 0666)) == -1) {
        printf("msg queue created error!\n");
        exit(2);
    }
    if ((s->medsid = sem_init(MEDKEY, 1)) == -1) {
        printf("sem for msg queue created error!\n");
        exit(3);
    }

    // low priority queue
    if ((s->lowqid = msgget(LOWKEY, IPC_CREAT | 0666)) == -1) {
        printf("msg queue created error!\n");
        exit(2);
    }
    if ((s->lowsid = sem_init(LOWKEY, 1)) == -1) {
        printf("sem for msg queue created error!\n");
        exit(3);
    }

    printf("Server: Initialize finish...\n");
    return s;
}
int sem_init(key_t key, int nsems) {
    int semid;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;

    if (semid = semget(key, nsems, IPC_CREAT | 0666)) {
        sb.sem_op = 1;
        sb.sem_flg = 0;
        arg.val = 1;

        for (sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) {
            if (semop(semid, &sb, 1) == -1) {
                int e = errno;
                semctl(semid, 0, IPC_RMID);
                return -1;
            }
        }
    }
    else {
        printf("Server: Initialize fail: cannot create sem buf, %s\n", strerror(errno));
    }

    return semid;
}



void request_handler(req* request) {
    int* head;
    int fd;
    struct sembuf sb;

    // lock

    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = SEM_UNDO;
#if DEBUG
    printf("Server: sem lock...\n");
#endif
    semop(request->sh_sem_id, &sb, 1);

#if 1
    printf("Server: Working on client(%d): %d request | ", request->pid, request->rid);
    printf("Shared Memory name: %s,  Semaphore id: %d.\n", request->name, request->sh_sem_id);
#endif
    // Use POSIX
    //create file
    fd = shm_open(request->name, O_CREAT | O_RDWR, 0666);
#if DEBUG
    if (fd < 0) {
        printf("Server: Illegal fd! exit\n");
        exit(1);
    }
#endif
    //map
    ftruncate(fd, SIZE);
    head = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#if DEBUG
    printf("Server: Mapping...\n");
    if (head == -1) {
        printf("Server: Mapping fail!, %s \n", strerror(errno));
        exit(1);
    }
#endif
    // writing
    int result = func(request->p.x, request->p.p);
    *(head + request->rid) = result;

    // WYH: use the spot after the last request to count
    *(head + request->num_requests) = request->rid + 1;

#if DEBUG
    printf("Server: Address:%d  ", head + request->rid);
    printf("Input: %d\n", *(head + request->rid));
#endif
    // unmapping

    if (munmap(head, SIZE) == -1) {
        printf("Server: Unmap failed!, %s \n", strerror(errno));
        exit(1);
    }
    // close file
    //shm_unlink(request->name);
#if DEBUG
    printf("Server: Unmap success!\n");
#endif

    sb.sem_op = 1;
#if DEBUG
    printf("Server: sem unlock...\n");
#endif
    semop(request->sh_sem_id, &sb, 1);
    return;
}

void request_schedule(server* s) {
    int i;
    int temp; //
    msgqnum_t  num;
    struct sembuf sb;
    struct msqid_ds qbuf;
    msgbuf buf;
    sb.sem_num = 0;
    sb.sem_flg = SEM_UNDO;

    // QoS:
    // The goal of the service is to response the request based on different priority
    // High > Medium > Low
    // The mechanism here is, first loop all msg in the same priority level.
    //

    // High
    printf("Server: Checking Queues.\n");
#if LOG
    printf("Server: Checking high priority queue.\n");
    //lock
#endif
    sb.sem_op = -1;
    if (semop(s->higsid, &sb, 1) == -1) {
        printf("Server: High queue lock fail!");
        exit(1);
    }
    if (msgctl(s->higqid, IPC_STAT, &qbuf) == -1) {
        printf("Server: msgctl error!\n");
        exit(1);
    }
    num = qbuf.msg_qnum;
    for (i = 0; i < HQSIZE && i < num; ++i) {
        if (msgrcv(s->higsid, &buf, sizeof(msgbuf), 0, MSG_NOERROR) == -1) {
            printf("Server: msgrcv error!\n");
        }
        //printf("%s", buf.r.name);
        request_handler(&buf.r);
    }
    temp = HQSIZE - i;
    //unlock
    sb.sem_op = 1;
    if (semop(s->higsid, &sb, 1) == -1) {
        printf("Server: High queue unlock fail!");
        exit(1);
    }

    // Meidium
#if LOG
    printf("Server: Checking medium priority queue.\n");
#endif
    //lock
    sb.sem_op = -1;
    if (semop(s->medsid, &sb, 1) == -1) {
        printf("Server: Medium queue lock fail!");
        exit(1);
    }
    if (msgctl(s->medqid, IPC_STAT, &qbuf) == -1) {
        printf("Server: msgctl error!\n");
        exit(1);
    }
    num = qbuf.msg_qnum;
    for (i = 0; i < (MQSIZE + temp) && i < num; ++i) {
        if (msgrcv(s->medsid, &buf, sizeof(msgbuf), 0, MSG_NOERROR) == -1) {
            printf("Server: msgrcv error!\n");
        }
        request_handler(&buf.r);
    }

    temp = (MQSIZE + temp) - i;
    //unlock
    sb.sem_op = 1;
    if (semop(s->medsid, &sb, 1) == -1) {
        printf("Server: High queue unlock fail!");
        exit(1);
    }

    // Low
#if LOG
    printf("Server: Checking low priority queue.\n");
#endif
    //lock
    sb.sem_op = -1;
    if (semop(s->lowsid, &sb, 1) == -1) {
        printf("Server: Low queue lock fail!");
        exit(1);
    }
    if (msgctl(s->lowqid, IPC_STAT, &qbuf) == -1) {
        printf("Server: msgctl error!\n");
        exit(1);
    }
    num = qbuf.msg_qnum;
    for (i = 0; i < (LQSZIE + temp) && i < num; ++i) {
        if (msgrcv(s->lowsid, &buf, sizeof(msgbuf), 0, MSG_NOERROR) == -1) {
            printf("Server: msgrcv error!\n");
        }
        request_handler(&buf.r);
    }

    //unlock
    sb.sem_op = 1;
    if (semop(s->lowsid, &sb, 1) == -1) {
        printf("Server: Low queue unlock fail!");
        exit(1);
    }
    return;
}



void server_start(server *s) {
    printf("Server: Start!...\n");

    while(1) {
        request_schedule(s);
        sleep(1);
    }

    return;
}


int func(int x, int p) { // change to long!!!!!
    int mul = 2*x; // protect bound
    return mul % p;
}