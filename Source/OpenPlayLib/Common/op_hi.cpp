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
#include "op_definitions.h"

/* ------------ dialog functions */

//----------------------------------------------------------------------------------------
// ProtocolSetupDialog
//----------------------------------------------------------------------------------------

/* HWND under windows, DialogPtr on Macs
 * id of ditl item for frame.  Should this be a rect instead?
 * This is NOT a ProtocolConfig-> it's the data hung off the data element
 */
NMErr ProtocolSetupDialog(
	DIALOGPTR dialog, 
	short frame, 
	short base,
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMSetupDialog)
		{
			err= config->NMSetupDialog(dialog, frame, base, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolHandleEvent
//----------------------------------------------------------------------------------------

/* item_hit is in the DITL's reference space. */
NMBoolean ProtocolHandleEvent(
	DIALOGPTR dialog, 
	EVENT *event, 
	PConfigRef config)
{
/* sjb 19990317 remove all vestige of returning errors from this function. */
/* This code doesn't return an error, it returns a boolean */
	NMBoolean handled= false;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config && config->NMHandleEvent)
		handled= config->NMHandleEvent(dialog, event, (NMProtocolConfigPriv*)config->configuration_data);

	return handled;
}

//----------------------------------------------------------------------------------------
// ProtocolHandleItemHit
//----------------------------------------------------------------------------------------

NMErr ProtocolHandleItemHit(
	DIALOGPTR dialog, 
	short inItemHit,
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMHandleItemHit)
		{
			err = config->NMHandleItemHit(dialog, inItemHit, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolDialogTeardown
//----------------------------------------------------------------------------------------

/* returns false if teardown is not possible (ie invalid parameters) */
NMBoolean ProtocolDialogTeardown(
	DIALOGPTR dialog, 
	NMBoolean update_config,
	PConfigRef config)
{
	NMBoolean can_teardown= true;
	NMErr	err;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMTeardownDialog)
		{
			can_teardown= config->NMTeardownDialog(dialog, update_config, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return can_teardown;
}

//----------------------------------------------------------------------------------------
// ProtocolGetRequiredDialogFrame
//----------------------------------------------------------------------------------------

void ProtocolGetRequiredDialogFrame(
	RECT *r,
	PConfigRef config)
{
	NMErr err= kNMNoError;
	
	if(config)
	{
		if(config->NMGetRequiredDialogFrame)
		{
			config->NMGetRequiredDialogFrame(r, (NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}
}

//----------------------------------------------------------------------------------------
// ProtocolBindEnumerationToConfig
//----------------------------------------------------------------------------------------

NMErr ProtocolBindEnumerationToConfig(
	PConfigRef config, 
	NMHostID inID)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMBindEnumerationItemToConfig)
		{
/* 19990124 sjb propagate err from bind */
			err = config->NMBindEnumerationItemToConfig((NMProtocolConfigPriv*)config->configuration_data, inID);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolStartEnumeration
//----------------------------------------------------------------------------------------

NMErr ProtocolStartEnumeration(
	PConfigRef config, 
	NMEnumerationCallbackPtr inCallback, 
	void *inContext, 
	NMBoolean inActive)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMStartEnumeration)
		{
/* 19990124 sjb propagate err from enumeration */
			err = config->NMStartEnumeration((NMProtocolConfigPriv*)config->configuration_data, inCallback, inContext, inActive);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolIdleEnumeration
//----------------------------------------------------------------------------------------

NMErr ProtocolIdleEnumeration(
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMIdleEnumeration)
		{
/* 19990124 sjb propagate err from idle */
			err = config->NMIdleEnumeration((NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}

//----------------------------------------------------------------------------------------
// ProtocolEndEnumeration
//----------------------------------------------------------------------------------------

NMErr ProtocolEndEnumeration(
	PConfigRef config)
{
	NMErr err= kNMNoError;

	/* call as often as possible (anything that is synchronous) */
	op_idle_synchronous(); 
	
	if(config)
	{
		if(config->NMEndEnumeration)
		{
/* 19990124 sjb propagate err from end enum */
			err = config->NMEndEnumeration((NMProtocolConfigPriv*)config->configuration_data);
		} else {
			err= errFunctionNotBound;
		}
	} else {
		err= errParamErr;
	}

	return err;
}
