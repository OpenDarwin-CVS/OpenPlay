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

#include "NSpPrefix.h"
#include "NSpGameSlave.h"
#include "NetSprocketLib.h"
#include "ByteSwapping.h"

#include "String_Utils.h"

#if macintosh_build
	#include <OpenTptAppleTalk.h>
	#include <OpenTptInternet.h>
	#include <OpenTptSerial.h>
#endif	/*	macintosh_build	*/



//----------------------------------------------------------------------------------------
// NSpGameSlave::NSpGameSlave
//----------------------------------------------------------------------------------------

NSpGameSlave::NSpGameSlave(NSpFlags inFlags) : NSpGame(0, NULL, NULL, kNSpClientServer, inFlags)
{
	mEndpoint = NULL;
	mPlayerID = -1;
	mObjectID = NSpGameSlave::class_ID;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::~NSpGameSlave
//----------------------------------------------------------------------------------------

NSpGameSlave::~NSpGameSlave()
{
	//Ä	delete mEndpoint;
	if (mEndpoint)
	{
		mEndpoint->Close();
	}
}


//----------------------------------------------------------------------------------------
// NSpGameSlave::Join
//----------------------------------------------------------------------------------------
/*
	Join takes an address of a server, sets up a new endpoint, if needed, and
	sends a join request.  If the request is granted, we use this endpoint
	for all our communications with the server
*/
OSStatus
NSpGameSlave::Join(
		ConstStr31Param		inPlayerName,
		ConstStr31Param		inPassword,
		NSpPlayerType		inType,
		void				*inCustomData,
		NMUInt32			inCustomDataLen,
		NSpAddressReference	inAddress)
{
	NMErr				status = kNMNoError;
	OSStatus			err = kNMNoError;
	NMType				netModuleType;
	/*
	char				netModuleTypeString[32];
	char				configString[1024];
	char				*typeStartPtr;
	char				*tokenPtr;
	NMSInt16			configStrLen;
	*/
	//Ä	Require a name
	if (inPlayerName == NULL)
		return (kNSpNameRequiredErr);

	//Try_
	{
		// Need to determine which type of endpoint to create...

/* LR -- added function to return type w/o having to do all the string manipulation

		err = ProtocolGetConfigStringLen((PConfigRef) inAddress, &configStrLen);
		ThrowIfOSErr_(err);

		op_assert(configStrLen < sizeof(configString));
		
		err = ProtocolGetConfigString((PConfigRef) inAddress, configString, configStrLen);
		ThrowIfOSErr_(err);

		typeStartPtr = strstr(configString,"type=") + 5;
			
		tokenPtr = strtok(typeStartPtr, "\t");
			
		strcpy(netModuleTypeString, tokenPtr);
			
		netModuleType = (NMType) atol(netModuleTypeString);
*/
		netModuleType = ProtocolGetConfigType( (PConfigRef)inAddress );
		switch (netModuleType)
		{
			case kATModuleType:
			{
#if macintosh_build
				mEndpoint = (CEndpoint *) new COTATEndpoint(this);			
#else
				status = kNSpFeatureNotImplementedErr;
				goto error;
				//Throw_(kNSpFeatureNotImplementedErr);

#endif		/*	macintosh_build	*/
			}
			break;
			 
			case kIPModuleType:
			{
				mEndpoint = (CEndpoint *) new COTIPEndpoint(this);			
			}
			break;
			
			default:
				//Throw_(kNSpInvalidProtocolRefErr);
				status = kNSpInvalidProtocolRefErr;
				goto error;
			break;

		}
	
		if (kNMNoError == err)
		{
			//Ä	Initialize our endpoint for send/receive only
			status = mEndpoint->InitNonAdvertiser( (NSpProtocolPriv *) inAddress);
			//ThrowIfOSErr_(err);
			if (status)
				goto error;

			
#if macintosh_build		//?? still needed??
				unsigned long dummyClock;	// crt 7/14/2001
				Delay(5, &dummyClock);		// crt 7/14/2001
#endif
			
			//Ä	Send our join request
			status = SendJoinRequest(inPlayerName, inPassword, inType, inCustomData, inCustomDataLen);
			//ThrowIfOSErr_(err);
			if (status)
				goto error;

		}

	}
	//Catch_(code)
	error:
	if (status)
	{
		NMErr code = status;
		return (code);
	}

	return (err);
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::SendJoinRequest
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::SendJoinRequest(
		ConstStr31Param	inPlayerName,
		ConstStr31Param	inPassword,
		NSpPlayerType	inType,
		void			*inCustomData,
		NMUInt32		inCustomDataLen)
{
OSStatus				status;
NSpJoinRequestMessage	*theMessage;
NMUInt32				messageSize;


	messageSize = sizeof (NSpJoinRequestMessage) + inCustomDataLen - 1;
	theMessage = (NSpJoinRequestMessage *) InterruptSafe_alloc(messageSize);

	if (NULL == theMessage)
		return (kNSpMemAllocationErr);

	NSpClearMessageHeader((NSpMessageHeader *)theMessage);

	theMessage->header.what = kNSpJoinRequest;
	theMessage->header.messageLen = messageSize;
	theMessage->header.version = kVersion10Message;
	theMessage->header.id = mNextMessageID++;
	theMessage->theType = inType;
	theMessage->customDataLen = inCustomDataLen;

	if (inCustomDataLen > 0)
		machine_move_data(inCustomData, theMessage->customData, inCustomDataLen);

	if (inPassword)
		doCopyPStrMax(inPassword, theMessage->password, 31);
	else
		theMessage->password[0] = 0;

	doCopyPStrMax(inPlayerName, theMessage->name, 31);

	status = mEndpoint->SendMessage((NSpMessageHeader *) theMessage, (NMUInt8 *) theMessage + sizeof (NSpMessageHeader), kNSpSendFlag_Registered);
	
	InterruptSafe_free((char *) theMessage);

	return (status);
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::SendUserMessage
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::SendUserMessage(NSpMessageHeader *inMessage, NSpFlags inFlags)
{
OSStatus status;
NSp_InterruptSafeListIterator 	groupIter(*mGroupList);
NSp_InterruptSafeListIterator 	*playerIter;
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
GroupListItem				*theGroup;
NMBoolean					bNoCopy = false;
NMBoolean					bSelfSent = false;
NMBoolean					swapBack = false;	//?? why here and not below?

	if (mGameState == kStopped)
		return kNSpGameTerminatedErr;

	inMessage->from = mPlayerID;
	inMessage->version = kVersion10Message;
	inMessage->id = mNextMessageID++;

	if (inFlags & kNSpSendFlag_SelfSend)
	{
		bNoCopy = true;
		status = DoSelfSend(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), inFlags);
		bSelfSent = true;
	}

	//Ä	If it's just to ourselves, handle that special case
	if (inMessage->to == mPlayerID && !bSelfSent)
	{
		status = DoSelfSend(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), inFlags);
		bSelfSent = true;
	}
	else if (inMessage->to < kNSpHostOnly)	//Ä	We need to handle if its a group. In case we're a member
	{
		if (!bSelfSent)
		{
			while (groupIter.Next(&theItem))
			{
				theGroup = (GroupListItem *) theItem;
				if (theGroup->id == inMessage->to)
				{
					playerIter = new NSp_InterruptSafeListIterator(*theGroup->players);
					//ThrowIfNULL_(playerIter);
					op_assert(playerIter);

					while (playerIter->Next(&theItem))
					{
						thePlayer = (PlayerListItem *)(((UInt32ListMember *)theItem)->GetValue());
						if (thePlayer->id == mPlayerID)
						{
							status = DoSelfSend(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), inFlags);
							bSelfSent = true;
							break;
						}
					}

					delete playerIter;
					break;
				}
			}
		}

		//Ä	Now send it to the host
		status = mEndpoint->SendMessage(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), inFlags);
		swapBack = true;
	}
	else
	{
		status = mEndpoint->SendMessage(inMessage, (NMUInt8 *)inMessage + sizeof (NSpMessageHeader), inFlags);
		swapBack = true;
	}

	// The user may end up resending their message or (unwisely) using the fields in the header
	// for some purpose, but they will be shocked to find that the header has been swapped on
	// them!  So if it has been swapped, then swap it back before returning...
	
