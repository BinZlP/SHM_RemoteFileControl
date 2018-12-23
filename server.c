#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Lpc.h"
#include "LpcStub.h"

#define PERMS 0666

#define SEM_MAX_NUM 1

#define MAX_CLIENT_NUM 5

extern int req_shmid[MAX_CLIENT_NUM];
extern int res_shmid[MAX_CLIENT_NUM];
extern char* req_shmat[MAX_CLIENT_NUM];
extern char* res_shmat[MAX_CLIENT_NUM];

extern int server_semid;
extern int client_semid;

extern int current_working_shm_no;

void signalHandler(int signum);


int lockSem(int semid){
	struct sembuf buf={0, -1, SEM_UNDO};
	return semop(semid, &buf, 1);
}



int main(int argc,const char* argv[]){
	char pBuf[LPC_DATA_MAX]={0,};
	LpcRequest* req=(LpcRequest*)malloc(sizeof(LpcRequest));
	
	signal(SIGINT, signalHandler);
	signal(SIGKILL, signalHandler);
	signal(SIGSEGV, signalHandler);

	Init();

	while(1){
		lockSem(server_semid);
		for(current_working_shm_no=0;current_working_shm_no<MAX_CLIENT_NUM;current_working_shm_no++){
			memset(req,0x00,sizeof(LpcRequest));
			memcpy(req,req_shmat[current_working_shm_no],sizeof(LpcRequest));
			
			if(req->pid==0) continue;
			else{
				//printf("[SERVER] Received message from %d - %d\n",req->pid, req->service);

				long done=0;
				memcpy(req_shmat[current_working_shm_no],&done,sizeof(done));
				switch(req->service){
				case LPC_OPEN_FILE: OpenFile(req); break;
				case LPC_READ_FILE: ReadFile(req); break;
				case LPC_WRITE_FILE: WriteFile(req); break;
				case LPC_CLOSE_FILE: CloseFile(req); break;
				case LPC_MAKE_DIRECTORY: MakeDirectory(req); break;
				}
			}
		}
		usleep(100);
	}

	free(req);
	return 0;
}

void signalHandler(int signum){
	printf("SIGNAL: %d\n",signum);
	if(signum==SIGINT||signum==SIGKILL||signum==SIGSEGV){
		for(int i=0;i<MAX_CLIENT_NUM;i++){
			shmdt(req_shmat[i]);
			shmdt(res_shmat[i]);
			shmctl(req_shmid[i],IPC_RMID,NULL);
			shmctl(res_shmid[i],IPC_RMID,NULL);
		}
		semctl(server_semid,0,IPC_RMID,0);
		semctl(client_semid,0,IPC_RMID,0);
		exit(0);
	}
}