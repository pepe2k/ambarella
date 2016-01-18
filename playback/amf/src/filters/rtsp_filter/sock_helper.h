#ifndef __SOCK_HELPER__
#define __SOCK_HELPER__

#include "am_types.h"

extern int SetupStreamSocket(AM_U32 local_addr,  AM_U16 local_port, AM_BOOL makeNonBlocking);
extern int SendStream(int socket, AM_U8* buffer, AM_U32 bufferSize);
extern int SetupDatagramSocket(AM_U32 localAddr,  AM_U16 localPort, AM_BOOL makeNonBlocking);
extern int SendDatagram(int socket, AM_U32 foreignAddr,  AM_U16 foreignPort, AM_U8* pBuffer, AM_U32 bufferSize);
extern int RecvDatagram(int socket, AM_U32 *pForeignAddr,  AM_U16 *pForeignPort, AM_U8 *pBuffer, AM_U32 bufferSize);
extern int GetOurIPAddress(AM_U32 *IPAddress);
extern char *HostAddressToString(AM_U32 hostIPAddress);
extern int GetIPAddressBySock(int sock, AM_U32 *pIPAddress);

#endif

