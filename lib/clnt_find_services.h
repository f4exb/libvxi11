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


#ifndef _RPC_CLNT_FIND_SERVICES_H
#define _RPC_CLNT_FIND_SERVICES_H
#include <rpc/rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr_in;
typedef bool_t (*resultfoundproc_t)(struct sockaddr_in *);

/*
 * The following is based on clnt_broadcast, with important differences that it
 * is only searching for devices which have the requested services.  There is
 * also a timeout variable which doesn't normally exist in clnt_broadcast.  If
 * found_callback is non NULL, this function is called when a device is found
 * with the requested service.
 */
enum clnt_stat 
#if (defined __LP64__) && (defined __APPLE__)
clnt_find_services(uint32_t prog, uint32_t vers, uint32_t proc, 
#else
clnt_find_services(u_long prog, u_long vers, u_long proc, 
#endif
                   struct timeval *t, resultfoundproc_t found_callback);

#ifdef __cplusplus
}
#endif

#endif /* !_RPC_CLNT_FIND_SERVICES_H */
