//
// Created by Zeyu Chen, Yaohong Wu on 2/19/18.
//

#ifndef IPC_SERVER_H
#define IPC_SERVER_H

#include <sys/types.h>

#include "queue.h"

#define LOWKEY 0X01
#define MEDKEY 0X02
#define HIGKEY 0X04

#define LQSZIE 0x01
#define MQSIZE 0X02
#define HQSIZE 0x04

typedef struct __server {
    int pid;

    // priority queue key
    int lowqid;
    int medqid;
    int higqid;
    // semaphore key for priority queue
    int lowsid;
    int medsid;
    int higsid;

} server;

server* server_init(int pid);
void server_start(server* s);

int sem_init(key_t key, int nsems);

void request_handler(req* request);
void request_schedule(server* s);
int func(int x, int p);

#endif //IPC_SERVER_H
