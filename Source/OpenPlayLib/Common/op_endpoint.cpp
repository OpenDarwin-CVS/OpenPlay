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
#include "OpenPlay.h"
#endif

#include "OPUtils.h"
#include "portable_files.h"

#include "NetModule.h"
#include "NetModulePrivate.h"

#include "op_definitions.h"
#include "op_globals.h"
#include "module_management.h"
#include "OPDLLUtils.h"

/* ------------ local code */
static OPENPLAY_CALLBACK net_module_callback_function(NMEndpointRef inEndpoint, 
	                   void* inContext, NMCallbackCode inCode, 
                           NMErr inError, void* inCookie);

static PEndpointRef create_endpoint_and_bind_to_protocol_library(NMType type, 
                      FileDesc *library_file, NMErr *err);

static Endpoint *create_endpoint_for_accept(PEndpointRef endpoint, NMErr *err, NMBoolean *from_cache);
static void clean_up_endpoint(PEndpointRef endpoint, NMBoolean return_to_cache);

#if (DEBUG)
	char* GetOPErrorName(NMErr err);
#endif 

#if defined(windows_build)
  #define RETURN_TO_CACHE false
#elif defined(macintosh_build)
  #define RETURN_TO_CACHE true
#endif


/* ---------- opening/closing */


//----------------------------------------------------------------------------------------
// ProtocolOpenEndpoint
//----------------------------------------------------------------------------------------

/*
	Takes the config data created by ProtocolConfig, and builds an endpoint out of it.
	active, passive
	listening & spawn (ie multiple endpoints)
	Callback for multiples.
*/

/*
	Binds to the protocol layer, fills in the function pointers, stays bound until
	CloseEndpoint is called, and calls ProtocolOpenEndpoint
*/
NMErr ProtocolOpenEndpoint(
	PConfigRef inConfig, 
	PEndpointCallbackFunction inCallback,
	void *inContext,
	PEndpointRef *outEndpoint, 
	NMOpenFlags flags)
{
	NMErr err= kNMNoError;
	Endpoint *ep;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	ep= create_endpoint_and_bind_to_protocol_library(inConfig->type, &inConfig->file, &err);
	if(!err)
	{
		op_assert(ep);
		
		ep->openFlags = flags;
		
		/* So we know where we came from.. */
		machine_copy_data(&inConfig->file, &ep->library_file, sizeof(FileDesc));
		/* Now call it's initialization function */
		if(ep->NMOpen)
		{
			/* setup the callback data */
			ep->callback.user_callback= inCallback;
			ep->callback.user_context= inContext;

			/* And open.. */			
			err= ep->NMOpen((NMProtocolConfigPriv*)inConfig->configuration_data, net_module_callback_function, (void *)ep, &ep->module, 
                                        (flags & kOpenActive) ? true : false);
			if(!err)
			{
				op_assert(ep->module);
				*outEndpoint= ep;
				DEBUG_PRINT("New endpoint created (0X%X) of type %c%c%c%c (0X%X to netmodule)",ep,inConfig->type >> 24, (inConfig->type >> 16) & 0xFF,(inConfig->type>>8)&0xFF,inConfig->type&0xFF,ep->module);
			} else {
				DEBUG_PRINT("Open failed creating endpoint #%d of type %c%c%c%c",count_endpoints_of_type(inConfig->type),inConfig->type>>24,(inConfig->type>>16)&0xFF,(inConfig->type>>8)&0xFF,inConfig->type&0xFF); 
				/* Close up everything... */
				clean_up_endpoint(ep, false);
			}
		} else {
			err= errFunctionNotBound;
		}
	}

	#if (DEBUG)
		DEBUG_PRINT("ProtocolOpenEndpoint returning %s (%d)\n",GetOPErrorName(err),err);
	#endif //DEBUG
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolCloseEndpoint
//----------------------------------------------------------------------------------------

/*
	closes the endpoint
	puts NULL in all the function pointers,
	unbinds from the shared library,
*/
NMErr ProtocolCloseEndpoint(
	PEndpointRef endpoint,
	NMBoolean inOrderly)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */

	DEBUG_PRINT("ProtocolCloseEndpoint called for endpoint (0X%X)",endpoint);
	
	if(valid_endpoint(endpoint) && endpoint->state != _state_closing)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMClose)
		{
			/* don't try to close it more than once... */
			if(endpoint->state==_state_unknown)
			{
				endpoint->state= _state_closing;
				
				//make sure we're not working with an incomplete endpoint
				op_assert(endpoint->module != NULL);
				err= endpoint->NMClose(endpoint->module, inOrderly);

			} else {
				op_vwarn(endpoint->state==_state_closing, csprintf(sz_temporary, 
					"endpoint state was %d should have been _state_unknown (%d) or _state_closing (%d) in ProtocolCloseEndpoint", 
					endpoint->state, _state_unknown, _state_closing));
			}
		} else {
			err= errFunctionNotBound;
		}
		/* note we clean up everything on the response from the NMClose... */
	} else {
		if (valid_endpoint(endpoint) == false)
			DEBUG_PRINT("invalid endpoint(0X%X) passed to ProtocolCloseEndpoint()",endpoint);
		else if (endpoint->state == _state_closing)
			DEBUG_PRINT("endpoint (0X%X) passed to ProtocolCloseEndpoint() in state _state_closing",endpoint);
		err= errParamErr;
	}
	
	#if (DEBUG)
		DEBUG_PRINT("ProtocolCloseEndpoint returning %s (%d) for endpoint (0X%X)",GetOPErrorName(err),err,endpoint);	
	#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// clean_up_endpoint
