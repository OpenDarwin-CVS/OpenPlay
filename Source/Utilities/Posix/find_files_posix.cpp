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


#include "portable_files.h"
#include "find_files.h"

/* --------- local prototypes */
static int alphabetical_names(const FileDesc *a, const FileDesc *b);

static FileError enumerate_files(struct find_file_pb *param_block);
	
/* ---------------- Parameter Block Version */
FileError find_files(
	struct find_file_pb *param_block)
{
  FileError err = 0;


  op_assert(param_block);

  /* If we need to create the destination buffer */
  if(param_block->flags & _ff_create_buffer) 
  {
    op_assert(param_block->search_type ==_fill_buffer);
    param_block->buffer = (FileDesc *) new_pointer(sizeof(FileDesc)*param_block->max);
  }

  /* Assert that we got it. */
  op_assert(param_block->search_type == _callback_only || param_block->buffer);
  op_assert(param_block->version == 0);

  /* Set the variables */
  param_block->count = 0;

  {
    err = enumerate_files(param_block);
  }

	/* Alphabetical */
	if(param_block->flags & _ff_alphabetical)
	{
		op_assert(param_block->search_type==_fill_buffer);
		qsort(param_block->buffer, param_block->count, 
			sizeof(FileDesc), (int (*)(const void *, const void *))alphabetical_names);
	}

	/* If we created the buffer, make it the exact right size */
	if(param_block->flags & _ff_create_buffer)
	{
		op_assert(param_block->search_type==_fill_buffer);
		param_block->buffer= (FileDesc *)realloc(param_block->buffer, sizeof(FileDesc)*param_block->count);
	}
	
	return err;
}

NMBoolean equal_filedescs(
	FileDesc *a, 
	FileDesc *b)
{
  NMBoolean equal = false;
	
  if ( strcmp((char*)a->name, (char*)b->name) == 0 )
    equal = true;
	
	return equal;
}

/* --------- Local Code --------- */

static int alphabetical_names(
	const FileDesc *a, 
	const FileDesc *b)
{
  return( strcmp((char*)a->name, (char*)b->name) );
}

static FileError enumerate_files(struct find_file_pb *param_block)
{
  NMBoolean result;
  FileDesc  temp_file;
  DIR       *dir_stream = NULL;
  struct dirent  *dir_entry;

  
  dir_stream = opendir((char*)param_block->start_search_from.name);

  if (dir_stream)
  {
    dir_entry = readdir(dir_stream);

    while(dir_entry)
    {
      strcpy((char*)temp_file.name, dir_entry->d_name);

      if ((param_block->search_type == _callback_only) && (param_block->callback))
      {
        if (param_block->flags & _ff_callback_with_catinfo)
	{
          result = param_block->callback(&temp_file, (void *)param_block->start_search_from.name); /* callback with catinfo */
        }
        else
          result = param_block->callback(&temp_file, param_block->user_data); /* callback with user data */
      } 

      dir_entry = readdir(dir_stream);
    }
  }
  else
    return(errno);
    
 return errno;

}
