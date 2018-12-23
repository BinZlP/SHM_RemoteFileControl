#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/wait.h>

#include "Lpc.h"
#include "LpcProxy.h"

#define SIZE 256
#define PROCESS 5

int check = 0;

void* KeyCallback(char* str)
{
	printf("%s\n", str);
	check=1;
}
int testcase1() {

	int i;
	char dirName[SIZE];
	char pBuf[SIZE];
	char arr[5][255] = { "I_LOVE_SOSIL","I_LOVE_OS","SIGONG_JOA","LINUX_JOA","TOVALDS_JOA" };

	if (MakeDirectory("./etc", 0755) == -1) {
		return -1;
	}

	if (MakeDirectory("./temp", 0755) == -1) {
		return -1;
	}



	for (i = 0; i < 5; i++)
	{
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./etc/%d", i);

		if (MakeDirectory(dirName, 0755) == -1) {
			return -1;
		}
	}

	char path[SIZE] = "./temp";
	for (i = 0; i < 5; i++)
	{
		char buf[3];	
		sprintf(buf, "/%d", i);
		strcat(path, buf);

		if (MakeDirectory(path, 0755) == -1) {
			return -1;
		}
	}

	// create file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./etc/%d/file%d", i % 5, i);
		if ((fd = OpenFile(dirName, O_CREAT)) == -1) { 
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	//write file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./etc/%d/file%d", i % 5, i);

		if ((fd = OpenFile(dirName, O_WRONLY)) == -1) {
			return -1;
		}

		strcpy(buf, arr[i % 5]);

		if (WriteFile(fd, buf, sizeof(buf)) == -1) {
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	//read file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		char pBuf[SIZE];

		memset(dirName, 0, SIZE);
		sprintf(dirName, "./etc/%d/file%d", i % 5, i);
		if ((fd = OpenFile(dirName, O_RDONLY)) == -1) { 
			return -1;
		}

		if (ReadFile(fd, pBuf, sizeof(pBuf)) == -1) {
			return -1;
		}

		strcpy(buf, arr[i % 5]);

		if (strcmp(pBuf, buf) != 0) {
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}


	return 1;
}

int prototype(char* t) {

	int i;
	char dirName[SIZE];
	char pBuf[SIZE];
	char arr[5][255] = { "I_LOVE_SOSIL","I_LOVE_OS","SIGONG_JOA","LINUX_JOA","TOVALDS_JOA" };

	char s1[SIZE];
	char s2[SIZE];
	sprintf(s1,"./%sA",t);
	sprintf(s2, "./%sB", t);

	if (MakeDirectory(s1, 0755) == -1) {
		return -1;
	}

	if (MakeDirectory(s2, 0755) == -1) {
		return -1;
	}



	for (i = 0; i < 5; i++)
	{
		memset(dirName, 0, SIZE);
		sprintf(dirName, "%s/%d",s1,i);
		usleep(300000);
		if (MakeDirectory(dirName, 0755) == -1) {
			return -1;
		}
	}


	for (i = 0; i < 5; i++)
	{
		char buf[3];		
		sprintf(buf, "/%d", i);
		strcat(s2, buf);
		usleep(300000);
		if (MakeDirectory(s2, 0755) == -1) {
			return -1;
		}
	}

	// create file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		memset(dirName, 0, SIZE);
		sprintf(dirName, "%s/%d/%s_file%d",s1, i % 5,t,i);
		usleep(300000);
		if ((fd = OpenFile(dirName, O_CREAT)) == -1) { 
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	//write file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		memset(dirName, 0, SIZE);
		sprintf(dirName, "%s/%d/%s_file%d",s1,i % 5,t, i);

		if ((fd = OpenFile(dirName, O_WRONLY)) == -1) { 
			return -1;
		}

		strcpy(buf, arr[i % 5]);
		usleep(300000);
		if (WriteFile(fd, buf, sizeof(buf)) == -1) { 
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	//read file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		char pBuf[SIZE];

		memset(dirName, 0, SIZE);
		sprintf(dirName, "%s/%d/%s_file%d",s1, i % 5,t, i);
		if ((fd = OpenFile(dirName, O_RDONLY)) == -1) { 
			return -1;
		}
		usleep(300000);
		if (ReadFile(fd, pBuf, sizeof(pBuf)) == -1) {
			return -1;
		}

		strcpy(buf, arr[i % 5]);

		if (strcmp(pBuf, buf) != 0) {
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}


	return 1;
}

int testcase2() {
	pid_t pids[PROCESS];
	int i;
	int status;

	for (int i = 0; i < PROCESS; i++) {
		if ((pids[i] = fork()) == 0) {
			char id[20];
			sprintf(id, "%d", getpid());
			if (prototype(id) == 1) {
				exit(10 + i);
			}
			else {
				exit(100);
			}
		}
	}

	for (int i = 0; i < PROCESS; i++) {
		pid_t wpid = waitpid(pids[i], &status, 0);
		if (WEXITSTATUS(status) == 100) {
			return -1;
		}
	}



	return 1;
}
int testcase3(){	// modified : void -> int

	int i;
	
	char dirName[SIZE];
	char pBuf[SIZE];
	char arr[5][255] = { "I_LOVE_SOSIL","I_LOVE_OS","SIGONG_JOA","LINUX_JOA","TOVALDS_JOA" };

	if (MakeDirectory("./usr", 0755) == -1) {
		return -1;
	}

	if (MakeDirectory("./bin", 0755) == -1) {
		return -1;
	}
	

	GetString(KeyCallback);
	
	for (i = 0; i < 5; i++)
	{
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./usr/%d", i);

		if (MakeDirectory(dirName, 0755) == -1) {
			return -1;
		}
	}

	char path[SIZE] = "./bin";
	for (i = 0; i < 5; i++)
	{
		char buf[3];
		sprintf(buf, "/%d", i);
		strcat(path, buf);

		if (MakeDirectory(path, 0755) == -1) {
			return -1;
		}
	}

	// create file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./usr/%d/file%d", i % 5, i);
		if ((fd = OpenFile(dirName, O_CREAT)) == -1) { 
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	while(1){

		if(check==1)
		{
			check =0;
			break;
		}
	}
	
	//write file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		memset(dirName, 0, SIZE);
		sprintf(dirName, "./usr/%d/file%d", i % 5, i);

		if ((fd = OpenFile(dirName, O_WRONLY)) == -1) {
			return -1;
		}

		strcpy(buf, arr[i % 5]);

		if (WriteFile(fd, buf, sizeof(buf)) == -1) {
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	GetString(KeyCallback);
	
	//read file
	for (int i = 0; i < 20; i++)
	{
		int fd;
		char buf[SIZE];
		char pBuf[SIZE];

		memset(dirName, 0, SIZE);
		sprintf(dirName, "./usr/%d/file%d", i % 5, i);
		if ((fd = OpenFile(dirName, O_RDONLY)) == -1) { 
			return -1;
		}

		if (ReadFile(fd, pBuf, sizeof(pBuf)) == -1) {
			return -1;
		}

		strcpy(buf, arr[i % 5]);

		if (strcmp(pBuf, buf) != 0) {
			return -1;
		}

		if (CloseFile(fd) == -1) {
			return -1;
		}
	}

	
	while(1){	
		if(check==1)
		{
			check=0;
			break;
		}

	}

	return 1;

}
int main(int argc, char** argv) {
	Init();
	if (argc != 2)
	{
		printf("usage: a.out [1-3])\n");
		return -1;
	}

	int TcNum = atoi(argv[1]);

	switch (TcNum) {
	case 1:
		if (testcase1() != 1)
			printf("testcase1 Fail!!!\n");
		else
			printf("testcase1 Success!!!\n");

		break;
	case 2:
		if (testcase2() != 1)
			printf("testcase2 Fail!!!\n");
		else
			printf("testcase2 Success!!!\n");

		break;
	case 3: 
		if(testcase3() == 1)
			printf("testcase3 Success!!\n");
		break;
	}

	return 0;
}

