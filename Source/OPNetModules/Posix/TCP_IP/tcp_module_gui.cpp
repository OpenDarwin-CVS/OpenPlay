/* 
 * File: tcp_module_gui.c
 *-------------------------------------------------------------
 * Description:
 *   Functions which handle user interface interaction
 *
 *------------------------------------------------------------- 
 *   Author: Kevin Holbrook
 *  Created: June 23, 1999
 *
 * Modified: $Date$
 * Revision: $Id$
 *
 *-------------------------------------------------------------
 *          Copyright (c) 1999 Kevin Holbrook
 *-------------------------------------------------------------
 */


#include "OPUtils.h"
#include "NetModule.h"


/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMSetupDialog(	DIALOGPTR 			dialog, 
						NMSInt16 			frame, 
						NMSInt16			inBaseItem, 
						NMConfigRef			inConfig)
{
	return kNMInternalErr;
} /* NMSetupDialog */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMBoolean NMHandleEvent(	DIALOGPTR			dialog, 
							EVENT *				event, 
							NMConfigRef 		inConfig)
{
	return false;
} /* NMHandleEvent */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

NMErr NMHandleItemHit(	DIALOGPTR			dialog, 
						NMSInt16			inItemHit, 
						NMConfigRef 		inConfig)
{
	return kNMInternalErr;
} /* NMHandleItemHit */


/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */


NMBoolean NMTeardownDialog(	DIALOGPTR 			dialog, 
							NMBoolean			inUpdateConfig, 
							NMConfigRef 		ioConfig)
{
	return false;
} /* NMTeardownDialog */



/* 
 * Function: 
 *--------------------------------------------------------------------
 * Parameters:
 *  
 *
 * Returns:
 *  
 *
 * Description:
 *   Function 
 *
 *--------------------------------------------------------------------
 */

void NMGetRequiredDialogFrame(	RECT *			r, 
								NMConfigRef 	inConfig)
{
	r->left = 0;
	r->right = 0;
	r->top = 0;
	r->bottom = 0;
} /* NMGetRequiredDialogFrame */

