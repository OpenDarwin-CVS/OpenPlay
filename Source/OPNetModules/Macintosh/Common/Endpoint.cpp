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
 */

//	------------------------------	Includes

#ifndef __OPENPLAY__
#include 			"OpenPlay.h"
#endif
#include "Endpoint.h"

//	------------------------------	Private Definitions
//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


//----------------------------------------------------------------------------------------
// Endpoint::Endpoint
//----------------------------------------------------------------------------------------

Endpoint::Endpoint(NMEndpointRef inRef, NMUInt32 inMode)
{
	mCallback = NULL;
	mRef = inRef;
	mTimeout = 5000;		// Default to a 5 second timeout
	mMode = inMode;
	mState = kDead;
	mAreForwardingQueries = false;
	mNetSprocketMode = false;
	
}

//----------------------------------------------------------------------------------------
// Endpoint::~Endpoint
//----------------------------------------------------------------------------------------

Endpoint::~Endpoint()
{
}

//----------------------------------------------------------------------------------------
// Endpoint::InstallCallback
//----------------------------------------------------------------------------------------

NMErr
Endpoint::InstallCallback(NMEndpointCallbackFunction *inCallback, void *inContext)
{
NMErr	err = kNMNoError;
	
	mCallback = inCallback;
	mContext = inContext;
	
	return err;
}

//----------------------------------------------------------------------------------------
// Endpoint::SetQueryForwarding
//----------------------------------------------------------------------------------------

NMBoolean
Endpoint::SetQueryForwarding(NMBoolean setTo)
{
	NMBoolean prev = mAreForwardingQueries;

	mAreForwardingQueries = setTo;

	return prev;
}
