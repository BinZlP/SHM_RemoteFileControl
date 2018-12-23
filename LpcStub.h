#ifndef __LPC_STUB_H__

#define __LPC_STUB_H__

#include "Lpc.h"

extern void Init(void);
extern int OpenFile(LpcRequest* pRequest);
extern int ReadFile(LpcRequest* pRequest);
extern int WriteFile(LpcRequest* pRequest);
extern int CloseFile(LpcRequest* pRequest);
extern int MakeDirectory(LpcRequest* pRequest);
extern int GetString(LpcRequest* pRequest);

#endif 