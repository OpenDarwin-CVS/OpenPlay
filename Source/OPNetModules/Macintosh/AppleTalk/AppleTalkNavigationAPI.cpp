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

#include <OpenTransport.h>

#include "AppleTalkNavigationAPI.h"
#include "OTEnumerators.h"

//	------------------------------	Private Definitions

//	------------------------------	Private Types
//	------------------------------	Private Variables
//	------------------------------	Private Functions
//	------------------------------	Public Variables


#pragma export on

//----------------------------------------------------------------------------------------
// ATCreateZonesEnumerator
//----------------------------------------------------------------------------------------

OSStatus
ATCreateZonesEnumerator(ATPortRef port, OTNotifyUPP notifier, void* contextPtr, ATZonesEnumeratorRef* ref)
{
CZonesEnumerator*	enumerator	= NULL;
OSStatus			err			= kNMNoError;
		
	enumerator = new COTZonesEnumerator(); 
	
	// Initialize it
	if (enumerator != NULL)
		err = enumerator->Initialize(port, notifier, contextPtr);
	else
		err = kATEnumeratorBadReferenceErr;
	
	// Return it
	*ref = (ATZonesEnumeratorRef)enumerator;
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATGetZonesCount
//----------------------------------------------------------------------------------------

OSStatus
ATGetZonesCount(ATZonesEnumeratorRef ref, NMBoolean* done, NMUInt32* count)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CZonesEnumerator* enumerator = (CZonesEnumerator *) ref;
		
		enumerator->GetCount(done, count);
	}
	else
	{
		// Bad ref
		*done = false;
		*count = 0;
		
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATGetMachineZone
//----------------------------------------------------------------------------------------

OSStatus
ATGetMachineZone(ATZonesEnumeratorRef ref, StringPtr zoneName)
{
OSStatus err = kNMNoError;
	
	if (ref != NULL)
	{
		CZonesEnumerator* enumerator = (CZonesEnumerator*)ref;		
		enumerator->GetMachineZone(zoneName);
	}
	else
	{
		// Bad ref
		zoneName[0] = 0;
		
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATGetIndexedZone
//----------------------------------------------------------------------------------------

OSStatus
ATGetIndexedZone(ATZonesEnumeratorRef ref, OneBasedIndex index, StringPtr zoneName)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CZonesEnumerator* enumerator = (CZonesEnumerator*)ref;
		
		enumerator->GetIndexedZone(index, zoneName);
	}
	else
	{
		// Bad ref
		zoneName[0] = 0;
		
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATSortZones
//----------------------------------------------------------------------------------------

OSStatus
ATSortZones(ATZonesEnumeratorRef ref)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CZonesEnumerator* enumerator = (CZonesEnumerator*)ref;
		enumerator->Sort();
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATDeleteZonesEnumerator
//----------------------------------------------------------------------------------------

OSStatus
ATDeleteZonesEnumerator(ATZonesEnumeratorRef* ref)
{
OSStatus	err = kNMNoError;
	
	if (*ref  != NULL)
	{
		delete (CZonesEnumerator*)*ref;
		*ref = NULL;
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

#pragma mark -

//----------------------------------------------------------------------------------------
// ATCreateNBPEntitiesEnumerator
//----------------------------------------------------------------------------------------

OSStatus
ATCreateNBPEntitiesEnumerator(
	ATPortRef					port,
	OTNotifyUPP					notifier,
	void						*contextPtr,
	ATNBPEntitiesEnumeratorRef	*ref)
{
CEntitiesEnumerator*	enumerator	= NULL;
OSStatus				err			= kNMNoError;
	
	enumerator = new COTEntitiesEnumerator(); 
	
	// Initialize it
	if (enumerator != NULL)
		err = enumerator->Initialize(port, notifier, contextPtr);
	else
		err = kATEnumeratorBadReferenceErr;
	
	// Return it
	*ref = (ATNBPEntitiesEnumeratorRef)enumerator;
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATStartNBPEntitiesLookup
//----------------------------------------------------------------------------------------

OSStatus
ATStartNBPEntitiesLookup(ATNBPEntitiesEnumeratorRef ref, Str32 zone, Str32 type, Str32 prefix, NMBoolean clearPreviousResults)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CEntitiesEnumerator* enumerator = (CEntitiesEnumerator*)ref;
		err = enumerator->StartLookup(zone, type, prefix, clearPreviousResults);
	}
	else
	{
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATGetNBPEntitiesCount
//----------------------------------------------------------------------------------------

OSStatus
ATGetNBPEntitiesCount(ATNBPEntitiesEnumeratorRef ref, NMBoolean* allFound, NMUInt32* count)
{
OSStatus err = kNMNoError;
	
	if (ref != NULL)
	{
		CEntitiesEnumerator* enumerator = (CEntitiesEnumerator*)ref;
		err = enumerator->GetCount(allFound, count);
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATGetIndexedNBPEntity
//----------------------------------------------------------------------------------------

OSStatus
ATGetIndexedNBPEntity(ATNBPEntitiesEnumeratorRef ref, OneBasedIndex index, StringPtr zone, StringPtr type, StringPtr name, ATAddress* address)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CEntitiesEnumerator* enumerator = (CEntitiesEnumerator*)ref;
		err = enumerator->GetIndexedEntity(index, zone, type, name, address);
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATSortNBPEntities
//----------------------------------------------------------------------------------------

OSStatus
ATSortNBPEntities(ATNBPEntitiesEnumeratorRef ref)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CEntitiesEnumerator* enumerator = (CEntitiesEnumerator*)ref;
		
		enumerator->Sort();
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATCancelNBPEntitiesLookup
//----------------------------------------------------------------------------------------

OSStatus
ATCancelNBPEntitiesLookup(ATNBPEntitiesEnumeratorRef ref)
{
OSStatus	err = kNMNoError;
	
	if (ref != NULL)
	{
		CEntitiesEnumerator* enumerator = (CEntitiesEnumerator*) ref;
		
		enumerator->CancelLookup();
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ATDeleteNBPEntitiesEnumerator
//----------------------------------------------------------------------------------------

OSStatus
ATDeleteNBPEntitiesEnumerator(ATNBPEntitiesEnumeratorRef* ref)
{
OSStatus	err = kNMNoError;
	
	if (*ref != NULL)
	{
		delete (CEntitiesEnumerator*)*ref;
		
		*ref = NULL;
	}
	else
	{
		// Bad ref
		err = kATEnumeratorBadReferenceErr;
	}
	
	return err;
}

#pragma export off
