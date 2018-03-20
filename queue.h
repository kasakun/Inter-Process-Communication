//
// Created by Zeyu Chen, Yaohong Wu on 2/19/18.
//

#ifndef IPC_QUEUE_H
#define IPC_QUEUE_H

#include <sys/types.h>

#define MAX_SIZE_NAME 16
#define MAX_SIZE 20
// Computation packet
typedef struct __packet {
    int p;
    int x;
} packet;

// Request structure
typedef struct __request {
    int rid;
    int pid;
    char name[MAX_SIZE_NAME];     // POSIX file name
    char* addr;
    //int shid      // System V
    int sh_sem_id;
    packet p;
    int num_requests;
    struct __request* next;
} req;

// Request message structure
typedef struct __msgbuf {
    long mtype;
    req r;
} msgbuf;

// Response message structure
typedef struct __response {
    int rsid;
    int* rid;
    int* res;
} res;

// Request queue
typedef struct __reqqueue {
    req* head;
    req* tail;
    int count;
} req_queue;

// Response queue should be implemented on client thread

union semun {
    int val;
    struct semid_ds * buf;
    ushort *array;
};



int Enqueue(req* ele, req_queue* queue); // return -1 if fail
req* Dequeue(req_queue* queue);          // return NULL if fail

int queueSize(req_queue* queue);
int isEmpty(req_queue* queue);

req_queue* queue_init();

req request_init(int rid, int pid, char* name, int sh_sem_id, packet p, int num_requests);

#endif //IPC_QUEUE_H
