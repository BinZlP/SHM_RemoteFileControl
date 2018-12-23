#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "Lpc.h"
#include "LpcProxy.h"

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

void wait_for_response(void* (*callback)(char*));

int req_shmid[MAX_CLIENT_NUM], res_shmid[MAX_CLIENT_NUM];
char *req_shmat[MAX_CLIENT_NUM], *res_shmat[MAX_CLIENT_NUM];
int my_shm_no=-1;
int server_semid, client_semid;
pid_t ppid;

void emptyHandler(int signum){ }

void spinLock() {
	long temp=0;
	while(!temp){
		memcpy(&temp,res_shmat[my_shm_no],sizeof(temp));
		if(temp!=0) break;
		usleep(100*1000);
	}
}

int checkMine(){
	long temp=0;
	memcpy(&temp,res_shmat[my_shm_no],sizeof(temp));
	memset(res_shmat[my_shm_no],0x00,sizeof(long));
	return temp;
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
	client_semid=semget((key_t)SEM_CLIENT_KEY,MAX_CLIENT_NUM,IPC_CREAT|PERMS);
	
	semunBuf.val=0;
	semctl(server_semid, 0, SETVAL, semunBuf);
	semctl(client_semid, 0, SETVAL, semunBuf);
	//printf("Semaphores Initialized.\n");
	
	ppid=getpid();
}

void getShmNumber(pid_t pid){
	if(my_shm_no!=-1) return;
	// struct shmid_ds buf;
	// for(int i=0; i<MAX_CLIENT_NUM; i++){
	// 	shmctl(req_shmid[i], IPC_STAT, &buf);
	// 	if(buf.shm_lpid==buf.shm_cpid ||
	// 		(int)pid-(int)(buf.shm_lpid)>=MAX_CLIENT_NUM){
	// 		my_shm_no=i;
	// 		printf("[%d] my_shm_no = %d\n", pid, my_shm_no);
	// 		return;
	// 	}
	// }
	my_shm_no=pid-ppid;
	//printf("[%d] my_shm_no = %d\n",pid,my_shm_no);
}

int lockSem(int semid){
	struct sembuf buf={0, -1, SEM_UNDO};
	return semop(semid, &buf, SEM_MAX_NUM);
}

int unlockSem(int semid){
	struct sembuf buf={0, 1, SEM_UNDO};
	return semop(semid, &buf, SEM_MAX_NUM);
}


int OpenFile(char* path, int flags){
	getShmNumber(getpid());

	int fd;
	LpcRequest req={0,};
	LpcResponse res={0,};

	req.pid=getpid();
	req.service=LPC_OPEN_FILE;
	req.numArg=2;
	req.lpcArgs[0].argSize=strlen(path);
	strcpy(req.lpcArgs[0].argData,path);
	req.lpcArgs[1].argSize=sizeof(flags);
	memcpy(req.lpcArgs[1].argData,&flags,sizeof(flags));


	memcpy(req_shmat[my_shm_no],&req,sizeof(req));
	unlockSem(server_semid);
	//printf("[%d] Request PID: %d\n",getpid(),req.pid);
	//printf("[%d] OpenFile sent request.\n", getpid());
	//printf("OpenFile sent request.\n");
	
	int isMine=0;
	while(1){
		lockSem(client_semid);
		isMine=checkMine();
		if(isMine==0){
			unlockSem(client_semid);
			usleep(100*1000);
		}
		else break;
	}
	
	//spinLock();
	memset(res_shmat[my_shm_no],0x00,sizeof(long));

	memcpy(&res,res_shmat[my_shm_no], sizeof(res));
	memset(res_shmat[my_shm_no],0x00,sizeof(long));
	memcpy(&fd,res.responseData,sizeof(int));
	//printf("[%d] OpenFile received message: %d\n",getpid(),fd);

	return fd;
}

int ReadFile(int fd, void* pBuf, int size){
	getShmNumber(getpid());
	int rSize;

	LpcRequest req={0,};
	LpcResponse res={0,};

	req.pid=getpid();
	req.service=LPC_READ_FILE;
	req.numArg=3;
	req.lpcArgs[0].argSize=sizeof(fd);
	memcpy(req.lpcArgs[0].argData,&fd,sizeof(fd));
	req.lpcArgs[1].argSize=size;
	memcpy(req.lpcArgs[1].argData,pBuf,size);
	req.lpcArgs[2].argSize=sizeof(size);
	memcpy(req.lpcArgs[2].argData,&size,sizeof(size));

	memcpy(req_shmat[my_shm_no],&req,sizeof(req));
	unlockSem(server_semid);
	//printf("[%d] ReadFile sent request.\n", getpid());
	
	//lockSem(client_semid);
	//spinLock();
	int isMine=0;
	while(1){
		lockSem(client_semid);
		isMine=checkMine();
		if(isMine==0){
			unlockSem(client_semid);
			usleep(100*1000);
		}
		else break;
	}
	memset(res_shmat[my_shm_no],0x00,sizeof(long));

	memcpy(&res,res_shmat[my_shm_no],sizeof(res));
	memcpy(pBuf,res.responseData,res.responseSize);

	rSize=res.responseSize;
	//printf("[%d] ReadFile received message: %d\n",getpid(),rSize);
	
	return rSize;
}


