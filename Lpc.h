#ifndef __LPC_H__

#define __LPC_H__

#define LPC_DATA_MAX    (512) 
#define LPC_ARG_MAX     (3)


typedef enum 
{
    LPC_OPEN_FILE = 0,
    LPC_READ_FILE = 1,
    LPC_WRITE_FILE = 2,
    LPC_CLOSE_FILE = 3,
    LPC_MAKE_DIRECTORY = 4,
    LPC_REMOVE_DIRECTORY = 5,
    LPC_GET_STRING
} LpcService;



typedef struct __LpcArg
{
    int argSize;
    char argData[LPC_DATA_MAX];    
} LpcArg;

typedef struct __LpcRequest
{
    long pid;               // message type
    LpcService service;
    int numArg;
    LpcArg lpcArgs[LPC_ARG_MAX];
} LpcRequest;

typedef struct __LpcResponse
{
    long pid;			// message type
    int errorno;
    int responseSize;
    char responseData[LPC_DATA_MAX];
} LpcResponse;


#endif 