//----------------------------------------------------------------------------------------

static void clean_up_endpoint(
	PEndpointRef endpoint,
	NMBoolean return_to_cache)
{
  UNUSED_PARAMETER(return_to_cache)

  op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "clean up: ep 0x%x is not valid (orderly: %d)", endpoint, return_to_cache));
 
  /* Free up everything.. */
  remove_endpoint_from_loaded_list(endpoint);

  /* done after the above to make sure everything's okay.. */
  endpoint->cookie = PENDPOINT_BAD_COOKIE;
  endpoint->state = _state_closed;

  unbind_from_protocol(endpoint->type, true);
	
#if defined(windows_build)
  dispose_pointer(endpoint);
#elif defined(macintosh_build)
  if(return_to_cache)
    {
      if(gOp_globals.cache_size+1 <= MAXIMUM_CACHED_ENDPOINTS)
	{
	  gOp_globals.endpoint_cache[gOp_globals.cache_size++] = endpoint;
	} else {
	  // we may be at interrupt time here, so we have to add this to a list of things that we free later... 
	  op_assert(gOp_globals.doomed_endpoint_count+1 < MAXIMUM_DOOMED_ENDPOINTS);
	  gOp_globals.doomed_endpoints[gOp_globals.doomed_endpoint_count++] = endpoint;
	}
    } else {
      dispose_pointer(endpoint);
    }
#else
  /* do something for ports here ?? */
#endif
}

//----------------------------------------------------------------------------------------
// ProtocolAcceptConnection
//----------------------------------------------------------------------------------------

