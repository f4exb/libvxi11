/*
 * This code contains modifications to clnt_broadcast available in the
 * rpc.subproj/pmap_rmt.c.  A summary of modifications:
 *
 *  1.  Allow blocking time to be set by the user, the previous hard-coded
 *  times were not general enough. 
 *  2.  Calls PORTMAP_GETPORT to determine if there's a service that satisfies
 *  the input specifications 
 *  3.  The clnt_broadcast has been renamed to clnt_find_services.
 *
 *  M. Marino November 2011
 *
 */
/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include "clnt_find_services.h"
#include <rpc/xdr.h>
#include <arpa/inet.h>
#include <rpc/pmap_rmt.h>
#include <rpc/pmap_prot.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#define MAX_BROADCAST_SIZE 1400

int
getbroadcastnets(struct in_addr *addrs, int sock, char *buf)
{
	struct ifconf ifc;
        struct ifreq ifreq, *ifr;
	struct sockaddr_in *sin;
        char *cp, *cplim;
        int i = 0;

        ifc.ifc_len = UDPMSGSIZE;
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
                perror("broadcast: ioctl (get interface configuration)");
                return (0);
        }
#if defined __APPLE__ /* This should really be BSD */
#define max(a, b) (a > b ? a : b)
#define size(p)	max((p).sa_len, sizeof(p))
#endif
	cplim = buf + ifc.ifc_len; /*skip over if's with big ifr_addr's */
	for (cp = buf; cp < cplim;
#if defined __APPLE__ /* This should really be BSD */
			cp += sizeof (ifr->ifr_name) + size(ifr->ifr_addr)) {
#else
			cp += sizeof (*ifr)) {
#endif
		ifr = (struct ifreq *)cp;
		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;
		ifreq = *ifr;
                if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
                        perror("broadcast: ioctl (get interface flags)");
                        continue;
                }
                if ((ifreq.ifr_flags & IFF_BROADCAST) &&
		    (ifreq.ifr_flags & IFF_UP)) {
			sin = (struct sockaddr_in *)&ifr->ifr_addr;
#ifdef SIOCGIFBRDADDR   /* 4.3BSD */
			if (ioctl(sock, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
				addrs[i++] =
				    inet_makeaddr(inet_netof(sin->sin_addr),
				    INADDR_ANY);
			} else {
				addrs[i++] = ((struct sockaddr_in*)
				  &ifreq.ifr_addr)->sin_addr;
			}
#else /* 4.2 BSD */
			addrs[i++] = inet_makeaddr(inet_netof(sin->sin_addr),
			    INADDR_ANY);
#endif
		}
	}
	return (i);
}


enum clnt_stat 
#if (defined __LP64__) && (defined __APPLE__) /* 64-bit APPLE machines */
clnt_find_services(uint32_t prog, uint32_t vers, uint32_t proc, 
#else
clnt_find_services(u_long prog, u_long vers, u_long proc, 
#endif
                   struct timeval *t, resultfoundproc_t found_callback)
{
	enum clnt_stat stat;
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, nets;
	unsigned int fromlen;
	register int sock;
	int on = 1;
	fd_set mask;
	fd_set readfds;
	register int i;
	bool_t done = FALSE;
	uint32_t xid;
#if (defined __LP64__) && (defined __APPLE__)
	unsigned int port;
#else
	unsigned long port;
#endif

	struct in_addr addrs[20];
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	char outbuf[MAX_BROADCAST_SIZE], inbuf[UDPMSGSIZE];
	int rfd;

	stat = RPC_SUCCESS;

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Cannot create socket for broadcast rpc");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#ifdef SO_BROADCAST
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		perror("Cannot set socket option SO_BROADCAST");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#endif /* def SO_BROADCAST */
	FD_ZERO(&mask);
	FD_SET(sock, &mask);
	nets = getbroadcastnets(addrs, sock, inbuf);
	memset((char *)&baddr, 0, sizeof (baddr));
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(PMAPPORT);
	baddr.sin_addr.s_addr = htonl(INADDR_ANY);
/*	baddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); */

	rfd = open("/dev/random", O_RDONLY, 0);
	if ((rfd < 0) || (read(rfd, &xid, sizeof(xid)) != sizeof(xid)))
	{
		gettimeofday(t, (struct timezone *)0);
		xid = getpid() ^ t->tv_sec ^ t->tv_usec;
	}
	if (rfd > 0) close(rfd);

	msg.rm_xid = xid;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_GETPORT;
	msg.rm_call.cb_cred = _null_auth;
	msg.rm_call.cb_verf = _null_auth;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = (xdrproc_t)xdr_void;
	a.args_ptr = NULL;
	a.arglen = 0;
	r.port_ptr = &port;
	r.xdr_results = (xdrproc_t)xdr_void;
	r.results_ptr = NULL;
        r.resultslen = 0;
	xdrmem_create(xdrs, outbuf, MAX_BROADCAST_SIZE, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);
	/*
	 * Basic loop: broadcast a packet and wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	for (i = 0; i < nets; i++) {
		baddr.sin_addr = addrs[i];
		if (sendto(sock, outbuf, outlen, 0,
			(struct sockaddr *)&baddr,
			sizeof (struct sockaddr)) != outlen) {
			perror("Cannot send broadcast packet");
			stat = RPC_CANTSEND;
			goto done_broad;
		}
	}
	if (found_callback == NULL ) {
		stat = RPC_SUCCESS;
		goto done_broad;
	}
recv_again:
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = (caddr_t)&r;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
        port = 0;
	readfds = mask;
	switch (select(sock+1, &readfds, NULL, NULL, t)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			goto done_broad;
        
		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			perror("Broadcast select problem");
			stat = RPC_CANTRECV;
			goto done_broad;
        
	}  /* end of select results switch */
try_again:
	fromlen = sizeof(struct sockaddr);
	inlen = recvfrom(sock, inbuf, UDPMSGSIZE, 0, (struct sockaddr *)&raddr, &fromlen);
	if (inlen < 0) {
		if (errno == EINTR)
			goto try_again;
		fprintf(stderr,"Cannot receive reply to broadcast: %s",strerror(errno));
		stat = RPC_CANTRECV;
		goto done_broad;
	}
#if (defined __LP64__) && (defined __APPLE__)
	if (inlen < (int)sizeof(uint32_t))
		goto recv_again;
#else
	if (inlen < (int)sizeof(u_long))
		goto recv_again;
#endif
	/*
	 * see if reply transaction id matches sent id.
	 * If so, decode the results.
	 */
	xdrmem_create(xdrs, inbuf, (u_int)inlen, XDR_DECODE);
        if (xdr_replymsg(xdrs, &msg) && 
            xdr_u_long(xdrs, &port)  && 
            port != 0) {
		if ((msg.rm_xid == xid) &&
		(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
		(msg.acpted_rply.ar_stat == SUCCESS)) {
			raddr.sin_port = htons((u_short)port);
			done = found_callback(&raddr);
		}
		/* otherwise, we just ignore the errors ... */
	} else {
#ifdef notdef
		/* some kind of deserialization problem ... */
		if (msg.rm_xid == xid)
			fprintf(stderr, "Broadcast deserialization problem");
		/* otherwise, just random garbage */
#endif
	}
	xdrs->x_op = XDR_FREE;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
	(void)xdr_replymsg(xdrs, &msg);
	xdr_destroy(xdrs);
	if (done) {
		stat = RPC_SUCCESS;
		goto done_broad;
	} else {
		goto recv_again;
	}
done_broad:
	(void)close(sock);
	return (stat);
}
