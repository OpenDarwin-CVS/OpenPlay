/* 
 * File: tcp_module_main.c
 *-------------------------------------------------------------
 * Description:
 *   Functions which are main entry points for module library.
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


#include "NetModule.h"
#include "tcp_module.h"
#include "OPUtils.h"
#include <stdio.h>

#if (posix_build)
	#include <pthread.h>
#endif

//  ------------------------------  Private Prototypes
extern "C"{
void _init(void);
void _fini(void);

#if (windows_build)
		BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);
#endif
}

//	------------------------------ Variables
static NMModuleInfo		gModuleInfo;
static const NMUInt32 moduleID = 'Inet';
static const char *kModuleName = "TCP/IP";
static const char *kModuleCopyright = "1996-1999 Apple Computer, Inc.";

NMEndpointPriv *endpointList = NULL;
NMUInt32 endpointListState = 0;
machine_lock *endpointListLock; //dont access the list without locking it!
machine_lock *notifierLock; //dont call the user back without locking it!

NMSInt32 module_inited = 0;

#if (windows_build)
	HINSTANCE application_instance;
#endif

extern "C"{

/* 
 * Function: _ini
 *--------------------------------------------------------------------
 * Parameters:
 *   none
 *
 * Returns:
 *   none
 *
 * Description:
 *   Function called when shared library is first loaded by system.
 *
 *--------------------------------------------------------------------
 */


void _init(void)
{
	DEBUG_ENTRY_EXIT("_init");
	
	
	// ecf - on the OSX bundle based builds, we don't seem to be re-initialized
	// so we have to share globals and _init may be called multiple times on the same
	// instance of our module - we must be prepared.
	module_inited++;
	if (module_inited != 1)
		return;
	//op_assert(module_inited == false);
	
	gModuleInfo.size= sizeof (NMModuleInfo);
	gModuleInfo.type = moduleID;
	strcpy(gModuleInfo.name, kModuleName);
	strcpy(gModuleInfo.copyright, kModuleCopyright);
	gModuleInfo.maxPacketSize = 1500;		// <ECF> is this right? i just copied from the mac one...
	gModuleInfo.maxEndpoints = kNMNoEndpointLimit;
	gModuleInfo.flags= kNMModuleHasStream | kNMModuleHasDatagram | kNMModuleHasExpedited;

	//create the lock for our main list
	endpointListLock = new machine_lock;
	notifierLock = new machine_lock;	
} /* _init */


/* 
 * Function: _fini
 *--------------------------------------------------------------------
 * Parameters:
 *   none
 *
 * Returns:
 *   none
 *
 * Description:
 *   Function called when shared library is unloaded by system.
 *
 *--------------------------------------------------------------------
 */

void _fini(void)
{
	DEBUG_ENTRY_EXIT("_fini");

	module_inited--;
	op_assert(module_inited >= 0);
	if (module_inited != 0)
		return;
			
	//op_assert(module_inited == true);

	//if we have a worker thread, kill it
	#if USE_WORKER_THREAD
		killWorkerThread();
	#endif
	
	//on widnows, kill winsock
	#if (windows_build)
		shutdownWinsock();
	#endif
	
	delete endpointListLock;
	delete notifierLock;
	
} /* _fini */


/* 
 *	Function: DllMain
	called when windows dll is loaded/unloaded.  Simply hooks to _init/_fini
 */
#if (windows_build)

BOOL WINAPI
DllMain(
	HINSTANCE	hInst,
	DWORD		fdwReason,
	LPVOID		lpvReserved)
{
	BOOL     success = true;			// ignored on everything but process attach

	UNUSED_PARAMETER(hInst)
	UNUSED_PARAMETER(lpvReserved)

	DEBUG_ENTRY_EXIT("DllMain");
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// we just got initialized.  Note we can use the TlsAlloc to alloc thread local storage
			// lpvReserved is NULL for dynamic loads, non-Null for static loads
			application_instance = hInst;
			_init();
			break;
			
		case DLL_THREAD_ATTACH:
			// called when a process creates another thread, so we can bind to it if we want..
			break;
			
		case DLL_THREAD_DETACH:
			// note this isn't necessary balanced by DLL_THREAD_ATTACH (ie first thread, or 
			//  if thread was already running before loading the dll)
			break;
			
		case DLL_PROCESS_DETACH:
			// Process exited, or FreeLibrary...
			// lpvReserved is NULL if called by FreeLibrary and non-NULL if called by process termination
			_fini();
			break;
	}
	
	return success;
}
#endif

/* 
 * Function: NMGetModuleInfo
 *--------------------------------------------------------------------
 * Parameters:
 *   [IN/OUT] module_info = ptr to module information structure to
 *                          be filled in.
 *
 * Returns:
 *   Network module error code
 *     kNMNoError            = succesfully got module information
 *     kNMParameterError     = module_info was not a valid pointer
 *     kNMModuleInfoTooSmall = size of passed structure was wrong
 *
 * Description:
 *   Function to get network module information.
 *
 *--------------------------------------------------------------------
 */

NMErr NMGetModuleInfo(NMModuleInfo *module_info)
{
	DEBUG_ENTRY_EXIT("NMGetModuleInfo");
	if (module_inited < 1){
		op_warn("NMGetModuleInfo called when module not inited");
		return kNMInternalErr;
	}
	
  /* validate pointer */
  if (!module_info)
    return(kNMParameterErr);

  /* validate size of structure passed to us */
  if (module_info->size >= sizeof(NMModuleInfo))
  {
	short	size_to_copy = (module_info->size<gModuleInfo.size) ? module_info->size : gModuleInfo.size;
	
		machine_move_data(&gModuleInfo, module_info, size_to_copy);
  
    return(kNMNoError);
  }
  else
  {
    return(kNMModuleInfoTooSmall);
  }

} /* NMGetModuleInfo */


} //extern C