/*
 * Copyright (c) 1999-2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999-2002 Apple Computer, Inc.  All Rights
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
 *
 * Modified: $Date$
 * Revision: $Id$
 */


#ifndef __MACHINELOCK__
#define __MACHINELOCK__

//	------------------------------	Includes

	#if defined(macintosh_build)
		#include <OpenTransport.h>
		#define machine_lock OTLock
	#elif defined (windows_build)
		#include "LinkedList.h"
		#define machine_lock OSCriticalSection
	#elif defined (posix_build)
		#ifdef __OPENTRANSPORT__
			#define machine_lock OTLock
		#else
			#include "LinkedList.h"
			#define machine_lock OSCriticalSection
		#endif
	#else
		#error need to define machine_lock for this platform
	#endif
	
//	------------------------------	Public Functions

	extern	NMBoolean	machine_acquire_lock(machine_lock *lockPtr);
	extern	void		machine_clear_lock(machine_lock *lockPtr);

#endif __MACHINELOCK__