/* -------------- connections */
NMErr 
ProtocolAcceptConnection(
	PEndpointRef endpoint,
	void *inCookie,
	PEndpointCallbackFunction inNewCallback,
	void *inNewContext)
{
	NMErr err= kNMNoError;

	op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "ProtocolAcceptConnection: ep 0x%x is not valid", endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMAcceptConnection)
		{
			Endpoint *new_endpoint;
			NMBoolean from_cache;
			
			new_endpoint= create_endpoint_for_accept(endpoint, &err, &from_cache);
			if(!err)
			{
				op_assert(new_endpoint);
				
				new_endpoint->callback.user_callback= inNewCallback;
				new_endpoint->callback.user_context= inNewContext;

/* DEBUG_PRINT("Calling accept connection on endpoint: 0x%x ep->module: 0x%x new_endpoint: 0x%x", endpoint, endpoint->module, new_endpoint); */
				err= endpoint->NMAcceptConnection(endpoint->module, inCookie, net_module_callback_function, new_endpoint);
				if(!err)
				{
				} else {
					/* error occured.  Clean up... */
					DEBUG_PRINT("Open failed creating endpoint #%d of type 0x%x for accept Error: %d", count_endpoints_of_type(endpoint->type), endpoint->type, err);
					clean_up_endpoint(new_endpoint, from_cache); /* don't return to the cache..*/
				}
			}
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
	
	#if (DEBUG)
		DEBUG_PRINT("ProtocolAcceptConnection returning %s (%d)",GetOPErrorName(err),err);	
	#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolRejectConnection
//----------------------------------------------------------------------------------------

NMErr ProtocolRejectConnection(
	PEndpointRef endpoint, 
	void *inCookie)
{
	NMErr err= kNMNoError;
	
	op_vassert(valid_endpoint(endpoint), csprintf(sz_temporary, "ProtocolRejectConnection: ep 0x%x is not valid", endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		if(endpoint->NMRejectConnection)
		{
			op_assert(endpoint->module);
			err= endpoint->NMRejectConnection(endpoint->module, inCookie);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
	
	#if (DEBUG)
		DEBUG_PRINT("ProtocolRejectConnection returning %s (%d)",GetOPErrorName(err),err);		
	#endif //DEBUG
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSetTimeout
//----------------------------------------------------------------------------------------

/* -------------- options */
/*
	set timeouts 
	 (in 60ths of a second)
*/
NMErr ProtocolSetTimeout(
	PEndpointRef endpoint, 
	long timeout)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMSetTimeout)
		{
			op_assert(endpoint->module);
			err= endpoint->NMSetTimeout(endpoint->module, timeout);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolIsAlive
//----------------------------------------------------------------------------------------

NMBoolean ProtocolIsAlive(
	PEndpointRef endpoint)
{
	NMBoolean state = false;
	
	if(valid_endpoint(endpoint) == false)
		return false;
		
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		/* keep a local copy. */
		if(endpoint->NMIsAlive)
		{
			op_assert(endpoint->module);
			state= endpoint->NMIsAlive(endpoint->module);
		}
	}

	return state;
}

//----------------------------------------------------------------------------------------
// ProtocolIdle
//----------------------------------------------------------------------------------------

NMErr ProtocolIdle(
	PEndpointRef endpoint)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 

	//ECF011115 it would seem that a protocol requiring idle time might still want it
	//while closing
	//if(valid_endpoint(endpoint) && endpoint->state != _state_closing)
	if(valid_endpoint(endpoint))
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMIdle)
		{
			op_assert(endpoint->module);
			err= endpoint->NMIdle(endpoint->module);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolFunctionPassThrough
//----------------------------------------------------------------------------------------

NMErr ProtocolFunctionPassThrough(
	PEndpointRef endpoint, 
	NMUInt32 inSelector, 
	void *inParamBlock)
{
	NMErr err= kNMNoError;
	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);

		/* keep a local copy. */
		if(endpoint->NMFunctionPassThrough)
		{
			op_assert(endpoint->module);
			err= endpoint->NMFunctionPassThrough(endpoint->module, inSelector, inParamBlock);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSetEndpointContext
//----------------------------------------------------------------------------------------


/* >>> [Edmark/PBE] 11/10/99 added ProtocolSetEndpointContext() 
  "... allows us to set an endpoint's context after it's created. Adding this 
   function saved us hundreds of lines of messy code elsewhere." */

NMErr ProtocolSetEndpointContext(PEndpointRef endpoint, void* newContext)
{
	NMErr err= kNMNoError;

	op_assert(valid_endpoint(endpoint));
	op_assert(endpoint->cookie == PENDPOINT_COOKIE);

	if (endpoint && (endpoint->cookie == PENDPOINT_COOKIE))
	{
		endpoint->callback.user_context = newContext;
	}
	else
	{
		err = errParamErr;
	}

	return err;
}


//----------------------------------------------------------------------------------------
// ProtocolStartAdvertising
//----------------------------------------------------------------------------------------

/* >>>> Moved ProtocolStart/StopAdvertising() from op_module_mgmt.c */
void ProtocolStartAdvertising(PEndpointRef endpoint)	/* [Edmark/PBE] 11/8/99 changed parameter from PConfigRef to PEndpointRef */
{
	endpoint->NMStartAdvertising(endpoint->module);
}

void ProtocolStopAdvertising(PEndpointRef endpoint)	/* [Edmark/PBE] 11/8/99 changed parameter from PConfigRef to PEndpointRef */
{
	endpoint->NMStopAdvertising(endpoint->module);
}

//----------------------------------------------------------------------------------------
// ProtocolSendPacket
//----------------------------------------------------------------------------------------

/* -------------- sending/receiving */	
/*
	Send the packet to the given endpoint
*/
NMErr ProtocolSendPacket(
	PEndpointRef endpoint, 
	void *inData, 
	NMUInt32 inLength, 
	NMFlags inFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMSendDatagram)
		{
			op_assert(endpoint->module);
			err= endpoint->NMSendDatagram(endpoint->module, (unsigned char*)inData, inLength, inFlags);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolReceivePacket
//----------------------------------------------------------------------------------------

/*
	Tries to receive the endpoint.  Does a copy here.
*/	
NMErr ProtocolReceivePacket(
	PEndpointRef endpoint, 
	void *outData, 
	NMUInt32 *outLength, 
	NMFlags *outFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMReceiveDatagram)
		{
			op_assert(endpoint->module);
			err= endpoint->NMReceiveDatagram(endpoint->module, (unsigned char*)outData, outLength, outFlags);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolSend
//----------------------------------------------------------------------------------------

/* ------------ streaming */
/*
	Sends a stream of data.  Is blocking and synchronous (right?)
	Connects to the other end if necessary...
*/
NMSInt32 ProtocolSend(
	PEndpointRef endpoint, 
	void *inData, 
	NMUInt32 inSize, 
	NMFlags inFlags)
{
NMSInt32 result= 0;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_assert(endpoint->cookie==PENDPOINT_COOKIE);
		if(endpoint->NMSend)
		{
			op_assert(endpoint->module);
			result= endpoint->NMSend(endpoint->module, inData, inSize, inFlags);
		} else {
			result= errFunctionNotBound;
		}
	} else {
		result= errParamErr;
	}
	
	return result; // is < 0 incase of error...
}

//----------------------------------------------------------------------------------------
// ProtocolReceive
//----------------------------------------------------------------------------------------

NMErr ProtocolReceive(
	PEndpointRef endpoint, 
	void *outData, 
	NMUInt32 *ioSize, 
	NMFlags *outFlags)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	if(endpoint)
	{
		op_vassert(endpoint->cookie==PENDPOINT_COOKIE, csprintf(sz_temporary, "Endpoint cookie not right: ep: 0x%x cookie: 0x%x",endpoint, endpoint->cookie));
		if(endpoint->NMReceive)
		{
			op_assert(endpoint->module);
			err= endpoint->NMReceive(endpoint->module, outData, ioSize, outFlags);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// entering/leaving notifiers
//----------------------------------------------------------------------------------------

NMErr ProtocolEnterNotifier(
	PEndpointRef endpoint,
	NMEndpointMode endpointMode)
{
	NMErr err= 0;

	if(endpoint)
	{
		if(endpoint->NMEnterNotifier)
		{
			err= endpoint->NMEnterNotifier(endpoint->module, endpointMode);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------

NMErr ProtocolLeaveNotifier(
	PEndpointRef endpoint,
	NMEndpointMode endpointMode)
{
	NMErr err= 0;

	if(endpoint)
	{
		if(endpoint->NMLeaveNotifier)
		{
			err= endpoint->NMLeaveNotifier(endpoint->module, endpointMode);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}


//----------------------------------------------------------------------------------------
// ProtocolGetEndpointInfo
//----------------------------------------------------------------------------------------

/* ----------- information functions */
NMErr ProtocolGetEndpointInfo(
	PEndpointRef endpoint, 
	NMModuleInfo *info)
{
	NMErr err= kNMNoError;

	/* Some of this error checking could be ignored.. */	
	op_assert(valid_endpoint(endpoint));
	op_assert(endpoint->cookie==PENDPOINT_COOKIE);
	if(endpoint)
	{
		if(endpoint->NMGetModuleInfo)
		{
			err= endpoint->NMGetModuleInfo(info);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
	
	return err;
}

//----------------------------------------------------------------------------------------
// net_module_callback_function
//----------------------------------------------------------------------------------------

/* ------------ static code */
static OPENPLAY_CALLBACK net_module_callback_function(
	NMEndpointRef inEndpoint, 
	void* inContext,
	NMCallbackCode inCode, 
	NMErr inError, 
	void* inCookie)
{
	PEndpointRef ep= (PEndpointRef) inContext;
	NMBoolean callback= true;
	
	op_assert(ep);
	op_assert(ep->callback.user_callback);
	op_vassert(ep->cookie==PENDPOINT_COOKIE, csprintf(sz_temporary, "inCookie wasn't our PEndpointRef? 0x%x Cookie: 0x%x", ep, ep->cookie));

	/* must always reset it, because it may not be set yet. (trust me) */
	op_assert(valid_endpoint(ep));
	ep->module= inEndpoint;
	switch(inCode)
	{
		case kNMAcceptComplete: /* new endpoint, cookie is the parent endpoint. (easy) */
			/* generate the kNMHandoffComplete code.. */
			op_assert(ep->parent);
			if(ep->parent->callback.user_callback)
			{
				ep->parent->callback.user_callback(ep->parent, ep->parent->callback.user_context, kNMHandoffComplete, 
					inError, ep);
			}
			
			/* now setup for the kNMAcceptComplete */
			inCookie= ep->parent;
			op_assert(inCookie);
			break;
			
		case kNMHandoffComplete:
			/* eat it. */
			callback= false;
			break;
		default:
		break;
	}

	if(callback && ep->callback.user_callback)
		ep->callback.user_callback(ep, ep->callback.user_context, inCode, inError, inCookie);
	if(inCode==kNMCloseComplete)
	{
		op_vwarn(ep->state==_state_closing, csprintf(sz_temporary, 
			"endpoint state was %d should have been _state_closing (%d) for kNMCloseComplete", 
			ep->state, _state_closing));
		clean_up_endpoint(ep, true); /* Must try to return to the cache, because we can't deallocate memory at interrupt time. */
	}
}

//----------------------------------------------------------------------------------------
// create_endpoint_and_bind_to_protocol_library
//----------------------------------------------------------------------------------------

static PEndpointRef create_endpoint_and_bind_to_protocol_library(
	NMType type,
	FileDesc *library_file,
	NMErr *err)
{
	Endpoint *ep = NULL;
	
	ep= (Endpoint *) new_pointer(sizeof(Endpoint));
	if(ep)
	{
		machine_mem_zero(ep, sizeof(Endpoint));
		*err= bind_to_protocol(type, library_file, &ep->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			ep->cookie= PENDPOINT_COOKIE;
			ep->type= type;
			ep->state= _state_unknown;
			ep->library_file= *library_file;
			ep->parent= NULL;

			/* Lock and load the pointers.. */
			ep->NMGetModuleInfo= (NMGetModuleInfoPtr) load_proc(ep->connection, kNMGetModuleInfo);

			ep->NMGetConfig= (NMGetConfigPtr) load_proc(ep->connection, kNMGetConfig);
			ep->NMGetConfigLen= (NMGetConfigLenPtr) load_proc(ep->connection, kNMGetConfigLen);

			ep->NMOpen= (NMOpenPtr) load_proc(ep->connection, kNMOpen);
			ep->NMClose= (NMClosePtr) load_proc(ep->connection, kNMClose);

			ep->NMAcceptConnection= (NMAcceptConnectionPtr) load_proc(ep->connection, kNMAcceptConnection);
			ep->NMRejectConnection= (NMRejectConnectionPtr) load_proc(ep->connection, kNMRejectConnection);

			ep->NMSendDatagram= (NMSendDatagramPtr) load_proc(ep->connection, kNMSendDatagram);
			ep->NMReceiveDatagram= (NMReceiveDatagramPtr) load_proc(ep->connection, kNMReceiveDatagram);

			ep->NMSend= (NMSendPtr) load_proc(ep->connection, kNMSend);
			ep->NMReceive= (NMReceivePtr) load_proc(ep->connection, kNMReceive);

			ep->NMSetTimeout= (NMSetTimeoutPtr) load_proc(ep->connection, kNMSetTimeout);
			ep->NMIsAlive= (NMIsAlivePtr) load_proc(ep->connection, kNMIsAlive);
			ep->NMFunctionPassThrough= (NMFunctionPassThroughPtr) load_proc(ep->connection, kNMFunctionPassThrough);

			ep->NMIdle= (NMIdlePtr) load_proc(ep->connection, kNMIdle);

			ep->NMStopAdvertising  = (NMStopAdvertisingPtr)  load_proc(ep->connection, kNMStopAdvertising);
			ep->NMStartAdvertising = (NMStartAdvertisingPtr) load_proc(ep->connection, kNMStartAdvertising);

			ep->NMEnterNotifier  = (NMEnterNotifierPtr)  load_proc(ep->connection, kNMEnterNotifier);
			ep->NMLeaveNotifier  = (NMLeaveNotifierPtr)  load_proc(ep->connection, kNMLeaveNotifier);

			/* [Edmark/PBE] 11/8/99 moved NMStart/StopAdvertising from ProtocolConfig to Endpoint */

			ep->next= NULL;

			/* remember this... */
			add_endpoint_to_loaded_list(ep);
		}
		
		if(*err)
		{
			/* clear! */
			dispose_pointer(ep);
			ep= NULL;
		}
	} else {
		*err= errNoMemory;
	}

	return ep;
}

//----------------------------------------------------------------------------------------
// create_endpoint_for_accept
//----------------------------------------------------------------------------------------

static Endpoint *create_endpoint_for_accept(
	PEndpointRef endpoint,
	NMErr *err,
	NMBoolean *from_cache)
{
	Endpoint *new_endpoint= NULL;

#if defined(windows_build)
	/* We must create the wrapper for it.. */
	new_endpoint= create_endpoint_and_bind_to_protocol_library(endpoint->type, &endpoint->library_file, err);
	*from_cache= false;
#elif defined(macintosh_build)
	if(gOp_globals.cache_size>0)
	{
		new_endpoint= gOp_globals.endpoint_cache[--gOp_globals.cache_size];
	}
	*from_cache= true;

	if(new_endpoint) 
	{
		*err= bind_to_protocol(endpoint->type, &endpoint->library_file, &new_endpoint->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			new_endpoint->cookie= PENDPOINT_COOKIE;
			new_endpoint->type= endpoint->type;
			new_endpoint->state= _state_unknown;
			new_endpoint->parent= NULL;

			new_endpoint->library_file= endpoint->library_file;
			new_endpoint->NMGetModuleInfo= endpoint->NMGetModuleInfo;
			new_endpoint->NMGetConfig= endpoint->NMGetConfig;
			new_endpoint->NMGetConfigLen= endpoint->NMGetConfigLen;
			new_endpoint->NMOpen= endpoint->NMOpen;
			new_endpoint->NMClose= endpoint->NMClose;
			new_endpoint->NMAcceptConnection= endpoint->NMAcceptConnection;
			new_endpoint->NMRejectConnection= endpoint->NMRejectConnection;
			new_endpoint->NMSendDatagram= endpoint->NMSendDatagram;
			new_endpoint->NMReceiveDatagram= endpoint->NMReceiveDatagram;
			new_endpoint->NMSend= endpoint->NMSend;
			new_endpoint->NMReceive= endpoint->NMReceive;
			new_endpoint->NMSetTimeout= endpoint->NMSetTimeout;
			new_endpoint->NMIsAlive= endpoint->NMIsAlive;
			new_endpoint->NMFunctionPassThrough= endpoint->NMFunctionPassThrough;
			new_endpoint->NMIdle= endpoint->NMIdle;

			new_endpoint->NMStopAdvertising  = endpoint->NMStopAdvertising;
			new_endpoint->NMStartAdvertising = endpoint->NMStartAdvertising;

			new_endpoint->NMEnterNotifier  = endpoint->NMEnterNotifier;
			new_endpoint->NMLeaveNotifier  = endpoint->NMLeaveNotifier;

			new_endpoint->next= NULL;

			add_endpoint_to_loaded_list(new_endpoint);
		} else {
			DEBUG_PRINT("Err %d binding to protocol", *err);
		}
		
		if(*err)
		{
			/* DEBUG_PRINT("returning to cache due to error.. %d", *err); */
			op_assert(gOp_globals.cache_size+1<=MAXIMUM_CACHED_ENDPOINTS);
			gOp_globals.endpoint_cache[gOp_globals.cache_size++]= new_endpoint;
			new_endpoint= NULL;
		}
	} else {
		*err= errNoMemory;
	}
#elif posix_build
	new_endpoint = (Endpoint*)new_pointer(sizeof(Endpoint));

	if(new_endpoint) 
	{
		*err= bind_to_protocol(endpoint->type, &endpoint->library_file, &new_endpoint->connection);
		if(!(*err))
		{
			/* Load in the endpoint information.. */
			new_endpoint->cookie= PENDPOINT_COOKIE;
			new_endpoint->type= endpoint->type;
			new_endpoint->state= _state_unknown;
			new_endpoint->parent= NULL;

			new_endpoint->library_file= endpoint->library_file;
			new_endpoint->NMGetModuleInfo= endpoint->NMGetModuleInfo;
			new_endpoint->NMGetConfig= endpoint->NMGetConfig;
			new_endpoint->NMGetConfigLen= endpoint->NMGetConfigLen;
			new_endpoint->NMOpen= endpoint->NMOpen;
			new_endpoint->NMClose= endpoint->NMClose;
			new_endpoint->NMAcceptConnection= endpoint->NMAcceptConnection;
			new_endpoint->NMRejectConnection= endpoint->NMRejectConnection;
			new_endpoint->NMSendDatagram= endpoint->NMSendDatagram;
			new_endpoint->NMReceiveDatagram= endpoint->NMReceiveDatagram;
			new_endpoint->NMSend= endpoint->NMSend;
			new_endpoint->NMReceive= endpoint->NMReceive;
			new_endpoint->NMSetTimeout= endpoint->NMSetTimeout;
			new_endpoint->NMIsAlive= endpoint->NMIsAlive;
			new_endpoint->NMFunctionPassThrough= endpoint->NMFunctionPassThrough;
			new_endpoint->NMIdle= endpoint->NMIdle;

			new_endpoint->NMStopAdvertising  = endpoint->NMStopAdvertising;
			new_endpoint->NMStartAdvertising = endpoint->NMStartAdvertising;

			new_endpoint->NMEnterNotifier  = endpoint->NMEnterNotifier;
			new_endpoint->NMLeaveNotifier  = endpoint->NMLeaveNotifier;

			new_endpoint->next= NULL;

			add_endpoint_to_loaded_list(new_endpoint);
		} else {
			DEBUG_PRINT("Err %d binding to protocol", *err);
		}
		
		if(*err)
		{
			new_endpoint= NULL;
		}
	} else {
		*err= errNoMemory;
	}
#else
	#error create_endpoint_for_accept not defined for this platform
#endif

	if(new_endpoint)
	{
		new_endpoint->parent= endpoint;
	}

	return new_endpoint;
}

#if (DEBUG)
#define DO_CASE(a) case a: return #a; break

char* GetOPErrorName(NMErr err)
{
	switch (err){
		DO_CASE(kNMOpenEndpointFailedErr);
		DO_CASE(kNMOutOfMemoryErr);
		DO_CASE(kNMParameterErr);
		DO_CASE(kNMTimeoutErr);
		DO_CASE(kNMInvalidConfigErr);
		DO_CASE(kNMNewerVersionErr);
		DO_CASE(kNMOpenFailedErr);
		DO_CASE(kNMAcceptFailedErr);
		DO_CASE(kNMConfigStringTooSmallErr);
		DO_CASE(kNMResourceErr);
		DO_CASE(kNMAlreadyEnumeratingErr);
		DO_CASE(kNMNotEnumeratingErr);
		DO_CASE(kNMEnumerationFailedErr);
		DO_CASE(kNMNoDataErr);
		DO_CASE(kNMProtocolInitFailedErr);
		DO_CASE(kNMInternalErr);
		DO_CASE(kNMMoreDataErr);
		DO_CASE(kNMUnknownPassThrough);
		DO_CASE(kNMAddressNotFound);
		DO_CASE(kNMAddressNotValidYet);
		DO_CASE(kNMWrongModeErr);
		DO_CASE(kNMBadStateErr);
		DO_CASE(kNMTooMuchDataErr);
		DO_CASE(kNMCantBlockErr);
		DO_CASE(kNMInitFailedErr);
		DO_CASE(kNMFlowErr);
		DO_CASE(kNMProtocolErr);
		DO_CASE(kNMModuleInfoTooSmall);
		DO_CASE(kNMClassicOnlyModuleErr);
		DO_CASE(kNMNoError);
		DO_CASE(errBadPacketDefinition);
		DO_CASE(errBadShortAlignment);
		DO_CASE(errBadLongAlignment);
		DO_CASE(errBadPacketDefinitionSize);
		DO_CASE(errBadVersion);
		DO_CASE(errParamErr);
		DO_CASE(errNoMoreNetModules);
		DO_CASE(errModuleNotFound);
		DO_CASE(errBadConfig);
		DO_CASE(errFunctionNotBound);
		DO_CASE(errNoMemory);
		default: return "unknown OP error"; break;
	}
}
#endif //DEBUG