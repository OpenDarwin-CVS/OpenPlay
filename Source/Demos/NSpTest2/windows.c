/*************************************************************************************
#
#		Windows.c
#
#		This segment handles the window creation, close, updates,
#
#		Author(s): 	Michael Marinkovich
#					Apple Developer Technical Support
#					marink@apple.com
#
#		Modification History: 
#
#			2/10/96		MWM 	Initial coding					 
#			3/2/02		ECF		Carbon/OpenPlay port				 
#
#		Copyright � 1992-96 Apple Computer, Inc., All Rights Reserved
#
#
#		You may incorporate this sample code into your applications without
#		restriction, though the sample code has been provided "AS IS" and the
#		responsibility for its operation is 100% yours.  However, what you are
#		not permitted to do is to redistribute the source as "DSC Sample Code"
#		after having made changes. If you're going to re-distribute the source,
#		we require that you make it clear in the source that the code was
#		descended from Apple Sample Code, but that you've made changes.
#
*************************************************************************************/

#if (!macho_build)
	#include <MacWindows.h>
	#include <NumberFormatting.h>
	#include <PLStringFuncs.h>
#endif

#include "App.h"
#include "Proto.h"
#include "netstuff.h"

//----------------------------------------------------------------------
//
//	Globals 
//
//----------------------------------------------------------------------

extern Boolean		gHasAbout;		// have an about box?
extern short		gWindCount;


//----------------------------------------------------------------------
//
//	CreateWindow - create a window from the info passed in. Will try to 
//				   load from resource if resID is supplied.
//
//----------------------------------------------------------------------

WindowPtr CreateWindow(short resID, void *wStorage, Rect *bounds,  Str255 title,
						Boolean visible, short procID , short kind,
						WindowRef behind, Boolean goAwayFlag, long refCon)
{
	OSErr			err = nil;
	WindowRef		newWindow = nil;
	
	
	if (resID != nil) 		// if res id isn't nil then load from disk
		newWindow = GetNewCWindow(resID, wStorage, behind);
	else					// otherwise make a new windowRecord
	 	newWindow = NewCWindow(wStorage, bounds, title, visible, 
							   procID, behind, goAwayFlag, refCon);
							   
	if (newWindow != nil) 
	{
		NewWindowTitle(newWindow, title);
		err = InitWindowProcs(newWindow, kind);
		
		if (err == noErr) 
		{
			if (visible)
			{
				#if (carbon_build)
					SetPortWindowPort(newWindow);
				#else
					SetPort(newWindow);
				#endif //carbon_build
				ShowWindow(newWindow);
			}
		}
				
		// initialization of Document Record faild
		// so kill the return window	
		else 
		{
			newWindow = nil;
			HandleError(err, false);
		}		
	}
	
	return newWindow;

}
	

//----------------------------------------------------------------------
//
//	RemoveWindow - applications Doc window disposal routine.
//				 
//
//----------------------------------------------------------------------

OSErr RemoveWindow( WindowRef window )
{
	OSErr			err = nil;
	
	DoCloseNetWindow(window);
	
	return err;
}		


//----------------------------------------------------------------------
//
//	DisposeWindowStructure - dispose of Document Record prior to
//				 		     releasing the window.
//
//----------------------------------------------------------------------

void DisposeWindowStructure(DocHnd doc)
{	
	if (doc != nil)
	{
		HLock((Handle) doc);
		
		// go through the fields of the DocHnd and 
		// dispose of memory that we allocated.
		if ((**doc).hScroll)
			DisposeControl((**doc).hScroll);
	
		if ((**doc).vScroll)
			DisposeControl((**doc).vScroll);
			
		if ((**doc).world)
			DisposeGWorld((**doc).world);
		
		if ((**doc).pict)
			KillPicture((**doc).pict);
		
		HUnlock((Handle) doc);
		DisposeHandle((Handle) doc);
	}
	

}


//----------------------------------------------------------------------
//
//	NewWindowTitle - if supplied title is nil then title is set to 
//				 	 global "gWindCount".
//
//----------------------------------------------------------------------

void NewWindowTitle(WindowRef window, Str255 str)
{
	Str255		catStr = "\pConnection ";
	Str255		newStr;
	
	
	if (str == nil || StrLength(str) == 0) 
	{
		PLstrcpy(newStr, catStr);
		NumToString(gWindCount, catStr);
		PLstrcat(newStr, catStr);
		gWindCount++;
		
		SetWTitle(window,newStr);
	}
	
	else
		SetWTitle(window, str);

}