#if !macintosh_build
	
	if (swapBack)
		SwapHeaderByteOrder(inMessage);	

#endif

	return status;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::IdleEndpoints
//----------------------------------------------------------------------------------------
void
NSpGameSlave::IdleEndpoints(void)
{
	//fixme - theoretically, we should be idling all our endpoints?
	if (mEndpoint)
		mEndpoint->Idle();
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::SendTo
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::SendTo(NSpPlayerID inTo, NMSInt32 inWhat, void *inData, NMUInt32 inLen, NSpFlags inFlags)
{
OSStatus status;
//NSpMessageHeader header;
NSpMessageHeader				*headerPtr = NULL;
NMUInt8							*dataPtr;
NSp_InterruptSafeListIterator 	groupIter(*mGroupList);
NSp_InterruptSafeListIterator 	*playerIter;
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem					*thePlayer;
GroupListItem					*theGroup;
NMBoolean						bNoCopy = false;
NMBoolean						bSelfSent = false;

	if (mGameState == kStopped)
		return kNSpGameTerminatedErr;

	dataPtr = new NMUInt8[inLen + sizeof(NSpMessageHeader)];
	
	if (dataPtr == NULL)
		return (kNSpSendFailedErr);

	headerPtr = (NSpMessageHeader *) dataPtr;
			
	NSpClearMessageHeader(headerPtr);
			
	headerPtr->from = mPlayerID;
	headerPtr->to = inTo;
	headerPtr->version = kVersion10Message;
	headerPtr->id = mNextMessageID++;
	headerPtr->what = inWhat;
	headerPtr->messageLen = inLen + sizeof(NSpMessageHeader);
	
	machine_move_data(inData, dataPtr + sizeof(NSpMessageHeader), inLen);


	if (inFlags & kNSpSendFlag_SelfSend)
	{
		bNoCopy = true;
		status = DoSelfSend(headerPtr, inData, inFlags);
		bSelfSent = true;
	}

	//Ä	If it's just to ourselves, handle that special case
	if (inTo == mPlayerID && !bSelfSent)
	{
		status = DoSelfSend(headerPtr, inData, inFlags);
		bSelfSent = true;
	}
	else if (inTo < kNSpHostOnly)	// We need to handle if its a group. In case we're a member
	{
		if (!bSelfSent)
		{
			while (groupIter.Next(&theItem))
			{
				theGroup = (GroupListItem *) theItem;
				if (theGroup->id == inTo)
				{
					playerIter = new NSp_InterruptSafeListIterator(*theGroup->players);
					//ThrowIfNULL_(playerIter);
					op_assert(playerIter);

					while (playerIter->Next(&theItem))
					{
						thePlayer = (PlayerListItem *)(((UInt32ListMember *)theItem)->GetValue());
						if (thePlayer->id == mPlayerID)
						{
							status = DoSelfSend(headerPtr, inData, inFlags);
							bSelfSent = true;
							break;
						}
					}

					delete playerIter;
					break;
				}
			}
		}

		//Ä	Now send it to the host
		status = mEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}
	else
	{
		status = mEndpoint->SendMessage(headerPtr, (NMUInt8 *) inData, inFlags);
	}

	if (dataPtr != NULL)
		delete dataPtr;

	return status;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::HandleJoinApproved
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::HandleJoinApproved(TJoinApprovedMessagePrivate *inMessage, NMUInt32 inTimeReceived)
{
OSStatus		status;
NMBoolean		handled = true;
NSpPlayerInfo	playerInfo;
NSpGroupInfoPtr	groupInfoPtr;
NMUInt32		i;
NMUInt32		rtt;
NMUInt32		hostProcessingTime;

	if (NULL == inMessage)
		return (false);
		
	mGameInfo.currentPlayers = 0;		// This will be incremented by AddPlayer
	mNextAvailableGroupID = inMessage->groupIDStartRange;

	//Ä	Get the info for the current players
	if (inMessage->playerCount > 0)
	{
	NSpPlayerInfoPtr	p = (NSpPlayerInfoPtr) inMessage->data;
	NMUInt8				*q = (NMUInt8 *) p;

		playerInfo.groupCount = 0;
		
		for (i = 0; i < inMessage->playerCount; i++)
		{
			machine_move_data(p, &playerInfo, kJoinApprovedPlayerInfoSize);

			q += kJoinApprovedPlayerInfoSize;
			p = (NSpPlayerInfoPtr) q;

			//Ä	We always get to other players via the server
			//Ä	Ä This should change if we do client-client topology or fault-tolerance
			handled = AddPlayer(&playerInfo, mEndpoint);

			if (false == handled)
			{
#if INTERNALDEBUG
				debug_message("AddPlayer not handled in HandleJoinApproved.");
#endif
				break;
			}
		}
	}

	if (false == handled)
		return (handled);

	//Ä	Record the groups state
	if (inMessage->groupCount > 0)
	{
		groupInfoPtr = (NSpGroupInfoPtr) (inMessage->data + (inMessage->playerCount * kJoinApprovedPlayerInfoSize));

		status = MakeGroupListFromJoinApprovedMessage(groupInfoPtr, inMessage->groupCount);
	}

	if (handled)
	{
		//Ä	The to field contains our player id
		mPlayerID = inMessage->header.to;
	}

	//Ä	Set the timestamp differential
	hostProcessingTime = inMessage->header.when - inMessage->receivedTimeStamp;
	rtt = inTimeReceived - mEndpoint->GetLastMessageSentTimeStamp() - hostProcessingTime;
	mTimeStampDifferential = (inMessage->receivedTimeStamp - mEndpoint->GetLastMessageSentTimeStamp()) - rtt/2;

	return handled;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::MakeGroupListFromJoinApprovedMessage
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::MakeGroupListFromJoinApprovedMessage(NSpGroupInfoPtr inGroups, NMUInt32 inCount)
{
	NMErr 						status = kNMNoError;
	NMUInt32					i, j;
	TCreateGroupMessage			message;
	TAddPlayerToGroupMessage	playerAddMessage;
	NMBoolean					handled;

	NSpClearMessageHeader(&message.header);

	//Try_
	{
		for (i = 0; i < inCount; i++)
		{
			message.id = inGroups->id;
			handled = HandleCreateGroupMessage(&message);
#if INTERNALDEBUG
			if (!handled)
			{
				DEBUG_PRINT("HandleCreateGroupMessage failed in MakeGroupListFromJoinApprovedMessage for group #%d", message.id);
			}
#endif
			for (j = 0; j < inGroups->playerCount; j++)
			{
				playerAddMessage.player = inGroups->players[j];
				playerAddMessage.group = inGroups->id;
				handled = HandleAddPlayerToGroupMessage(&playerAddMessage);
			}

			inGroups = (NSpGroupInfoPtr)((NMUInt8 *)inGroups + sizeof (NSpGroupInfo) + (((inGroups->playerCount == 0) ? 0 : inGroups->playerCount - 1)
							* sizeof (NSpPlayerID)));
		}
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
// NSpGameSlave::HandleJoinDenied
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::HandleJoinDenied(NSpJoinDeniedMessage *inMessage)
{
UNUSED_PARAMETER(inMessage);

	NMBoolean handled = true;

	return handled;
}


//----------------------------------------------------------------------------------------
// NSpGameSlave::HandlePlayerJoined
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::HandlePlayerJoined(NSpPlayerJoinedMessage *inMessage)
{
NMBoolean 			handled = true;
NSpPlayerID		newPlayerID = inMessage->playerInfo.id;	
	
	//Ä	If it wasn't us that joined, add them
	if (newPlayerID != mPlayerID)
		handled = AddPlayer(&(inMessage->playerInfo), mEndpoint);

	return handled;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::HandleGameTerminated
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::HandleGameTerminated(NSpMessageHeader *inMessage)
{
UNUSED_PARAMETER(inMessage);

	mGameState = kStopped;

	return true;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::HandleNewEvent
//----------------------------------------------------------------------------------------

void
NSpGameSlave::HandleNewEvent(ERObject *inERObject, CEndpoint *inEndpoint, void *inCookie)
{
UNUSED_PARAMETER(inCookie);

	NSpMessageHeader 	*theMessage = inERObject->PeekNetMessage();
	NMBoolean			handled;
	
	if (theMessage == NULL)
		return;

	//Ä	Any system event should be handled immediately
	if (theMessage->what & kNSpSystemMessagePrefix)
	{
		NMBoolean passToUser;

		if (theMessage->what == kNSpJoinApproved)
		{
			SwapJoinApproved(theMessage);			
			handled = HandleJoinApproved((TJoinApprovedMessagePrivate *) theMessage, inERObject->GetTimeReceived());
			handled = true;
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
		}
 		else
 		{
	 		passToUser = ProcessSystemMessage(theMessage);
		}

		if (!passToUser)
		{
			ReleaseERObject(inERObject);
			return;
		}
	}

	//Ä	Allow the user to have a crack at it, if he installed a message handler
	//Ä	User function returns a boolean telling whether or not to enqueue the message
	if (mAsyncMessageHandler && !((mAsyncMessageHandler)((NSpGameReference) this->GetGameOwner(), theMessage, mAsyncMessageContext)))
	{
		ReleaseERObject(inERObject);
		return;
	}

	//Ä	Set the endpoint and address
	if (inEndpoint)
		inERObject->SetEndpoint(inEndpoint);

	mEventQ->Enqueue(inERObject);
	mMessageQLen++;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::PrepareForDeletion
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::PrepareForDeletion(NSpFlags inFlags)
{
UNUSED_PARAMETER(inFlags);

OSStatus	status = kNMNoError;

	if (mGameState == kStopped)
	{
		//Ä	Remove all the players, in case we never did get the disconnect
		RemovePlayer(kNSpAllPlayers, false);

		//Ä	Wait for the server to disconnect us
		status = mEndpoint->WaitForDisconnect(5);
		
		if (status == kNSpTimeoutErr)
		{
			status = mEndpoint->Disconnect(false);
		}
	}
	else
	{
		//Ä	Do an orderly disconnect
		status = mEndpoint->Disconnect(true);

		//Ä	Wait for the server to ack it
//		status = mEndpoint->WaitForDisconnect(5);
		
		if (status != kNMNoError)
		{
			status = mEndpoint->Disconnect(false);
		}
		
		mGameState = kStopped;
	}

	return status;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::ProcessSystemMessage
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::ProcessSystemMessage(NSpMessageHeader *inMessage)
{
	NMBoolean	passToUser = false;
	NMBoolean	handled;

	
	switch (inMessage->what)
	{
		case kNSpJoinDenied:
			// Join denied message requires no byte-swapping.
			handled = HandleJoinDenied((NSpJoinDeniedMessage *) inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
			break;

		case kNSpPlayerJoined:
			SwapPlayerJoined(inMessage);
			handled = HandlePlayerJoined((NSpPlayerJoinedMessage *) inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
			break;
			
		case kNSpPlayerLeft:
			SwapPlayerLeft(inMessage);
			handled = HandlePlayerLeft((NSpPlayerLeftMessage *) inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
			break;

		case kNSCreateGroup:
			SwapCreateGroup(inMessage);
			handled = HandleCreateGroupMessage((TCreateGroupMessage *)inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			break;

		case kNSDeleteGroup:			// Delete group and create group messages
			SwapCreateGroup(inMessage);	// have identical structures.
			handled = HandleDeleteGroupMessage((TDeleteGroupMessage *)inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			break;

		case kNSpGameTerminated:
			// Game terminated message requires no byte-swapping.
			handled = HandleGameTerminated(inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
			break;

		case kNSAddPlayerToGroup:
			SwapAddPlayerToGroup(inMessage);
			handled = HandleAddPlayerToGroupMessage((TAddPlayerToGroupMessage *)inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			break;

		case kNSRemovePlayerFromGroup:			// Add and remove player to/from group
			SwapAddPlayerToGroup(inMessage);	// have identical structures.
			handled = HandleRemovePlayerFromGroupMessage((TRemovePlayerFromGroupMessage *)inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			break;

		case kNSPauseGame:
			mGameState = kPaused;
			break;

		case kNSResumeGame:
			mGameState = kRunning;
			break;

		//Ä	Message type handling of kNSpPlayerTypeChanged added here
		//	by Randy Thompson on July, 7, 2000.
		case kNSpPlayerTypeChanged:
			SwapPlayerTypeChanged(inMessage);
			handled = HandlePlayerTypeChangedMessage((NSpPlayerTypeChangedMessage *) inMessage);
			//ThrowIfNot_(handled);
			op_assert(handled);
			passToUser = true;
			break;
	}

	return passToUser;
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::RemovePlayer
//----------------------------------------------------------------------------------------

NMBoolean
NSpGameSlave::RemovePlayer(NSpPlayerID inPlayer, NMBoolean inDisconnect)
{
UNUSED_PARAMETER(inDisconnect);

NSp_InterruptSafeListIterator	iter(*mPlayerList);
NSp_InterruptSafeListIterator	groupIter(*mGroupList);
NSp_InterruptSafeListMember 	*theItem;
PlayerListItem				*thePlayer;
NMBoolean					removeAll = false;
NMBoolean					found = false;

	if (inPlayer == kNSpAllPlayers)
		removeAll = true;

	//Ä	Remove the player from any groups that they are in

	//Ä	In theory we should look at the player's group list
	//Ä	and just remove them from those.  However, we don't have
	//Ä	any clean method for that except for sending ourselves a
	//Ä	RemovePlayerFromGroup message and that has a lot of overhead.
	while (groupIter.Next(&theItem))
	{
	GroupListItem	*theGroup;
	
		theGroup = (GroupListItem *) theItem;

		(void) theGroup->RemovePlayer(inPlayer);
	}

	//Ä	Then remove them from the list of players
	while (!found && iter.Next(&theItem))
	{
		thePlayer = (PlayerListItem *)theItem;
		if (removeAll || thePlayer->id == inPlayer)
		{
			if (mPlayerList->Remove(theItem))
			{
				delete thePlayer;
				--mGameInfo.currentPlayers;
			}

			if (! removeAll)
				found = true;
		}
	}

	return (true);
}

//----------------------------------------------------------------------------------------
// NSpGameSlave::HandleEndpointDisconnected
//----------------------------------------------------------------------------------------

OSStatus
NSpGameSlave::HandleEndpointDisconnected(CEndpoint *inEndpoint)
{
UNUSED_PARAMETER(inEndpoint);

OSStatus					status = kNMNoError;
NSpGameTerminatedMessage	message;

	if (mGameState != kStopped)		// Only tell the user if we didn't already
	{
		//Ä	Send a local message that the game has been terminated
		NSpClearMessageHeader(&message.header);

		message.header.what = kNSpGameTerminated;
		message.header.to = kNSpAllPlayers;
		message.header.version = kVersion10Message;
		message.header.id = mNextMessageID++;
		message.header.messageLen = sizeof (NSpGameTerminatedMessage);

		//Ä	Tell the user that the game is over
		status = DoSelfSend(&message.header, NULL, kNSpSendFlag_Registered);
	}

	//Ä	Remove all the players
	RemovePlayer(kNSpAllPlayers, false);

	//Ä	Receiving this message means we've been disconnected from the server
	mGameState = kStopped;

	return (status);
}