int WriteFile(int fd, void* pBuf, int size){
	getShmNumber(getpid());
	//printf("WriteFile start. FD: %d, Size: %d\n",fd,size);
	int wSize;

	LpcRequest req={0,};
	LpcResponse res={0,};

	req.pid=getpid();
	req.service=LPC_WRITE_FILE;
	req.numArg=3;
	req.lpcArgs[0].argSize=sizeof(fd);
	memcpy(req.lpcArgs[0].argData,&fd,sizeof(fd));
	req.lpcArgs[1].argSize=size;
	memcpy(req.lpcArgs[1].argData,pBuf,size);
	req.lpcArgs[2].argSize=sizeof(size);
	memcpy(req.lpcArgs[2].argData,&size,sizeof(size));

	memcpy(req_shmat[my_shm_no],&req,sizeof(req));
	unlockSem(server_semid);
	//printf("[%d] WriteFile sent request.\n", getpid());
	
	//lockSem(client_semid);
	//spinLock();
	int isMine=0;
	while(1){
		lockSem(client_semid);
		isMine=checkMine();
		if(isMine==0){
			unlockSem(client_semid);
			usleep(100*1000);
		}
		else break;
	}
	memset(res_shmat[my_shm_no],0x00,sizeof(long));

	memcpy(&res,res_shmat[my_shm_no],sizeof(res));
	wSize=res.responseSize;
	//printf("[%d] WriteFile received message: %d\n",getpid(),wSize);
	
	return wSize;
}


int CloseFile(int fd){
	getShmNumber(getpid());
	int errorno;
	int result;

	LpcRequest req={0,};
	LpcResponse res={0,};
	
	req.pid=getpid();
	req.service=LPC_CLOSE_FILE;
	req.numArg=1;
	req.lpcArgs[0].argSize=sizeof(fd);
	memcpy(req.lpcArgs[0].argData,&fd,sizeof(fd));

	memcpy(req_shmat[my_shm_no],&req,sizeof(req));
	unlockSem(server_semid);
	//printf("[%d] CloseFile sent request.\n", getpid());
	
	//lockSem(client_semid);
	//spinLock();
	int isMine=0;
	while(1){
		lockSem(client_semid);
		isMine=checkMine();
		if(isMine==0){
			unlockSem(client_semid);
			usleep(100*1000);
		}
		else break;
	}
	memset(res_shmat[my_shm_no],0x00,sizeof(long));

	memcpy(&res,res_shmat[my_shm_no],sizeof(res));
	errorno=res.errorno;
	memcpy(&result,res.responseData,res.responseSize);
	//printf("[%d] CloseFile received message: %d\n",getpid(),result);

	return result;
}

int MakeDirectory(char* path, int mode){
	getShmNumber(getpid());
	int result;
	int errorno;

	LpcRequest req={0,};
	LpcResponse res={0,};

	req.pid=getpid();
	req.service=LPC_MAKE_DIRECTORY;
	req.numArg=2;
	req.lpcArgs[0].argSize=strlen(path);
	strcpy(req.lpcArgs[0].argData,path);
	req.lpcArgs[1].argSize=sizeof(mode);
	memcpy(req.lpcArgs[1].argData,&mode,sizeof(mode));
	//printf("[%d] LpcRequest setting complete.\n", getpid());

	memcpy(req_shmat[my_shm_no],&req,sizeof(req));
	unlockSem(server_semid);
	//printf("[%d] Request PID: %d -> %x\n",getpid(),req.pid,req_shmat[my_shm_no]);
	//printf("[%d] MakeDirectory sent request.\n", getpid());
	
	//lockSem(client_semid);
	//spinLock();
	int isMine=0;
	while(1){
		lockSem(client_semid);
		isMine=checkMine();
		if(isMine==0){
			unlockSem(client_semid);
			usleep(100*1000);
		}
		else break;
	}
	memset(res_shmat[my_shm_no],0x00,sizeof(long));

	memcpy(&res,res_shmat[my_shm_no],sizeof(res));
	errorno=res.errorno;
	memcpy(&result,res.responseData,res.responseSize);
	//printf("[%d] MakeDirectory received message: %d\n",getpid(),result);

	return result;
}

int GetString(void* (*callback)(char*)){
	return 0;
}