//----------------------------------------------------------------------
//
//	InitWindowProcs - init a window with proper callback event. Fills 
//				 	  out custom procs for different windowkinds.
//
//----------------------------------------------------------------------

OSErr InitWindowProcs(WindowRef window, short windKind)
{
	OSErr			err = nil;
	DocHnd			doc;
	
	doc = (DocHnd)NewHandle(sizeof(DocRec));
	if (doc != nil) 
	{
		SetWRefCon(window, (long)doc);

		switch(windKind) 
		{
			case kDocKind:
				(**doc).idleProc		= DoIdle;
				(**doc).mMenuProc		= HandleMenuChoice;
				(**doc).inContentProc	= HandleContentClick;
				(**doc).inGoAwayProc	= nil;
				(**doc).inZoomProc		= HandleZoomClick;
				(**doc).inGrowProc		= HandleGrow;
				(**doc).keyProc			= nil;
				(**doc).activateProc	= DoActivate;
				(**doc).updateProc		= DrawWindow;	
				(**doc).hScroll 		= nil;
				(**doc).vScroll			= nil;
				(**doc).world			= nil;
				(**doc).pict			= nil;
//				(**doc).printer			= nil;
				(**doc).dirty			= false;
				
				break;
				
			case kDialogKind:
				break;

			case kAboutKind:
				(**doc).idleProc		= DoIdle;
				(**doc).mMenuProc		= HandleMenuChoice;
				(**doc).inContentProc	= nil;
				(**doc).inGoAwayProc	= nil;
				(**doc).inZoomProc		= nil;
				(**doc).inGrowProc		= nil;
				(**doc).keyProc			= nil;
				(**doc).activateProc	= nil;
				(**doc).updateProc		= DrawAboutWindow;	
				(**doc).hScroll 		= nil;
				(**doc).vScroll			= nil;
				(**doc).world			= nil;
				(**doc).pict			= nil;
				(**doc).dirty			= false;
				
				break;
				
			default:
				err = 25;
				break;	
		}
		
		#if (carbon_build)
			SetWindowKind(window,windKind);
		#else
			((WindowPeek)window)->windowKind = windKind;
		#endif //carbon_build
	}			
	return err;

}


		
//----------------------------------------------------------------------
//
//	DrawWindow - custom proc that is called to update window contents.
//				 
//
//----------------------------------------------------------------------

void DrawWindow(WindowRef window, void *refCon)
{
	DocHnd			doc;
	
	doc = (DocHnd)GetWRefCon(window);	
	if (doc != nil) 
	{
	}
	
}


//----------------------------------------------------------------------
//
//	DrawAboutWindow - custom proc that is called to update about window.
//				 
//
//----------------------------------------------------------------------

void DrawAboutWindow( WindowRef window, void *refCon )
{	
	CGrafPtr		oldPort;
	GDHandle		oldGD;
	PicHandle		thePict;
	
	GetGWorld(&oldPort, &oldGD);
	
	thePict = GetPicture(rAboutPictID);
	
	if (thePict != nil)
	{
		SetGWorld((CGrafPtr)window, nil);
		
		#if (carbon_build)
		{
			Rect portRect;
			GetPortBounds(GetWindowPort(window),&portRect);
			DrawPicture(thePict,&portRect);
		}
		#else	
			DrawPicture(thePict, &window->portRect);
		#endif //carbon_build
	}
	
	SetGWorld(oldPort, oldGD);

}


//----------------------------------------------------------------------
//
//	DoResizeWindow - custom proc that is called to update window.
//				 
//
//----------------------------------------------------------------------

void DoResizeWindow (WindowRef window) 
{

}


//----------------------------------------------------------------------
//
//	GetWindKind - returns the windowkind.
//				 
//
//----------------------------------------------------------------------

short GetWindKind(WindowRef window)
{

	#if (carbon_build)
		return GetWindowKind(window);
	#else
		return ((WindowPeek)window)->windowKind;
	#endif //carbon_build	

}


//----------------------------------------------------------------------
//
//	GetIsAppWindow - is the window a 'userKind'.
//				 
//
//----------------------------------------------------------------------

Boolean GetIsAppWindow(WindowRef window)
{
	return (GetWindKind(window) == kDocKind);

}


//----------------------------------------------------------------------
//
//	GetIsAboutWindow - is the window an about box.
//				 
//
//----------------------------------------------------------------------

Boolean GetIsAboutWindow(WindowRef window)
{
	return (GetWindKind(window) == kAboutKind);
	
}
