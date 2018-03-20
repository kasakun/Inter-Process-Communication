//
// Created by Yaohong Wu, Zeyu Chen on 2/22/18.
//

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "client.h"

#define SIZE 4096

// initialize a client and its requests
client* client_init(int pid, int priority, int async, int num_requests){
	client* cp = (client*)malloc(sizeof(client));
	cp->pid = pid;
	cp->priority = priority;
	cp->async = async;
	//cp->num_requests = num_requests;
	
	sprintf(cp->shm_name, "/shm_%d", pid);
	cp->rq = queue_init();
	requests_init(cp, num_requests);
	
	printf("client finishes initialization...\n");
	return cp;
}

int* call_service_sync(client* cp){
	printf("Calling service sync...\n");
	assert(cp->async == 0);
	
	get_expected_result(cp);
	
	int priority = cp->priority;
	
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = -1;
 	sb.sem_flg = SEM_UNDO;
	
	int sem_id = semget(priority, 1, 0666);
	int msgq_id;

	msgbuf msg;
	msg.mtype = 1;
	
	printf("Locking msg queue semaphore\n");
	if(semop(sem_id, &sb, 1) == -1){
		printf("Error in semop\n");
		exit(1);
	}

	if((msgq_id = msgget(priority, 0666 | IPC_CREAT)) == -1)
	{
		printf("Error in msgget\n");
		exit(1);
	}

	while(!isEmpty(cp->rq)){
		req* r = Dequeue(cp->rq);
		msg.r = *r;
		msg.r.next = NULL;  // next pointer makes no sense for another process (different address space!)
		if(msgsnd(msgq_id, &msg, sizeof(msgbuf), MSG_NOERROR) == -1){  // send request to message queue
			printf("Error in msgsend\n");
		}
	}
	
	printf("Unlocking msg queue semaphore\n");
	sb.sem_op = 1;
	if(semop(sem_id, &sb, 1)==-1){
		printf("Error in semop\n");
		exit(1);
	}
	
	wait_for_result_sync(cp);  //blocking
	
	return cp->results;
}

int* wait_for_result_sync(client* cp){
	int n = cp->num_requests;
	
	int fd;
	do{
        fd = shm_open(cp->shm_name, O_RDWR, 0666);    
	}while(fd < 0);
    
	int* head = (int*)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	while(*(head + n) < n);  // blocking
	
	if(close(fd) == -1){
	    printf("Error in close file descriptor!\n");
	}
	
	memcpy(cp->results, head, sizeof(int) * n);
	
	// reset elements in shared memory
	memset(head, 0, sizeof(int)*(n+1));
	
	return cp->results;
}

request_handle call_service_async(client* cp){
    printf("Calling service async...\n");
	assert(cp->async == 1);
	
	get_expected_result(cp);
	int priority = cp->priority;
	
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = -1;
 	sb.sem_flg = SEM_UNDO;
	
	int sem_id = semget(priority, 1, 0666);
	int msgq_id;

	msgbuf msg;
	msg.mtype = 1;
	
	printf("Locking msg queue semaphore\n");
	if(semop(sem_id, &sb, 1) == -1){
		printf("Error in semop\n");
		exit(1);
	}

	if((msgq_id = msgget(priority, 0666 | IPC_CREAT)) == -1)
	{
		printf("Error in msgget\n");
		exit(1);
	}

	while(!isEmpty(cp->rq)){
		req* r = Dequeue(cp->rq);
		msg.r = *r;
		msg.r.next = NULL;  // next pointer makes no sense for another process (different address space!)
		if(msgsnd(msgq_id, &msg, sizeof(msgbuf), MSG_NOERROR) == -1){  // send request to message queue
			printf("Error in msgsend\n");
		}
	}
	
	printf("Unlocking msg queue semaphore\n");
	sb.sem_op = 1;
	if(semop(sem_id, &sb, 1)==-1){
		printf("Error in semop\n");
		exit(1);
	}
	
	int fd;
	do{
	    fd = shm_open(cp->shm_name, O_RDWR, 0666);
	}while(fd < 0);
	
	int* head = (int*)mmap(NULL, SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	request_handle rh = {head, cp->num_requests};
	return rh;
}

int has_received_all_responses(request_handle rh){
    if (*(rh.head + rh.n) < rh.n) return 0;
	else return 1;
}

void get_results(request_handle rh, int* results){
    int* head = rh.head;
	int n = rh.n;
	memcpy(results, head, sizeof(int)*n);
	memset(head, 0, sizeof(int)*(n+1));
}


int do_something(){
	//printf("I am doing something!\n");
	for(int i = 0; i < 1000000; ++i){
	    rand();
	}
	//printf("I finish doing something!\n");
	return 0;
}

int* get_expected_result(client* cp){
    int n = cp->num_requests;
	int* results = (int*)malloc(sizeof(int) * n);
	req* ptr = cp->rq->head;
	int i = 0;
	while(ptr){
	    int x = ptr->p.x;
		int p = ptr->p.p;
		int r = (2 * x) % p;
        printf("Expected result of request %d (x = %d, p = %d) = %d...\n", i, x, p, r);
        ptr = ptr->next;
        i++;
	}
	return results;
}

void client_exit(client* cp){
    if(shm_unlink(cp->shm_name) == -1){
	    printf("Error in shm_unlink!\n");
		exit(1);
	}
	free(cp->rq);
	free(cp);
}

void requests_init(client* cp, int num_requests){
	assert(num_requests >= 0 && num_requests < SIZE / sizeof(int));
	cp->num_requests = num_requests;
    for(int i = 0; i < num_requests; ++i){
	    packet pkt;
		pkt.x = rand() % MAX_X + 1;  // x >= 1
		pkt.p = rand() % MAX_P + 1;  // p >= 1
        req* rp = (req*)malloc(sizeof(req));
		*rp = request_init(i, cp->pid, cp->shm_name, cp->priority, pkt, num_requests);  //sh_sem_id is priority
		Enqueue(rp, cp->rq);
	}
	printf("finish initializing a group of requests!\n");
}
