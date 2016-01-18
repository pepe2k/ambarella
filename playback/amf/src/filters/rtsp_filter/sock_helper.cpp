/*
 * sock_helper.cpp
 *
 * History:
 *    2011/8/31 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<unistd.h>
#include <ctype.h>

#include "am_types.h"
#include "sock_helper.h"

#define LISTENQ 20

static AM_INT reuseFlag = 1;

/*
 * Internet network address interpretation routine.
 * The library routines call this routine to interpret
 * network numbers.
 */
AM_UINT inet_network(register const char *cp)
{
	register AM_UINT val, base, n, i;
	register char c;
	AM_UINT parts[4], *pp = parts;
	AM_INT digit;

again:
	val = 0; base = 10; digit = 0;
	if (*cp == '0')
		digit = 1, base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		digit = 0, base = 16, cp++;
	while ((c = *cp) != 0) {
		if (isdigit(c)) {
			if (base == 8 && (c == '8' || c == '9'))
				return (INADDR_NONE);
			val = (val * base) + (c - '0');
			cp++;
			digit = 1;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (tolower (c) + 10 - 'a');
			cp++;
			digit = 1;
			continue;
		}
		break;
	}
	if (!digit)
		return (INADDR_NONE);
	if (pp >= parts + 4 || val > 0xff)
		return (INADDR_NONE);
	if (*cp == '.') {
		*pp++ = val, cp++;
		goto again;
	}
	if (*cp && !isspace(*cp))
		return (INADDR_NONE);
	if (pp >= parts + 4 || val > 0xff)
		return (INADDR_NONE);
	*pp++ = val;
	n = pp - parts;
	for (val = 0, i = 0; i < n; i++) {
		val <<= 8;
		val |= parts[i] & 0xff;
	}
	return (val);
}

inline AM_INT IsMulticastAddress(AM_U32 addressInHostOrder) {
	// Note: We return False for addresses in the range 224.0.0.0
	// through 224.0.0.255, because these are non-routable
	// Note: IPv4-specific #####
	return addressInHostOrder >=  inet_network("224.0.1.0") &&
		 addressInHostOrder <= inet_network("239.255.255.255");
}

AM_INT SetupStreamSocket(AM_U32 localAddr,  AM_U16 localPort, AM_BOOL makeNonBlocking) 
{
	struct sockaddr_in	servaddr;
	AM_INT newSocket = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(localAddr);
	servaddr.sin_port        = htons(localPort);


	if (newSocket < 0) {
		AM_ERROR("unable to create stream socket\n");
		return newSocket;
	}

	if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&reuseFlag, sizeof reuseFlag) < 0) {
		AM_ERROR("setsockopt(SO_REUSEADDR) error\n");
		close(newSocket);
		return -1;
	}

	if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
		AM_ERROR("bind() error (port number: %d)\n", localPort);
		close(newSocket);
		return -1;
	}

	if (makeNonBlocking) {
		AM_INT curFlags = fcntl(newSocket, F_GETFL, 0);
		if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) != 0) {
			AM_ERROR("failed to make non-blocking\n");
			close(newSocket);
			return -1;
		}
	}

	AM_INT requestedSize = 50*1024;
	if (setsockopt(newSocket, SOL_SOCKET, SO_SNDBUF,
		&requestedSize, sizeof requestedSize) != 0) {
		AM_ERROR("failed to set send buffer size\n");
		close(newSocket);
		return -1;
	}

	if (listen(newSocket, LISTENQ) < 0) {
		AM_ERROR("listen() failed\n");
		return -1;
	}

	return newSocket;
}

AM_INT SendStream(AM_INT socket, AM_U8* buffer, AM_U32 bufferSize)
{
	AM_INT bytesSent = send(socket, (char*)buffer, bufferSize, 0);

	if (bytesSent != (AM_INT)bufferSize) {
		AM_ERROR("sendDatagram error, expect %d, actually sent %d\n ", bufferSize, bytesSent);
		return -1;
	}

	return 0;
}

AM_INT SetupDatagramSocket(AM_U32 localAddr,  AM_U16 localPort, AM_BOOL makeNonBlocking)
{
	struct sockaddr_in	servaddr;
	AM_INT newSocket = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(localAddr);
	servaddr.sin_port        = htons(localPort);


	if (newSocket < 0) {
		AM_ERROR("unable to create stream socket\n");
		return newSocket;
	}

	if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&reuseFlag, sizeof reuseFlag) < 0) {
		AM_ERROR("setsockopt(SO_REUSEADDR) error\n");
		close(newSocket);
		return -1;
	}

	if (bind(newSocket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
		AM_ERROR("bind() error (port number: %d)\n", localPort);
		close(newSocket);
		return -1;
	}

	if (makeNonBlocking) {
		AM_INT curFlags = fcntl(newSocket, F_GETFL, 0);
		if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) != 0) {
			AM_ERROR("failed to make non-blocking\n");
			close(newSocket);
			return -1;
		}
	}

	return newSocket;
}

