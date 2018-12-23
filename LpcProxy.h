#ifndef __LPC_PROXY_H__

#define __LPC_PROXY_H__

extern void Init(void);
extern int OpenFile(char* path, int flags);
extern int ReadFile(int fd, void* pBuf, int size);
extern int WriteFile(int fd, void* pBuf, int size);
extern int CloseFile(int fd);
extern int MakeDirectory(char* path, int mode);
extern int GetString(void* (*callback)(char*));

#endif 