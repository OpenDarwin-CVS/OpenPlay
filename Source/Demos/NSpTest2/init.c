/***********************************************************************
#
#		init.c
#
#		basic initialization code.
#
#		Author: Michael Marinkovich
#				Apple Developer Technical Support
#
#
#		Modification History: 
#
#			6/4/95		MWM 	Initial coding					 
#			10/12/95	MWM		cleaned up
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#		Copyright � 1992-95 Apple Computer, Inc., All Rights Reserved
#
#
***********************************************************************/

#if (!macho_build)
	#include <AppleEvents.h>
	#include <Displays.h>
	#include <Events.h>
	#include <Fonts.h>
	#include <Gestalt.h>
	#include <Menus.h>
	#include <OSUtils.h>
#endif

#include "App.h"
#include "Proto.h"
#include "NetStuff.h"

extern Boolean 			gHasDMTwo;
extern Boolean 			gHasDrag;
extern Boolean			gInBackground;
extern short			gWindCount;


//----------------------------------------------------------------------
//
//	Initialize - the main entry point for the initialization
//
//
//----------------------------------------------------------------------

OSErr Initialize(void)
{
	OSErr		err = noErr;
	OSStatus	status;	
	
	ToolBoxInit();
	CheckEnvironment();
	err = InitApp();
	status = InitNetworking('BLAm');
	
	return err;
}


//----------------------------------------------------------------------
//
//	ToolBoxInit - initialization all the needed managers
//
//
//----------------------------------------------------------------------

void ToolBoxInit(void)
{

	#if (!carbon_build)
		InitGraf(&qd.thePort);
		InitFonts();
		InitWindows();
		InitMenus();
		TEInit();
		InitDialogs(nil);
	#endif //!carbon_build
	
	InitCursor();
		
	FlushEvents(everyEvent, 0);

}


//----------------------------------------------------------------------
//
//	CheckEnvironment - make sure we can run with current sys and managers.
//					   Also initialize globals - have drag & drop
//					  
//----------------------------------------------------------------------

void CheckEnvironment(void)
{
	
	// your stuff here
}


//----------------------------------------------------------------------
//
//	InitApp - initialization all the application specific stuff
//
//
//----------------------------------------------------------------------

OSErr InitApp(void)
{
	OSErr				err;

	// init AppleEvents
	err = AEInit();
	MenuSetup();

	// init any globals
	gWindCount = 1;

	return err;
	
}


//----------------------------------------------------------------------
//
//	MenuSetup - 
//
//
//----------------------------------------------------------------------

void MenuSetup(void)
{
	Handle			menu;
		
		
	menu = GetNewMBar(rMBarID);		//	get our menus from resource
	SetMenuBar(menu);
	DisposeHandle(menu);
	
	#if (!carbon_build)
		AppendResMenu(GetMenuHandle(mApple ), 'DRVR');		//	add apple menu items
	#endif //!carbon_build

	DrawMenuBar();

		
}
