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

//	------------------------------	Includes

#include "NetModule.h"
#include "OTUtils.h"
#include "OTIPEndpoint.h"
#include "SetupLibraryState.h"
#include "OTIPEnumerator.h"
#include "configuration.h"
#include "configfields.h"
#include "EndpointDisposer.h"
#include "EndpointHander.h"
#include "Exceptions.h"
#include "String_Utils.h"
#include <sioux.h>

//	------------------------------	Private Definitions

//defined in NetModule.h now
//#define kDefaultNetSprocketMode true;

//	------------------------------	Private Types

enum
{
	kInfoStrings		= 2000,
	kDITLID				= 2000
};

enum
{
	kIPModuleNameIndex	= 1,
	kCopyrightIndex		= 2
};
		
enum
{
	kJoinTitle			= 1,
	kHostLabel,
	kHostText,
	kPortLabel,
	kPortText
};

//	------------------------------	Private Variables

static const char *kIPConfigAddress = "IPaddr";
static const char *kIPConfigPort = "IPport";

static const char *kModuleName = "TCP/IP";
static const char *kModuleCopyright = "1996-1999 Apple Computer, Inc.";
static const char *kModuleIDString = "Inet";
static const Rect	kFrameSize = {0, 0, 100, 430};

static NMModuleInfo		gModuleInfo;
static IPEnumerator		*gEnumerator;
static NMSInt16			gBaseItem;
 
//	------------------------------	Private Functions

#ifdef __cplusplus
extern "C" {
#endif

	OSErr 	__myinitialize(CFragInitBlockPtr ibp);
	void 	__myterminate(void);
	OSErr  __initialize(CFragInitBlockPtr ibp);
	void  __terminate(void);
	static NMErr NMStartup(void);
	static NMErr NMShutdown(void);
	
#ifdef __cplusplus
}
#endif

static NMErr ParseConfigString(const char *inConfigStr, NMType inGameID, const char *inGameName, 
								const void *inEnumData, NMUInt32 inDataLen, NMIPConfigPriv *outConfig);
static UInt16 	GenerateDefaultPort(NMType inGameID);

//	------------------------------	Public Variables

FSSpec					gFSSpec;
NMBoolean 				gTerminating = false;


//----------------------------------------------------------------------------------------
// __myinitialize
//----------------------------------------------------------------------------------------

OSErr
__myinitialize(CFragInitBlockPtr ibp)
{
OSErr	err;

	err = __initialize(ibp);
	if (kNMNoError != err)
		return (err);
	
	// we should always be kDataForkCFragLocator when we are a shared library.
	// assuming this is is the case, we want to open our resource file so we
	// can access our resources if needed. our termination routine will close
	// the file.
	if (ibp->fragLocator.where == kDataForkCFragLocator)
	{
		gFSSpec.vRefNum = ibp->fragLocator.u.onDisk.fileSpec->vRefNum;
		gFSSpec.parID = ibp->fragLocator.u.onDisk.fileSpec->parID;
		doCopyPStrMax(ibp->fragLocator.u.onDisk.fileSpec->name, gFSSpec.name, 64);
	}
	else
	{
		return (-1);
	}

	err = NMStartup();

	if (kNMNoError != err)
	{
		DEBUG_PRINT("NMStartup returned error: %ld\n", err);
	}

	//set up sioux if we're using it - they won't be able to move the window, so stick it somewhere
	//unique..
	SIOUXSettings.userwindowtitle = "\pNetModuleIP debug output";
	SIOUXSettings.toppixel = 350;
	SIOUXSettings.leftpixel = 50;
	SIOUXSettings.setupmenus = false;
	SIOUXSettings.standalone = false;

	return (err);
}

//----------------------------------------------------------------------------------------
// __myterminate
//----------------------------------------------------------------------------------------

void
__myterminate(void)
{
	
	NMShutdown();	
	__terminate();	//  "You're Terminated."
					// because we use OT's global new (and delete), we must call
					//	CloseOpenTransport only as the very last thing.  This includes
					//	the global static destructor chain from _terminate

	//	Query- what if other modules have OT open, too?
	OTUtils::CloseOpenTransport();
}

//----------------------------------------------------------------------------------------
// NMStartAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStartAdvertising(NMEndpointRef inEndpoint)
{
NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *) inEndpoint;

	return (epRef->ep->SetQueryForwarding(true));
}

//----------------------------------------------------------------------------------------
// NMStopAdvertising
//----------------------------------------------------------------------------------------

NMBoolean
NMStopAdvertising(NMEndpointRef inEndpoint)
{
NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *) inEndpoint;

	return (epRef->ep->SetQueryForwarding(false));
}

//----------------------------------------------------------------------------------------
// NMStartup
//----------------------------------------------------------------------------------------

