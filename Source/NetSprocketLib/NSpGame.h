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

#ifndef __NSPGAME__
#define __NSPGAME__

//	------------------------------	Includes

	#ifndef __OPENPLAY__
	#include 			"OpenPlay.h"
	#endif

	#if macintosh_build
		#include <OpenTransport.h>
	#endif

	#include "NSp_InterruptSafeList.h"
	//#include "CPlayerMapComparator.h"
	#include "ERObject.h"
	#include "PrivateMessages.h"

	#include "NSpLists_OP.h"


//	------------------------------	Public Definitions

	#define POLLING 1
	
//	------------------------------	Public Variables

	extern NMUInt32	gStandardMessageSize;
	extern NMUInt32	gQElements;
//extern NMUInt32	gBufferSize;

//	------------------------------	Public Types

// Indices into the STR# resource for this class
	enum
	{
		kUntitledGame = 1
	};

	enum
	{
		kBigMessageFlag = 0x00010000
	};

	enum { kVersion10Message = 0x10000000 };
	typedef enum {kRunning = 1, kPaused, kStopped} GameState;

	class NSpGamePrivate;

	class NSpGame
	{
	public:
		enum { class_ID = 'Game' };
		//enum { kReceiveBufferSize = 4096};
		
		NMUInt32	mObjectID;			// An ID used to confirm object type

					NSpGame(NMUInt32 inMaxPlayers, ConstStr31Param inGameName, ConstStr31Param inPassword, NSpTopology inTopology, NSpFlags inFlags);
		virtual		~NSpGame();

	//	Methods for handling message buffers
				ERObject	*GetFreeERObject(NMUInt32 inSize = gStandardMessageSize);					// ERObjects hold TNetMessage structs
				void		ReleaseERObject(ERObject *inObject);

				NMUInt8		*GetNewDataBuffer(NMUInt32	inLength);	//	Returns buffers of any length
				void		FreeDataBuffer(NMUInt8 *inBuf);

				void		FreeNetMessage(NSpMessageHeader *inMessage);
		
	//	Methods for iterating through players and getting info
		inline	NMUInt32	GetPlayerCount(void) { return mGameInfo.currentPlayers;}
		inline 	NMUInt32	GetGroupCount(void) {return mGameInfo.currentGroups;}
				OSStatus	NSpPlayer_GetInfo(NSpPlayerID inID, NSpPlayerInfoPtr *outInfo);
				void		NSpPlayer_ReleaseInfo(NSpPlayerInfoPtr inInfo);
				OSStatus	NSpPlayer_GetEnumeration(NSpPlayerEnumerationPtr *outPlayers);
				void		NSpPlayer_ReleaseEnumeration(NSpPlayerEnumerationPtr inPlayers);		
				NMUInt32	GetRTT(NSpPlayerID inPlayer);
				NMUInt32	GetThruput(NSpPlayerID inPlayer);
				
	//	Group management
		virtual	OSStatus	NSpGroup_Create(NSpGroupID *outGroupID);
		virtual	OSStatus	NSpGroup_Delete(NSpGroupID inGroupID);
		virtual	OSStatus	NSpGroup_AddPlayer(NSpGroupID inGroupID, NSpPlayerID inPlayerID);
		virtual	OSStatus 	NSpGroup_RemovePlayer(NSpGroupID inGroupID, NSpPlayerID inPlayerID);
				OSStatus	NSpGroup_GetInfo(NSpGroupID inGroupID, NSpGroupInfoPtr *outInfo);
				void		NSpGroup_ReleaseInfo(NSpGroupInfoPtr inInfo);
				OSStatus	NSpGroup_GetEnumeration(NSpGroupEnumerationPtr *outGroups);
				void		NSpGroup_ReleaseEnumeration(NSpGroupEnumerationPtr inGroups);
				NMBoolean	HandleCreateGroupMessage(TCreateGroupMessage *inMessage);
				NMBoolean	HandleDeleteGroupMessage(TDeleteGroupMessage *inMessage);
				OSStatus	DoDeleteGroup(NSpGroupID inID);
				NMBoolean	HandleAddPlayerToGroupMessage(TAddPlayerToGroupMessage *inMessage);
				NMBoolean 	HandleRemovePlayerFromGroupMessage(TRemovePlayerFromGroupMessage *inMessage);
				NMBoolean	HandlePlayerTypeChangedMessage(NSpPlayerTypeChangedMessage *inMessage);
				
	//	Methods that need to be overridden by the subclass
		virtual OSStatus	SendUserMessage(NSpMessageHeader *inMessage, NSpFlags inFlags) = 0;
		
		//ecf - allows idling op endpoints
		virtual void		IdleEndpoints(void) = 0;
		virtual OSStatus	SendTo(NSpPlayerID inTo, NMSInt32 inWhat, void *inData, NMUInt32 inLen, NSpFlags inFlags) = 0;
		virtual	void		HandleNewEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie) = 0;
		virtual OSStatus	PrepareForDeletion(NSpFlags inFlags) = 0;
				NMBoolean	NSpMessage_Get(NSpMessageHeader **outMessage);
		virtual	OSStatus	HandleEndpointDisconnected(CEndpoint *inEndpoint) = 0;
		
	//	Accessors
		inline 	NSpPlayerID	NSpPlayer_GetMyID(void) { return mPlayerID;}
		inline 	NMSInt32	GetTimeStampDifferential(void) {return mTimeStampDifferential;}
		inline	NSpGameInfo *GetGameInfo() {return &mGameInfo;}
		inline	void		SetGameInfo(const NSpGameInfo *inInfo) { mGameInfo = *inInfo;}

		inline  NSpGamePrivate *GetGameOwner() {return mOwner;}
		inline  void		   SetGameOwner(NSpGamePrivate *theOwner) {mOwner = theOwner;}
				
	//	Stuff
				NMUInt32	GetCurrentTimeStamp(void);
				void		InstallAsyncMessageHandler(NSpMessageHandlerProcPtr	inAsyncMessageHandler,
									void *inAsyncMessageContext);
				void		InstallCallbackHandler(NSpCallbackProcPtr	inCallbackHandler,
									void *inCallbackContext);
				void		GetQState(NMUInt32 *outFreeQ, NMUInt32 *outCookieQ, NMUInt32 *outMessageQ);
	protected:
	//	Methods for handling the player list
		virtual	NMBoolean	AddPlayer(NSpPlayerInfo *inInfo, CEndpoint *inEndpoint);
		virtual	NMBoolean	RemovePlayer(NSpPlayerID inPlayer, NMBoolean inDisconnect) = 0;
				PlayerListItem *GetPlayerListItem(NSpPlayerID inPlayerID);
				OSStatus	DoSelfSend(NSpMessageHeader *inMessage, void *inBody, NSpFlags inFlags, NMBoolean inCopy = true);
				void		HandleEventForSelf(ERObject *inERObject, CEndpoint *inEndpoint);
				NMBoolean	HandlePlayerLeft(NSpPlayerLeftMessage *inMessage);

	// Other private methods
				ERObject	*GetCookieERObject(void);
				void		FillInGroups(NSpPlayerID inPlayer, NSpGroupID **outGroups, NMUInt32 *outGroupCount);
				void		ReleaseGroups(NSpGroupID *inGroups);
				
		NSpGroupID						mNextAvailableGroupID;
		NMUInt32						mNextMessageID;
		
	//	User fields
		NSpFlags						mFlags;
		
		NSp_InterruptSafeList			*mPlayerList;
		NSp_InterruptSafeList			*mGroupList;
		NSp_InterruptSafeListIterator	*mPlayerListIter;
		NSp_InterruptSafeListIterator	*mGroupListIter;
		
		NSpPlayerID						mPlayerID;
		
		OTLIFO							*mEventQ;
		OTLIFO							*mFreeQ;
		OTLIFO							*mCookieQ;
		ERObject						*mPendingMessages;
		NMUInt32						mCookieQLen, 
										mFreeQLen, 
										mMessageQLen;
		NMBoolean						bGameOver;
		NMBoolean						bGamePaused;
		GameState						mGameState;
		NMSInt32						mTimeStampDifferential;
		
		NSpMessageHandlerProcPtr		mAsyncMessageHandler;
		void							*mAsyncMessageContext;

		NSpCallbackProcPtr				mCallbackHandler;
		void							*mCallbackContext;
		NSpGameInfo						mGameInfo;
		
		NSpGamePrivate					*mOwner;

	};

#endif // __NSPGAME__
