all: server a.out
server: server.c LpcStub.c
	gcc server.c LpcStub.c -o server -lpthread
a.out: testcase.c LpcProxy.c
	gcc testcase.c LpcProxy.c -lpthread