AM_INT SendDatagram(AM_INT socket, AM_U32 foreignAddr,  AM_U16 foreignPort, AM_U8 *pBuffer, AM_U32 bufferSize)
{
	struct sockaddr_in	destAddr;

	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family      = AF_INET;
	destAddr.sin_addr.s_addr = htonl(foreignAddr);
	destAddr.sin_port        = htons(foreignPort);

	AM_INT bytesSent = sendto(socket, (char*)pBuffer, bufferSize, 0,
					   (struct sockaddr*)&destAddr, sizeof destAddr);

	if (bytesSent != (AM_INT)bufferSize) {
		if (errno == EWOULDBLOCK)
			AM_ERROR("sendDatagram would block\n");
		else
			AM_ERROR("sendDatagram error, expect %d, actually sent %d\n ", bufferSize, bytesSent);
		return -1;
	}

	return 0;
}

AM_INT RecvDatagram(AM_INT socket, AM_U32 *pForeignAddr,  AM_U16 *pForeignPort, AM_U8 *pBuffer, AM_U32 bufferSize)
{
	struct sockaddr_in	srcAddr;
	socklen_t addrSize;

	AM_INT bytesRead = recvfrom(socket, (char*)pBuffer, bufferSize, 0,
		   (struct sockaddr*)&srcAddr, &addrSize);

	if (bytesRead <= 0) {
		AM_ERROR("RecvDatagram error");
		return -1;
	}
	AM_PRINTF("RecvDatagram success, expect %d, actually read %d\n ", bufferSize, bytesRead);

	*pForeignAddr = ntohl(srcAddr.sin_addr.s_addr);
	*pForeignPort = ntohs(srcAddr.sin_port);

	return 0;
}

AM_INT SocketJoinGroup(AM_INT socket, AM_U32 groupAddress)
{
	if (!IsMulticastAddress(groupAddress)) return 0; // ignore this case

	struct ip_mreq imr;
	imr.imr_multiaddr.s_addr = htonl(groupAddress);
	imr.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		 (const char*)&imr, sizeof (struct ip_mreq)) < 0) {
		AM_ERROR("SocketJoinGroup failed\n");
		return -1;
	}
	return 0;
}

AM_INT SocketLeaveGroup(AM_INT socket, AM_U32 groupAddress)
{
	if (!IsMulticastAddress(groupAddress)) return 0; // ignore this case

	struct ip_mreq imr;
	imr.imr_multiaddr.s_addr = htonl(groupAddress);
	imr.imr_interface.s_addr = htonl(INADDR_ANY);

	if (setsockopt(socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(const char*)&imr, sizeof (struct ip_mreq)) < 0) {
		AM_ERROR("SocketLeaveGroup failed\n");
		return -1;
	}
	return 0;
}

AM_INT GetOurIPAddress(AM_U32 *pIPAddress)
{
	AM_U16 testPort = 15947;
	AM_U32 testGroupAddress = inet_network("228.67.43.91"); // arbitrary
	AM_U16 ourPort;

	AM_INT sock = SetupDatagramSocket(INADDR_ANY, testPort, 0);
	if (sock < 0) return -1;

	if (SocketJoinGroup(sock, testGroupAddress) < 0)
		return -1;

	AM_U8 testString[] = "hostIdTest";

	if (SendDatagram(sock, testGroupAddress, testPort, testString, sizeof testString) < 0)
		return -1;

	AM_U8 readBuffer[20];
	if (RecvDatagram(sock, pIPAddress, &ourPort, readBuffer, sizeof readBuffer) < 0)
		return -1;

	return 0;
}

AM_INT GetIPAddressBySock(AM_INT sock, AM_U32 *pIPAddress)
{
	struct sockaddr_in ourSockAddress;
	socklen_t namelen = sizeof ourSockAddress;
	getsockname(sock, (struct sockaddr*)&ourSockAddress, &namelen);
	*pIPAddress = ntohl(ourSockAddress.sin_addr.s_addr);

	return 0;
}

char *HostAddressToString(AM_U32 hostIPAddress)
{
	struct in_addr sin_addr;
	sin_addr.s_addr = htonl(hostIPAddress);
	return inet_ntoa(sin_addr);
}

char *NetAddressToString(AM_U32 netIPAddress)
{
	struct in_addr sin_addr;
	sin_addr.s_addr = netIPAddress;
	return inet_ntoa(sin_addr);
}


