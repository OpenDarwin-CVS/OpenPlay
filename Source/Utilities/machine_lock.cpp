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

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#include "OPUtils.h"
#include "machine_lock.h"



//----------------------------------------------------------------------------------------
// machine_acquire_lock
//----------------------------------------------------------------------------------------

NMBoolean 
machine_acquire_lock(machine_lock *lockPtr)
{
#if defined(macintosh_build)
	return OTAcquireLock(lockPtr);
#elif defined(__OPENTRANSPORT__)
	return OTAcquireLock(lockPtr);
#elif defined(windows_build)
	op_assert(lockPtr);
	return lockPtr->acquire();
#elif defined(posix_build)
	op_assert(lockPtr);
	return lockPtr->acquire();
#else
	#error "undefined machine_acquire_lock";
#endif
}

//----------------------------------------------------------------------------------------
// machine_acquire_lock
//----------------------------------------------------------------------------------------

void
machine_clear_lock(machine_lock *lockPtr)
{
#if defined(macintosh_build)
	OTClearLock(lockPtr);
#elif defined(__OPENTRANSPORT__)
	OTClearLock(lockPtr);
#elif defined(windows_build)
	op_assert(lockPtr);
	lockPtr->release();
#elif defined (posix_build)
	op_assert(lockPtr);
	lockPtr->release();
#else
	#error "undefined machine_clear_lock";
#endif
}
