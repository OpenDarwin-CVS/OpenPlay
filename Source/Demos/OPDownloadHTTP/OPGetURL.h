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
 
#include "OpenPlay.h"

typedef struct OPHTTPDownload *OPHTTPDownloadPtr;

typedef enum
{
	kDLStatusRunning = 0,
	kDLStatusCantConnect,
	kDLStatusComplete,
	kDLStatusFailed
}OPHTTPDownloadStatus;

/* Get ready to rumble.  On mac carbon Open-Transport based builds, you can optionally pass
an OTContext to be used - otherwise a new context will be inited using kInitOTForApplicationMask. */

#if (carbon_build)
	void OPHTTPInit(OTClientContextPtr theOTContext);
#else
	void OPHTTPInit(void *unused);
#endif

/* Cleans up. */
void OPHTTPTerm(void);

/* Start a new download.  This never fails - any errors are returned with the first call to ProcessOPHTTPDownload. */
OPHTTPDownload* NewOPHTTPDownload(char *url, NMBoolean skipHeader);

/* Disposes a download - cancels if not finished. */
void	DisposeOPHTTPDownload(OPHTTPDownload *theDownload);

/* Gives the download processing time and returns any new data. */
OPHTTPDownloadStatus	ProcessOPHTTPDownload(OPHTTPDownload *theDownload, char *buffer, NMUInt32 *bufferSize);
