//
// Created by Zeyu Chen, Yaohong Wu on 2/19/18.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>

#include "server.h"
#include "client.h"

#define TEST 0
#define OUTPUT 1
#define SIZE 4096

#define SYNCMODE(num) (num == 0 ? "sync" : "async")
#define KEY(num) (num == 0 ? 0x01 :((num == 1) ? 0x02 : 0x04))
typedef struct timeval timeval_t;

int output(pid_t pid, int  priority, int sync_mode, timeval_t tv1, timeval_t tv2) {
    FILE* fp;
    char filename[100];
    char append[10];
    double interval;

    strcpy(filename, "./output/client_");
    sprintf(append, "%d", getpid());
    strcat(filename, append);
    strcat(filename, ".txt");
    //printf("%s", filename);
    fp = fopen(filename, "w");
    if (fp == -1) {
        printf("Open fail! \n");
        exit(1);
    }
    interval = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
    fprintf(fp, "Client %d, priority: 0x%x, sync mode:%s, lifetime:%.4fs", pid, KEY(priority), SYNCMODE(sync_mode), interval);
}



int main() {
    int sync_mode;  // 0 for sync, 1 async
    int priority;   // high, medium, low
    server *s;
    pid_t server_pid, child_pid;

    server_pid =getpid();
    printf("Server(%d)\n", server_pid);


    int loop = 3;   // create 7 processes, 6 clients, 1 idle

    while (loop--)
        fork();

    if (getpid() != server_pid) {
        timeval_t tv1, tv2;

        sync_mode = (getpid() % 2  == 0) ? 0 : 1;
        priority = (getpid() - server_pid) % 3;
        printf("Client %d: my priority is %x, my sync mode is %s\n", getpid(), KEY(priority), SYNCMODE(sync_mode));

        sleep(5); // wait for server init..

        gettimeofday(&tv1, NULL); // all processes, mark time stamp 1
        if (sync_mode == 0) {
            int num_requests1 = 20;
            client* cp1 = client_init(getpid(), KEY(priority), sync_mode, num_requests1);
            int* results1 = cp1->results;

            results1 = call_service_sync(cp1); // sync call
            for(int i = 0; i < cp1->num_requests; ++i){
                printf("Client %d: Actual result of request %d = %d...\n", getpid(), i, results1[i]);
            }

            gettimeofday(&tv2, NULL);  //mark time stamp 2
            output(getpid(), priority, sync_mode, tv1, tv2);
            client_exit(cp1);
        }
        else {
            int num_requests2 = 20;
            client* cp2 = client_init(getpid(), KEY(priority), sync_mode, num_requests2);
            int* results2 = cp2->results;

            request_handle rh = call_service_async(cp2); // async call

            while(has_received_all_responses(rh) == 0){  // not receive all results
                do_something();
            }

            get_results(rh, results2);
            for(int i = 0; i < cp2->num_requests; ++i){
                printf("Client %d: Actual result of request %d = %d...\n", getpid(), i, results2[i]);
            }

            gettimeofday(&tv2, NULL); //mark time stamp 2
            output(getpid(), priority, sync_mode, tv1, tv2);
            client_exit(cp2);
        }
    }

    if (server_pid == getpid()) {
        s = server_init(server_pid); // sever init
        server_start(s);             // server start
    }

#if TEST
    pid_t child;

    server_pid = getpid();
    printf("Server pid: %d\n", server_pid);


    child = fork();
    printf("child: %d\n", child);
    if (getpid() == server_pid) {  // server si// WYH: use the spot after the last request to count
    *(head + request->num_requests) = request->rid + 1;	de
        s = server_init(server_pid);
        server_start(s);
    }

    if (getppid() == server_pid) {  // sample client process // !!!!!!please remember to a child process getpid() == 0! please use getppid!!!

        sleep(2); // wait for the server init...
        int msgq;
        int fd;
        int* head;

        // request 0
        msgbuf buf;
        packet p = {3, 5};  //p, x
        buf.mtype = 1;
        buf.r = request_init(0, child, "/share", HIGKEY, p);
        printf ("Child req0 expected result: %d\n", (2*buf.r.p.x)%buf.r.p.p);

        // request 1
        msgbuf buf0;
        packet p0 = {6, 2};  //p, x
        buf0.mtype = 1;
        buf0.r = request_init(1, child, "/share", HIGKEY, p0);

        printf ("Child req1 expected result: %d\n", (2*buf0.r.p.x)%buf0.r.p.p);

        /*
        * A REQUEST LIFE TIME START (SUPPOSE YOU HAVE CREATE THE FILE
        */


        // Get queue
        if ((msgq = msgget(HIGKEY, 0666 | IPC_CREAT)) == -1) {
            printf("msg get queue error!\n");
            return 1;
        }
        // send 0
        if (msgsnd(msgq, &buf, sizeof(msgbuf),MSG_NOERROR) == -1) {
            printf("send error!, %s\n", strerror(errno)); // all ele in msg must be valid
            return 1;
        }
        // send 1
        if (msgsnd(msgq, &buf0, sizeof(msgbuf),MSG_NOERROR) == -1) {
            printf("send error!, %s\n", strerror(errno)); // all ele in msg must be valid
            return 1;
        }
        //wait

        sleep(5);

        // map to the area
        fd = shm_open(buf.r.name, O_RDONLY, 0666);
        head = mmap(NULL, SIZE, PROT_READ, MAP_SHARED, fd, 0);

        if (fd == -1) {
            printf("client: OPEN FAIL, %s\n", strerror(errno));
            exit(1);
        }

        // 0
        printf("Server req0 address:%d\n", head + buf.r.rid);
        printf("Server req0 result:%d\n", *(head + buf.r.rid));

        //1
        printf("Server req1 address:%d\n", head + buf0.r.rid);
        printf("Server req1 result:%d\n", *(head + buf0.r.rid));

        if (munmap(buf.r.addr, SIZE) == -1) {
            printf("Unmap failed! %s\n", strerror(errno));
            exit(1);
        }

        /*
         * A REQUEST LIFE TIME END
         */

        printf("Test over! Kill Server..\n");
        if (getppid() == server_pid) {
            kill(getppid(), SIGKILL);  // Kill the right father
        }

        printf("Child die\n");
    }

#endif
    return 0;
}