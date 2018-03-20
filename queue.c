//
// Created by Zeyu Chen, Yaohong Wu on 2/19/18.
//

#define DEBUG 0



#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>



#include "queue.h"


int Enqueue(req* ele, req_queue* queue) { // return -1 if fail
    if (isEmpty(queue)) {
#if DEBUG
        printf("Current queue is empty...\n");
#endif
        ele->next = NULL;
        queue->tail = ele;
        queue->head = ele;
    }
    else {
#if DEBUG
        printf("Current queue is not empty...\n");
#endif
        ele->next = NULL;
        queue->tail->next = ele;
        queue->tail = ele;
    }
    
    queue->count++;
#if DEBUG
    printf("Request: %d  from pid: %d ,Enqueued...\n", ele->rid, ele->pid);
#endif
}

req* Dequeue(req_queue* queue) {          // return NULL if fail
    req* ele = queue->head;
    
    if (isEmpty(queue)) {
        printf("Empty queue! Wrong dequeue call!\n");
        return NULL;   
    }
    else {
        if (ele->next == NULL) {
#if DEBUG
            printf("Current queue will be empty after dequeue.\n" );
#endif
            queue->count--;
            queue->head = NULL;
            queue->tail = NULL;
            return ele;
        }
        else {
#if DEBUG
            printf("Current queue will not be empty after dequeue.\n" );
#endif
            queue->count--;
            queue->head = queue->head->next;
            return ele;

        }
    }
    
}

int queueSize(req_queue* queue) {
    return queue->count;
}
int isEmpty(req_queue* queue) { // 1 is empty, 0 is not empty
    return queueSize(queue) ? 0 : 1;
}

req_queue* queue_init() {
#if DEBUG
    printf("Queue is about to created...\n");
#endif
    req_queue* q = (req_queue*) malloc(sizeof(req_queue));
    q->head = q->tail = NULL;
    q->count = 0;
#if DEBUG
    printf("Queue is created...\n");
#endif
    return q;
}

req request_init(int rid, int pid, char* name, int sh_sem_id, packet p, int num_requests) {
    req r;
    r.rid = rid;
    r.pid = pid;
    //r.name = name;
    memcpy(r.name, name, MAX_SIZE_NAME);
    r.addr = NULL; // used in mmap()
    r.sh_sem_id = sh_sem_id;
    r.p = p;
    r.num_requests = num_requests;
    r.next = NULL;
    return r;
}