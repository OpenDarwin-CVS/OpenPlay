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

#ifndef __OTENUMERATORS__
#define __OTENUMERATORS__

//	------------------------------	Includes

	#include "Enumerators.h"
	#include "AppleTalkNavigationAPI.h"

//	------------------------------	Public Types

	class TZoneList;
	class AppleTalkLookup;

	class COTZonesEnumerator : public CZonesEnumerator
	{
		public:
		
								COTZonesEnumerator();
			virtual				~COTZonesEnumerator();
						
			virtual OSStatus	Initialize(ATPortRef port, OTNotifyUPP notifier, void* contextPtr);		
			virtual OSStatus	GetCount(NMBoolean* allFound, NMUInt32* count);
			virtual OSStatus	GetMachineZone(StringPtr zoneName);
			virtual OSStatus	GetIndexedZone(OneBasedIndex index, StringPtr zoneName);
			virtual OSStatus	Sort();

		private:
		
			static pascal void 	ZonesListNotifier(void* contextPtr, ATEventCode code, OSStatus result, void* cookie);
			
			TZoneList*			fZonesList;
			OTNotifyUPP			fNotifier;
			void*				fNotifierContext;
	};

	class COTEntitiesEnumerator : public CEntitiesEnumerator
	{
	public:
	
							COTEntitiesEnumerator();
		virtual				~COTEntitiesEnumerator();
							
		virtual OSStatus	Initialize(ATPortRef port, OTNotifyUPP notifier, void* contextPtr);		
		virtual OSStatus	StartLookup(Str32 zone, Str32 type, Str32 prefix, NMBoolean clearPreviousResults);
		virtual OSStatus	GetCount(NMBoolean* allFound, NMUInt32* count);
		virtual OSStatus	GetIndexedEntity(OneBasedIndex index, StringPtr zone, StringPtr type, StringPtr name, ATAddress* address);
		virtual OSStatus	Sort();
		virtual OSStatus	CancelLookup();


	private:
	
		AppleTalkLookup* fLookup;
	};




#endif