// Called by the App (or whatever) to set thing up
NMErr
NMStartup(void)
{
NMErr	status = kNMNoError;

	gModuleInfo.size= sizeof (NMModuleInfo);
	gModuleInfo.type = kModuleID;
	strcpy(gModuleInfo.name, kModuleName);
	strcpy(gModuleInfo.copyright, kModuleCopyright);
	gModuleInfo.maxPacketSize = 1500;		// <WIP> This isn't right for internet
	gModuleInfo.maxEndpoints = kNMNoEndpointLimit;
	gModuleInfo.flags= kNMModuleHasStream | kNMModuleHasDatagram | kNMModuleHasExpedited | kNMModuleRequiresIdleTime;
	
	//	this is a flag to tell any closing endpoints that the module is about to be unloaded, and they
	//		better hurry up and die
	EndpointDisposer::sLastChance = false;

	//	Make sure we have the necessary libraries available for this module

	status = OTUtils::StartOpenTransport(OTUtils::kOTCheckForTCPIP,5000,2500);

	// prepare the dead endpoint list
	gDeadEndpoints->fHead = NULL;

	return (status);
}

//----------------------------------------------------------------------------------------
// NMShutdown
//----------------------------------------------------------------------------------------

// Called only when the module is being unloaded.  
NMErr
NMShutdown(void)
{
	gTerminating = true;

/*#ifndef OP_PLATFORM_MAC_CARBON_FLAG
	if (OTEndpoint::sServiceEPCacheSystemTask)
		OTDestroySystemTask(OTEndpoint::sServiceEPCacheSystemTask);
#endif*/

	//	See if there are any endpoints still trying to close.  If so, give them a moment
	//	this is totally OT dependent, but who cares?  We're never going to do a classic version...	
	if (! OTEndpoint::sZombieList.IsEmpty())
	{
	UnsignedWide	startTime;
	NMUInt32			elapsedMilliseconds;
		
		op_vpause("Stragglers are still trying to close, let's give them a moment...");
		
		//	signal that the party's over, and anyone left should die NOW!
		EndpointDisposer::sLastChance = true;
		
		OTGetTimeStamp(&startTime); 

		while (! OTEndpoint::sZombieList.IsEmpty()) 
		{
			elapsedMilliseconds = OTElapsedMilliseconds(&startTime);

			if (elapsedMilliseconds > 5000)
			{
				op_vpause("Timed out waiting for endpoints to finish. OT is probably hosed.");
				break;
			}
		}

	}

	// cleanup any remaining endpoints [gg]
	EndpointHander::CleanupEndpoints();

	
	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NMOpen
//----------------------------------------------------------------------------------------

// Open creates the endpoint and either listens or makes a connection
NMErr
NMOpen(NMConfigRef inConfig, NMEndpointCallbackFunction *inCallback, void *inContext, NMEndpointRef *outEndpoint, NMBoolean inActive)
{
	DEBUG_ENTRY_EXIT("NMOpen");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef = NULL;
	NMIPConfigPriv		*theConfig = (NMIPConfigPriv *) inConfig;

	op_vassert_return((outEndpoint != NULL),"outEndpoint is NIL!",kNMParameterErr);
	op_vassert_return((inConfig != NULL),"Config is NIL!",kNMParameterErr);
	op_vassert_return((inCallback != NULL),"Callback function is NIL!",kNMParameterErr);

	*outEndpoint = NULL;	// assume an error

	//	if this is a passive endpoint, we need to ignore the address specified in the
	//	config
	if (! inActive)
	{
		theConfig->address.fHost = 0;
	}
	
	//	Alloc, but don't initialize the NMEndpoint
	status = MakeNewIPEndpointPriv(inConfig, inCallback, inContext, theConfig->connectionMode, false, &epRef);	// If this returns an error, there's nothing to clean up
	
	if (kNMNoError != status)
		return (status);
		
	//	Either connect or listen (passive = listen, active = connect)
	if (inActive)
	{
		status = epRef->ep->Connect();
		if (status)
			goto error;
	}
	else
	{
		status = epRef->ep->Listen();
		if (status)
			goto error;
	}

	*outEndpoint = (NMEndpointRef) epRef;
	
	if (inActive)
		epRef->ep->mState = Endpoint::kRunning;
	else
		epRef->ep->mState = Endpoint::kListening;
		
error:
	return (status);
}

//----------------------------------------------------------------------------------------
// NMClose
//----------------------------------------------------------------------------------------

//	Closes a connection (or listener) and deletes the endpoint
NMErr
NMClose(NMEndpointRef inEndpoint, NMBoolean inOrderly)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMIPEndpointPriv	*epRef;
Endpoint *ep;

	UNUSED_PARAMETER(inOrderly);

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = (Endpoint *) epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep->Close();

	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NMAcceptConnection
//----------------------------------------------------------------------------------------

NMErr
NMAcceptConnection(
	NMEndpointRef				inEndpoint,
	void						*inCookie, 
	NMEndpointCallbackFunction	*inCallback,
	void						*inContext)
{
	DEBUG_ENTRY_EXIT("NMAcceptConnection");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint			*ep;
	NMIPEndpointPriv	*newEPRef;
	
	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inCookie != NULL),"Cookie is NIL!",kNMParameterErr);
	op_vassert_return((inCallback != NULL),"Callback function is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	// First, we need to create a new endpoint that will be returned to the user
	status = MakeNewIPEndpointPriv(NULL, inCallback, inContext, ep->mMode, ep->mNetSprocketMode, &newEPRef);

	if( !status )
		status = ep->AcceptConnection(newEPRef->ep, inCookie);	// Hands off the new connection async

	return (status);
}

