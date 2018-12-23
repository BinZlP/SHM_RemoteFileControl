#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "Lpc.h"
#include "LpcStub.h"

#define RES_SHM_KEY 2424 // server send
#define REQ_SHM_KEY 1111 // server receive

#define SEM_CLIENT_KEY 1111
#define SEM_SERVER_KEY 2222
#define SEM_MAX_NUM 1

#define MAX_CLIENT_NUM 5

#define PERMS 0666

union semun {
	int val;
	struct semid_ds* buf;
	unsigned short int* array;
};

int req_shmid[MAX_CLIENT_NUM], res_shmid[MAX_CLIENT_NUM];
char *req_shmat[MAX_CLIENT_NUM], *res_shmat[MAX_CLIENT_NUM];
int server_semid, client_semid;

int current_working_shm_no=0;

int unlockSem(int semid){
	struct sembuf buf={0, 1, SEM_UNDO};
	return semop(semid, &buf, SEM_MAX_NUM);
}

void Init(void){
	key_t req_shmkey;
	key_t res_shmkey;

	union semun semunBuf;

	
	/*
	for(int i=0; i<MAX_CLIENT_NUM; i++){
		req_shmkey=ftok(REQ_SHM_KEY, i*10);
		req_shmid[i]=shmget(req_shmkey, sizeof(LpcRequest), IPC_CREAT|PERMS);
		req_shmat[i]=shmat(req_shmid[i],NULL,0);
		printf("%p ",req_shmat[i]);

		res_shmkey=ftok(RES_SHM_KEY, i*10);
		res_shmid[i]=shmget(res_shmkey, sizeof(LpcResponse), IPC_CREAT|PERMS);
		res_shmat[i]=shmat(res_shmid[i],NULL,0);
		printf("%p\n",res_shmat[i]);
	}*/
	for(int i=0;i<MAX_CLIENT_NUM;i++){
		req_shmid[i]=shmget((key_t)REQ_SHM_KEY*(i+1), sizeof(LpcRequest), IPC_CREAT|PERMS);
		if(req_shmid[i]==-1) perror("shmget()");
		req_shmat[i]=shmat(req_shmid[i],NULL,0);
		if(req_shmat[i]==-1) perror("shmat()");
		res_shmid[i]=shmget((key_t)RES_SHM_KEY*(i+1), sizeof(LpcResponse), IPC_CREAT|PERMS);
		if(res_shmid[i]==-1) perror("shmget()");
		res_shmat[i]=shmat(res_shmid[i],NULL,0);
		if(res_shmat[i]==-1) perror("shmat()");
 		//printf("req_shmid[%d]: %x | res_shmid[%d] %x\n",i,req_shmat[i],i,res_shmat[i]);
		
	}	
	//printf("Shared Memory Initialized\n");
	//for(int i=0;i<MAX_CLIENT_NUM;i++) printf("%d ",req_shmid[i]);
	//printf("\n");

	//key_t server_semkey=ftok(SEM_SERVER_KEY, SEM_SERVER_NO);
	server_semid=semget((key_t)SEM_SERVER_KEY,1,IPC_CREAT|PERMS);
	//key_t client_semkey=ftok(SEM_CLIENT_KEY, SEM_CLIENT_NO);
	client_semid=semget((key_t)SEM_CLIENT_KEY,1,IPC_CREAT|PERMS);
	
	semunBuf.val=0;
	semctl(server_semid, 0, SETVAL, semunBuf);
	semctl(client_semid, 0, SETVAL, semunBuf);
	//printf("Semaphores Initialized.\n");
}

int OpenFile(LpcRequest* pRequest){
	LpcResponse res={0,};
	//printf("%s\n",pRequest->lpcArgs[0].argData);

	int fd, flags;
	memcpy(&flags,pRequest->lpcArgs[1].argData,pRequest->lpcArgs[1].argSize);
	fd=open(pRequest->lpcArgs[0].argData,flags,PERMS);
	if(fd==-1) perror("open()");

	res.pid=pRequest->pid;
	res.responseSize=sizeof(fd);
	memcpy(res.responseData,&fd,sizeof(fd));

	memcpy(res_shmat[current_working_shm_no],&res,sizeof(res));
	unlockSem(client_semid);

	return fd;
}

int ReadFile(LpcRequest* pRequest){
	int rsize, fd, size;
	LpcResponse res={0,};
	char tmp[512]={0,};

	memcpy(&fd,pRequest->lpcArgs[0].argData,pRequest->lpcArgs[0].argSize);
	memcpy(&size,pRequest->lpcArgs[2].argData,pRequest->lpcArgs[2].argSize);
	
	rsize=read(fd,res.responseData,size);
	if(rsize==-1) perror("read()");
	
	//memcpy(tmp,res.responseData,size);
	//printf("ReadFile: %s, %d\n", tmp, rsize);

	res.pid=pRequest->pid;
	res.responseSize=rsize;

	memcpy(res_shmat[current_working_shm_no],&res,sizeof(res));
	unlockSem(client_semid);

	return rsize;
}


int WriteFile(LpcRequest* pRequest){
	int fd,size,wsize;
	LpcResponse res={0,};

	memcpy(&fd,pRequest->lpcArgs[0].argData,pRequest->lpcArgs[0].argSize);
	memcpy(&size,pRequest->lpcArgs[2].argData,pRequest->lpcArgs[2].argSize);
	wsize=write(fd,pRequest->lpcArgs[1].argData,size);
	if(wsize==-1) perror("write()");

	res.pid=pRequest->pid;
	res.responseSize=wsize;
	
	memcpy(res_shmat[current_working_shm_no],&res,sizeof(res));
	unlockSem(client_semid);

	return wsize;
}


int CloseFile(LpcRequest* pRequest){
	int result, fd;
	LpcResponse res={0,};

	memcpy(&fd,pRequest->lpcArgs[0].argData,pRequest->lpcArgs[0].argSize);

	result=close(fd);	
	if(result==-1) perror("close()");

	res.pid=pRequest->pid;
	res.responseSize=sizeof(result);
	memcpy(res.responseData,&result,sizeof(result));
	
	memcpy(res_shmat[current_working_shm_no],&res,sizeof(res));
	unlockSem(client_semid);
	
	return result;
}

int MakeDirectory(LpcRequest* pRequest){
	int ret, mode;
	LpcResponse res={0,};
	char path[LPC_DATA_MAX]={0,};

	memcpy(&mode,pRequest->lpcArgs[1].argData,pRequest->lpcArgs[1].argSize);
	memcpy(path,pRequest->lpcArgs[0].argData,pRequest->lpcArgs[0].argSize);

	ret=mkdir(pRequest->lpcArgs[0].argData,mode);
	if(ret==-1) perror("mkdir()");

	res.pid=pRequest->pid;
	res.responseSize=sizeof(ret);
	memcpy(res.responseData,&ret,sizeof(ret));

	memcpy(res_shmat[current_working_shm_no],&res,sizeof(res));
	unlockSem(client_semid);

	return ret;
}

 