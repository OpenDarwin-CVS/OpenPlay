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

#ifndef __PORTABLE_FILES__
#define __PORTABLE_FILES__

#include "OpenPlay.h"

/*--------------------------------Windows Section-----------------------------*/
#if defined(windows_build)

  #define MAX_PATHNAME_LENGTH (256)
  #define EXTENSION_LENGTH (4) /* whee! */

  typedef struct
  {
    char name[MAX_PATHNAME_LENGTH];
  } FileDesc;


  #define d_LIBRARY_TYPE   'DLL '

  #define d_ERR_FILE_EOF        38  /* ERROR_HANDLE_EOF */
  #define d_ERR_FILE_NOTFOUND   2   /* ERROR_FILE_NOT_FOUND */


/*-------------------------------Macintosh Section----------------------------*/
#elif defined(macintosh_build)

//#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
//#endif

  typedef struct
  {
    short vRefNum;
    long parID;
    unsigned char name[64];
  } FileDesc; /* FileDesc==FileDescriptor */

//#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
//#endif

  #define d_LIBRARY_TYPE  kCFragLibraryFileType

  #define d_ERR_FILE_EOF       -39
  #define d_ERR_FILE_NOTFOUND  -43

/*----------------------------------Linux Section-----------------------------*/
#elif defined(posix_build)

#include <sys/types.h>
#include <sys/param.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

  typedef struct
  {
    char name[PATH_MAX];
  } FileDesc;


  #define d_LIBRARY_TYPE  1482  /* constant used for File Type in searching */

  #define d_ERR_FILE_EOF       1
  #define d_ERR_FILE_NOTFOUND  2

  #define d_OPENPLAY_LIB_LOCATION  "/usr/local/lib/OpenPlay"

/*------------------------------------Unknown---------------------------------*/
#else

  #error "Porting Error - Unknown target platform"

#endif
/*----------------------------------------------------------------------------*/

/* ------------- data types */

	typedef NMSInt16 fileref;            /* File descriptor, for portability */

//defined in the mac precomp header
#if !macintosh_build
typedef NMUInt32 FileType;   /* same as an OSType. For DOS, this is the extension... */
typedef NMErr FileError;          /* same as NMErr */
#endif //macintosh_build

/* FIXME - this enum needs a name! */
enum
{
  errHitFileEOF    = d_ERR_FILE_EOF,
  errFileNotFound  = d_ERR_FILE_NOTFOUND
};

#endif