//----------------------------------------------------------------------------------------
// NMRejectConnection
//----------------------------------------------------------------------------------------

NMErr
NMRejectConnection(NMEndpointRef inEndpoint, void *inCookie)
{
	DEBUG_ENTRY_EXIT("NMRejectConnection");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inCookie != NULL),"Cookie is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = (Endpoint *) epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	status = ep->RejectConnection(inCookie);			
	return (status);
}

//----------------------------------------------------------------------------------------
// NMGetModuleInfo
//----------------------------------------------------------------------------------------

// Returns info about the module
NMErr
NMGetModuleInfo(NMModuleInfo *outInfo)
{
NMErr	err = kNMNoError;

	op_vassert_return((outInfo != NULL),"NMModuleInfo is NIL!",kNMParameterErr);

	if (outInfo->size >= sizeof (NMModuleInfo))
	{
	NMSInt16	size_to_copy = (outInfo->size<gModuleInfo.size) ? outInfo->size : gModuleInfo.size;
	
		machine_move_data(&gModuleInfo, outInfo, size_to_copy);
	}
	else
	{
		err = (kNMModuleInfoTooSmall);
	}
	
	return (err);
}

//----------------------------------------------------------------------------------------
// NMFreeAddress
//----------------------------------------------------------------------------------------

NMErr
NMFreeAddress(NMEndpointRef inEndpoint, void **outAddress)
{
	DEBUG_ENTRY_EXIT("NMFreeAddress")

	NMErr				err = noErr;
	NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *)inEndpoint;
	OTEndpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (OTEndpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->FreeAddress( outAddress );

	return( err );
}

//----------------------------------------------------------------------------------------
// NMGetAddress
//----------------------------------------------------------------------------------------

NMErr
NMGetAddress(NMEndpointRef inEndpoint, NMAddressType addressType, void **outAddress)
{
	DEBUG_ENTRY_EXIT("NMGetAddress")

	NMErr				err = noErr;
	NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *)inEndpoint;
	OTEndpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (OTEndpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->GetAddress( addressType, outAddress );

	return( err );
}

//----------------------------------------------------------------------------------------
// NMIsAlive
//----------------------------------------------------------------------------------------

NMBoolean
NMIsAlive(NMEndpointRef inEndpoint)
{
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;
	NMBoolean			result = false;
	NMErr				status = kNMNoError;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = (Endpoint *) epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	result = ep->IsAlive();
	return (result);
}

//----------------------------------------------------------------------------------------
// NMSetTimeout
//----------------------------------------------------------------------------------------

NMErr
NMSetTimeout(NMEndpointRef inEndpoint, NMUInt32 inTimeout)
{
NMErr				err = kNMNoError;
NMIPEndpointPriv	*epRef;
Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inTimeout >= 1),"Timeout of less than 1 millisecond requested!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = (Endpoint *) epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	ep->SetTimeout(inTimeout);	
	return (kNMNoError);
}

//----------------------------------------------------------------------------------------
// NMIdle
//----------------------------------------------------------------------------------------

NMErr
NMIdle(NMEndpointRef inEndpoint)
{
	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	OTEndpoint::ServiceEPCaches();
	EndpointHander::CleanupEndpoints();

	status = ep->Idle();
	return (status);
}

//----------------------------------------------------------------------------------------
// NMFunctionPassThrough
//----------------------------------------------------------------------------------------

NMErr
NMFunctionPassThrough(
	NMEndpointRef	inEndpoint,
	NMUInt32			inSelector,
	void			*inParamBlock)
{
	DEBUG_ENTRY_EXIT("NMFunctionPassThrough");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	status = ep->FunctionPassThrough(inSelector, inParamBlock);
	return (status);
}

//----------------------------------------------------------------------------------------
// NMSendDatagram
//----------------------------------------------------------------------------------------

NMErr
NMSendDatagram(
	NMEndpointRef	inEndpoint,
	NMUInt8			*inData,
	NMUInt32			inSize,
	NMFlags			inFlags)
{
	DEBUG_ENTRY_EXIT("NMSendDatagram");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((inSize > 0),"Data size < 1!",kNMParameterErr);
	if (inSize > gModuleInfo.maxPacketSize)
		#if DEBUG
		{
			char error[256];
			sprintf(error,"Data size (%d) greater than max data size! (%d)",inSize,gModuleInfo.maxPacketSize);
			op_vhalt(error);
		}
		#else
			return kNMTooMuchDataErr;
		#endif //DEBUG

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	status = ep->SendDatagram(inData, inSize, inFlags);
	return (status);
}

