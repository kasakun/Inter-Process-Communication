//
// Created by Yaohong Wu, Zeyu Chen on 2/22/18.
//

#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

#include "queue.h"

#define MAX_SIZE_SHM_NAME 16
#define MAX_SIZE_RESULTS 1024

#define MAX_X 100
#define MAX_P 113


typedef struct __client
{
	int pid;  // process id
	
	int priority; // one of {1: LOW, 2: MID, 4: HIG}
	int async; // one of {0: sync, 1: async}
	int num_requests;  // number of requests, assume that they have same priority and are all sync/async
	
	char shm_name[MAX_SIZE_SHM_NAME];  // name of shared memory object, a big pool of memory used for multiple requests
	req_queue* rq;  // local request queue of a client process
	int results[MAX_SIZE_RESULTS];  // local response buffer
	
} client;

typedef struct __request_handle{
	int* head;
	int n;
} request_handle;

client* client_init(int pid, int priority, int async, int num_requests);

void requests_init(client* cp, int num_requests);

int* call_service_sync(client* cp);

int* wait_for_result_sync(client* cp);

request_handle call_service_async(client* cp);

int has_received_all_responses(request_handle rh);

void get_results(request_handle rh, int* results);

int do_something();

int* get_expected_result(client* cp);

void client_exit(client* cp);

#endif