//----------------------------------------------------------------------------------------
// NMReceiveDatagram
//----------------------------------------------------------------------------------------

NMErr
NMReceiveDatagram(
	NMEndpointRef	inEndpoint,
	NMUInt8			*ioData,
	NMUInt32			*ioSize,
	NMFlags			*outFlags)
{
	DEBUG_ENTRY_EXIT("NMReceiveDatagram");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ioData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((ioSize != NULL),"Data size pointer is NULL!",kNMParameterErr);
	op_vassert_return((outFlags != NULL),"Flags pointer is NULL!",kNMParameterErr);
	op_vassert_return((*ioSize >= 1),"Data size < 1!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	status = ep->ReceiveDatagram(ioData, ioSize, outFlags);
	return (status);
}

//----------------------------------------------------------------------------------------
// NMSend
//----------------------------------------------------------------------------------------

NMSInt32
NMSend(
	NMEndpointRef	inEndpoint,
	void			*inData,
	NMUInt32			inSize,
	NMFlags			inFlags)
{
	DEBUG_ENTRY_EXIT("NMSend");

	NMSInt32			rv;	
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;
	NMErr				status = kNMNoError;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((inData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((inSize > 0),"Data size < 1!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	//	Send returns the number of bytes actually sent, or an error
	rv = ep->Send(inData, inSize, inFlags);
	return (rv);
}

//----------------------------------------------------------------------------------------
// NMReceive
//----------------------------------------------------------------------------------------

NMErr
NMReceive(
	NMEndpointRef	inEndpoint,
	void			*ioData,
	NMUInt32			*ioSize,
	NMFlags			*outFlags)
{
	DEBUG_ENTRY_EXIT("NMReceive");

	NMErr				status = kNMNoError;
	NMIPEndpointPriv	*epRef;
	Endpoint 			*ep;

	op_vassert_return((inEndpoint != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((ioData != NULL),"Data pointer is NULL!",kNMParameterErr);
	op_vassert_return((ioSize != NULL),"Data size pointer is NULL!",kNMParameterErr);
	op_vassert_return((outFlags != NULL),"Flags pointer is NULL!",kNMParameterErr);
	op_vassert_return((*ioSize >= 1),"Data size < 1!",kNMParameterErr);

	epRef = (NMIPEndpointPriv *) inEndpoint;
	ep = epRef->ep;

	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);
		
	status = ep->Receive(ioData, ioSize, outFlags);
	return (status);
}

//	--------------------	NMEnterNotifier
//	Calls the "Enter Notifier" function on the requested endpoint (stream or datagram).
NMErr
NMEnterNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMErr				err = noErr;
NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *)inEndpoint;
Endpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (Endpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->EnterNotifier(endpointMode);
	
	return err;
}


//	--------------------	NMLeaveNotifier
//	Calls the "Leave Notifier" function on the requested endpoint (stream or datagram).
NMErr
NMLeaveNotifier(NMEndpointRef inEndpoint, NMEndpointMode endpointMode)
{
	DEBUG_ENTRY_EXIT("NMClose");

NMErr				err = noErr;
NMIPEndpointPriv	*epRef = (NMIPEndpointPriv *)inEndpoint;
Endpoint 			*ep;

	op_vassert_return((epRef != NULL),"Endpoint is NIL!",kNMParameterErr);
	op_vassert_return((epRef->id == kModuleID),"Endpoint id is not mine!",kNMParameterErr);

	ep = (Endpoint *) epRef->ep;
	op_vassert_return((ep != NULL),"Private endpoint is NIL!",kNMParameterErr);

	err = ep->LeaveNotifier(endpointMode);
	
	return err;
}

//----------------------------------------------------------------------------------------
// NMCreateConfig
//----------------------------------------------------------------------------------------

NMErr
NMCreateConfig(
	char		*inConfigStr,
	NMType		inGameID,
	const char	*inGameName,
	const void	*inEnumData,
	NMUInt32		inDataLen,
	NMConfigRef	*outConfig)
{
	DEBUG_ENTRY_EXIT("NMCreateConfig");

	NMErr			status = kNMNoError;
	NMIPConfigPriv	*config = NULL;
	
	if (inConfigStr == NULL)
	{
		op_vassert_return((inGameName != NULL),"Game name is NIL!",kNMParameterErr);
		op_vassert_return((inGameID != 0),"Game ID is 0.  Use your creator type!",kNMParameterErr);
	}

	op_vassert_return((outConfig != NULL),"outConfig pointer is NIL!",kNMParameterErr);
		
	config = new NMIPConfigPriv;
	if (config == NULL){
		status = kNMOutOfMemoryErr;
		goto error;
	}

	status = ParseConfigString(inConfigStr, inGameID, inGameName, inEnumData, inDataLen, config);
	if (status)
	{
		delete config;
		goto error;
	}
	else
		*outConfig = (NMConfigRef) config;

error:
	return (status);
}

//----------------------------------------------------------------------------------------
// NMGetConfigLen
//----------------------------------------------------------------------------------------

NMSInt16
NMGetConfigLen(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMGetConfigLen");

NMSInt16	configLen = 1024;
char		configString[1024];	// yes, a hack.
NMErr		err;
	
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",0);

	err = NMGetConfig(inConfig, configString, &configLen);
	DEBUG_PRINT("our config: %s\nlen: %d\nstrlen: %d",configString,configLen,strlen(configString));
	
	return (configLen + 1);
}

//----------------------------------------------------------------------------------------
// NMGetConfig
//----------------------------------------------------------------------------------------

NMErr
NMGetConfig(
	NMConfigRef	inConfig,
	char		*outConfigStr,
	NMSInt16	*ioConfigStrLen)
{
	DEBUG_ENTRY_EXIT("NMGetConfig");

	NMIPConfigPriv *theConfig = (NMIPConfigPriv *) inConfig;
	char			hostName[kMaxHostNameLen + 1];
	NMBoolean		putToken;
	NMUInt32		port;
	NMUInt32		tokenLen;
	NMErr			status = kNMNoError;

	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((outConfigStr != NULL),"outConfigStr is NULL!",kNMParameterErr);
	op_vassert_return((theConfig->type == kModuleID),"Config ref doesn't belong to this module!",kNMInvalidConfigErr);
	op_vassert_return((theConfig->version <= kVersion),"Config ref belongs to a later version of this module!",kNMNewerVersionErr);
	op_vassert_return((ioConfigStrLen != NULL),"ioConfigStrLen is NULL!",kNMParameterErr);
		
	//Try_
	{
		
		*outConfigStr = '\0';

		//	Write the type
		tokenLen = sizeof (NMType);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigModuleType, LONG_DATA, &theConfig->type, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the version
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigModuleVersion, LONG_DATA, &theConfig->version, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the game id
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigGameID, LONG_DATA, &theConfig->gameID, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the name
		tokenLen = strlen(theConfig->name);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigGameName, STRING_DATA, &theConfig->name, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the connection mode
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigEndpointMode, LONG_DATA, &theConfig->connectionMode, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the NetSprocket mode
		tokenLen = sizeof (NMBoolean);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigNetSprocketMode, BOOLEAN_DATA, &theConfig->netSprocketMode, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the custom data, if any
		tokenLen = theConfig->customEnumDataLen;

		if (tokenLen)
		{
			putToken = put_token(outConfigStr, *ioConfigStrLen, kConfigCustomData, BINARY_DATA, &theConfig->customEnumData, tokenLen);

			op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);
		}

		//	Convert out host addr into a string
		OTInetHostToString(theConfig->address.fHost, hostName);

		//	Write the name
		tokenLen = strlen(hostName);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kIPConfigAddress, STRING_DATA, hostName, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		//	Write the port
		port = theConfig->address.fPort;
		tokenLen = sizeof (NMUInt32);
		putToken = put_token(outConfigStr, *ioConfigStrLen, kIPConfigPort, LONG_DATA, &port, tokenLen);

		op_vassert_return((putToken),"put_token returned false.  We need a bigger config string!",kNMInvalidConfigErr);

		*ioConfigStrLen = strlen(outConfigStr);
		
		return kNMNoError;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMDeleteConfig
//----------------------------------------------------------------------------------------

NMErr
NMDeleteConfig(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMDeleteConfig");

NMIPConfigPriv *theConfig = (NMIPConfigPriv *) inConfig;
	
	op_vassert_return((inConfig != NULL),"Config ref is NULL!",kNMParameterErr);

	delete theConfig;
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMSetupDialog
//----------------------------------------------------------------------------------------

NMErr
NMSetupDialog(NMDialogPtr dialog, short frame,  short inBaseItem,NMConfigRef config)
{
	DEBUG_ENTRY_EXIT("NMSetupDialog");

	NMErr				status = kNMNoError;
	Handle				ourDITL;
	NMIPConfigPriv		*theConfig = (NMIPConfigPriv *) config;
	SetupLibraryState	duh;	// Make sure we're accessing the right resource fork
	Str255				hostName;
	Str255				portText;

	NMSInt16			kind;
	Handle				h;
	Rect				r; 
		
	SetTempPort			port(dialog);
	
	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);
		
	//Try_
	{
		gBaseItem = inBaseItem;

		//	Try to load in our DITL.  If we fail, we should bail
		ourDITL = Get1Resource('DITL', kDITLID);
		//ThrowIfNil_(ourDITL);
		if (ourDITL == NULL){
			status = err_NilPointer;
			goto error;
		}
		
		if (ourDITL == NULL)
		{
			NMSInt16 err = ResError();
			return kNMResourceErr;
		}	

		//	Append our DITL relative to the frame by passing the negative of the frame's id
		AppendDITL(dialog, ourDITL, -frame);
		ReleaseResource(ourDITL);

		//	Setup our dialog info.
		if (theConfig->address.fHost != 0)
		{
			//	Try to get the canonical name
			status = OTUtils::MakeInetNameFromAddress(theConfig->address.fHost, (char *) hostName);
			
			//	if that fails, just use the string version of the dotted quad
			if (status != kNMNoError)
				OTInetHostToString(theConfig->address.fHost, (char *) hostName);

			c2pstr((char *) hostName);				
		}
		else
		{
			doCopyPStr("\p0.0.0.0", hostName);
		}

		//	get the port
		NumToString(theConfig->address.fPort, portText);
		
		GetDialogItem(dialog, gBaseItem + kHostText, &kind, &h, &r);
		SetDialogItemText(h, hostName);

		GetDialogItem(dialog, gBaseItem + kPortText, &kind, &h, &r);
		SetDialogItemText(h, portText);

		return kNMNoError;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return kNMResourceErr;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMHandleEvent
//----------------------------------------------------------------------------------------

NMBoolean
NMHandleEvent(
	NMDialogPtr	dialog,
	NMEvent		*event,
	NMConfigRef	config)
{
NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) config;
SetTempPort		port(dialog);

	UNUSED_PARAMETER(event);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",false);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",false);
	op_vassert_return((event != NULL),"event ptr is NULL!",false);

	return false;
}

//----------------------------------------------------------------------------------------
// NMHandleItemHit
//----------------------------------------------------------------------------------------

NMErr
NMHandleItemHit(NMDialogPtr dialog, short inItemHit, NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMHandleItemHit");

NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) inConfig;
NMSInt16		theRealItem = inItemHit - gBaseItem;
SetTempPort		port(dialog);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",kNMParameterErr);
	
	switch (theRealItem)
	{
		case kPortText:
		case kHostText:
		default:
			break;
	}

	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// NMTeardownDialog
//----------------------------------------------------------------------------------------

NMBoolean
NMTeardownDialog(
	NMDialogPtr	dialog,
	NMBoolean	inUpdateConfig,
	NMConfigRef	ioConfig)
{
	DEBUG_ENTRY_EXIT("NMTeardownDialog");

NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) ioConfig;
NMErr		status;
InetAddress		addr;
Str255			hostText;
Str255			portText;

NMSInt16		kind;
Rect 			r;
Handle 			h;
NMSInt32		theVal;
SetTempPort		port(dialog);

	op_vassert_return((theConfig != NULL),"Config ref is NULL!",true);
	op_vassert_return((dialog != NULL),"Dialog ptr is NULL!",true);

	if (inUpdateConfig)
	{
		// 	Get the host text	
		GetDialogItem(dialog, gBaseItem + kHostText, &kind, &h, &r);
		GetDialogItemText(h, hostText);

		p2cstr(hostText);

		//	resolve it into a host addr
		status = OTUtils::MakeInetAddressFromString((const char *)hostText, &addr);
		op_warn(status == kNMNoError);
		
		if (status != kNMNoError)
			return false;
			
		theConfig->address = addr;
		
		//	Get the port text
		GetDialogItem(dialog, gBaseItem + kPortText, &kind, &h, &r);
		GetDialogItemText(h, portText);
		StringToNum(portText, &theVal);

		if (theVal != 0)
			theConfig->address.fPort = theVal;
	}
	
	return true;
}

//----------------------------------------------------------------------------------------
// NMGetRequiredDialogFrame
//----------------------------------------------------------------------------------------

void
NMGetRequiredDialogFrame(NMRect *r, NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMGetRequiredDialogFrame");

	UNUSED_PARAMETER(inConfig);

	//	I don't know of any reason why the config is going to affect the size of the frame
	*r = kFrameSize;
}

//----------------------------------------------------------------------------------------
// NMStartEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMStartEnumeration(NMConfigRef inConfig, NMEnumerationCallbackPtr inCallback, void *inContext, NMBoolean inActive)
{
	DEBUG_ENTRY_EXIT("NMStartEnumeration");

	NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) inConfig;
	NMErr		status = kNMNoError;
	
	op_vassert_return((theConfig != NULL),"Config ref is NULL!",kNMParameterErr);
	op_vassert_return((inCallback != NULL),"Callback function ptr is NULL!",kNMParameterErr);

	//Try_
	{
		if (gEnumerator != NULL)
			return kNMAlreadyEnumeratingErr;
		
		if (OTUtils::OTInitialized())
			gEnumerator = new OTIPEnumerator(theConfig, inCallback, inContext, inActive);
		else
		{
			gEnumerator = NULL;		// Fill in when we have classic support
			DEBUG_PRINT("We don't have classic IP support yet");
		}			
		//ThrowIfNil_(gEnumerator);
		if (gEnumerator == NULL){
			status = err_NilPointer;
			goto error;
		}
		

		status = gEnumerator->StartEnumeration();
		//ThrowIfOSErr_(status);
		if (status)
			goto error;
		
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		delete gEnumerator;
		gEnumerator = NULL;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMIdleEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMIdleEnumeration(NMConfigRef inConfig)
{
	NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) inConfig;
	NMErr			status = kNMNoError;
	
	if (! gEnumerator)
		return kNMNotEnumeratingErr;
	
	//Try_
	{	
		return gEnumerator->IdleEnumeration();
	}
	//catch(...)
	error:
	if (status)
	{
		NMErr code = status;
		//DEBUG_PRINT("An exception was thrown, but not caught.  This is bad.");
		return kNMInternalErr;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMEndEnumeration
//----------------------------------------------------------------------------------------

NMErr
NMEndEnumeration(NMConfigRef inConfig)
{
	DEBUG_ENTRY_EXIT("NMEndEnumeration");

	NMIPConfigPriv	*theConfig = (NMIPConfigPriv *) inConfig;
	NMErr			status = kNMNoError;
	
	if (! gEnumerator)
		return kNMNotEnumeratingErr;

	//Try_
	{
		status = gEnumerator->EndEnumeration();
		//ThrowIfOSErr_(status);
		if (status)
			goto error;
		
		delete gEnumerator;
		gEnumerator = NULL;
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		delete gEnumerator;
		gEnumerator = NULL;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// NMBindEnumerationItemToConfig
//----------------------------------------------------------------------------------------

NMErr
NMBindEnumerationItemToConfig(NMConfigRef inConfig, NMHostID inID)
{	
	DEBUG_ENTRY_EXIT("NMBindEnumerationItemToConfig");

	NMErr	status = kNMNoError;

	if (gEnumerator == NULL)
		return kNMNotEnumeratingErr;
		
	//Try_
	{
		status = gEnumerator->SetConfigFromEnumerationItem((NMIPConfigPriv *)inConfig, (IPEnumerationItemPriv *) inID);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;
				
		return status;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// ParseConfigString
//----------------------------------------------------------------------------------------

static NMErr
ParseConfigString(
	const char		*inConfigStr,
	NMType			inGameID,
	const char		*inGameName, 
	const void		*inEnumData,
	NMUInt32			inDataLen,
	NMIPConfigPriv	*outConfig)
{
	NMErr	status = kNMNoError;
	NMInetPort	port = GenerateDefaultPort(inGameID);
	InetHost	host = 0x7f000001;		// loopback address
	NMBoolean	gotToken;
	NMSInt32	tokenLen;
	char		workString[kMaxHostNameLen + 1];
	NMUInt32		p;

	outConfig->type = kModuleID;
	outConfig->version = kVersion;

	outConfig->gameID = inGameID;
	outConfig->connectionMode = kNMNormalMode;

	if (inGameName)
	{
		strncpy(outConfig->name, inGameName, kMaxGameNameLen);
		outConfig->name[kMaxGameNameLen]= 0;
	}
	else
	{
		strcpy(outConfig->name, "unknown");
	}

	if (inDataLen)
	{
		if (inDataLen > kNMEnumDataLen)
			return kNMInvalidConfigErr;

		op_assert(inDataLen <= kNMEnumDataLen);
		machine_move_data(inEnumData, outConfig->customEnumData, inDataLen);
		outConfig->customEnumDataLen = inDataLen;
	}		
	else
	{
		outConfig->customEnumDataLen = 0;
	}
	
	if (OTUtils::OTInitialized())
	{
		OTInitInetAddress((InetAddress *)&outConfig->address, port, host);
	}
	else
	{
		op_vpause("Classic not supported!");
	}

	//  default netSprocketMode
	outConfig->netSprocketMode = kDefaultNetSprocketMode;

	//	If we got a null string, just create a default config
	if (inConfigStr == NULL)
		return kNMNoError;
	 
	//Try_
	{	
		//	Generic module information

		//	Read the type
		tokenLen = sizeof (NMType);
		gotToken = get_token(inConfigStr, kConfigModuleType, LONG_DATA, &outConfig->type, &tokenLen);
		if (!gotToken)
			outConfig->type = kModuleID;

		//	Read the version
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigModuleVersion, LONG_DATA, &outConfig->version, &tokenLen);

		if (!gotToken)
			outConfig->version = kVersion;

		//	Read the game id
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigGameID, LONG_DATA, &outConfig->gameID, &tokenLen);

		if (!gotToken)
			outConfig->gameID = 0;

		//	Read the game name
		if (inGameName == NULL || inGameName[0] == 0)
		{
			tokenLen = kMaxGameNameLen;
			gotToken = get_token(inConfigStr, kConfigGameName, STRING_DATA, &outConfig->name, &tokenLen);

			if (!gotToken)
				strcpy(outConfig->name, "unknown");
		}
		
		//	Read the connection mode
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kConfigEndpointMode, LONG_DATA, &outConfig->connectionMode, &tokenLen);

		if (! gotToken)
			outConfig->connectionMode = kNMNormalMode;

		//	Read the netSprocketMode mode
		tokenLen = sizeof (NMBoolean);
		gotToken = get_token(inConfigStr, kConfigNetSprocketMode, BOOLEAN_DATA, &outConfig->netSprocketMode, &tokenLen);

		if (!gotToken)
			outConfig->netSprocketMode = kDefaultNetSprocketMode;	

		//	read the custom data, if any
		if (inDataLen == 0)
		{
			tokenLen = kNMEnumDataLen;
			gotToken = get_token(inConfigStr, kConfigCustomData, BINARY_DATA, &outConfig->customEnumData, &tokenLen);

			if (gotToken)		
				outConfig->customEnumDataLen = tokenLen;
		}	
		
		//	IP Module-specific information

		//	Read the dotted quad IP address
		tokenLen = kMaxHostNameLen;
		gotToken = get_token(inConfigStr, kIPConfigAddress, STRING_DATA, workString, &tokenLen);

		if (! gotToken)
		{
			outConfig->address.fHost = host;
		}
		else
		{
			//	This is OT-dependent, and could cause the PPP module to dial-in.
			//	It will also fail if DNS is not available
			status = OTUtils::MakeInetAddressFromString(workString, &outConfig->address);
		}

		if (outConfig->address.fPort == 0)
			outConfig->address.fPort = port;
			
		//	Read the port.  We don't care if it's not present.  It might have been in the address
		tokenLen = sizeof (NMUInt32);
		gotToken = get_token(inConfigStr, kIPConfigPort, LONG_DATA, &p, &tokenLen);

		if (gotToken)
			outConfig->address.fPort = p;
		
		return kNMNoError;
	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return code;
	}
	return status;
}

//----------------------------------------------------------------------------------------
// MakeNewIPEndpointPriv
//----------------------------------------------------------------------------------------

NMErr
MakeNewIPEndpointPriv(
	NMConfigRef					inConfig,
	NMEndpointCallbackFunction	*inCallback, 
	void						*inContext,
	NMUInt32						inMode,
	NMBoolean					inNetSprocketMode,
	NMIPEndpointPriv			**theEP)
{
	NMErr				status = kNMNoError;
	Endpoint			*ep = NULL;
	NMIPConfigPriv		*config = (NMIPConfigPriv *) inConfig;
	NMUInt32				mode = (config == NULL) ? inMode : config->connectionMode;
	
	//Try_
	{	
				
		*theEP = new NMIPEndpointPriv;
		//ThrowIfNULL_(*theEP);
		if (*theEP == NULL){
			status = err_NilPointer;
			goto error;
		}

		ep = new OTIPEndpoint((NMEndpointRef) *theEP, mode);
		//ThrowIfNULL_(ep);
		if (ep == NULL){
			status = err_NilPointer;
			goto error;
		}
		
		//  Set NetSprocket mode
		ep->mNetSprocketMode = (config == NULL) ? inNetSprocketMode : config->netSprocketMode;
		
		//	Install the user's callback function
		status = ep->InstallCallback(inCallback, inContext);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;
		
		//	Allow the endpoint to do one-time init stuff
		status = ep->Initialize(inConfig);
		//ThrowIfOSErr_(status);
		if (status)
			goto error;

		(*theEP)->ep = ep;
		(*theEP)->id = kModuleID;
		
	}
	//Catch_(code)
	error:
	if (status)
	{			
		NMErr code = status;
		delete *theEP;
		delete ep;
		
		*theEP = NULL;
		return code;
	}
	
	return kNMNoError;
}

//----------------------------------------------------------------------------------------
// DisposeIPEndpointPriv
//----------------------------------------------------------------------------------------

void
DisposeIPEndpointPriv(NMIPEndpointPriv *inEP)
{
	if (! inEP)
		return;
		
	delete inEP->ep;	// Delete the IP endpoint
	delete inEP; 		// delete user's version
}

//----------------------------------------------------------------------------------------
// GenerateDefaultPort
//----------------------------------------------------------------------------------------

static UInt16
GenerateDefaultPort(NMType inGameID)
{
	return (inGameID % (32760 - 1024)) + 1024;
}

//----------------------------------------------------------------------------------------
// CloseEPComplete
//----------------------------------------------------------------------------------------

// Just disposes of the endpoint.  The user has already been notified
void
CloseEPComplete(NMEndpointRef inEPRef)
{
	DisposeIPEndpointPriv((NMIPEndpointPriv *) inEPRef);
}

