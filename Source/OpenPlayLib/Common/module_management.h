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
 */

#ifndef __MODULE_MANAGEMENT__
#define __MODULE_MANAGEMENT__

//  ------------------------------  Public Definitions

	#define MAXIMUM_NETMODULES			(64)

//	------------------------------	Public Functions

	extern NMErr 		bind_to_protocol(NMType protocol, FileDesc *spec, ConnectionRef *connection);
	extern void 		unbind_from_protocol(NMType protocol, NMBoolean synchronous);

	extern void 		add_endpoint_to_loaded_list(Endpoint *endpoint);
	extern void 		remove_endpoint_from_loaded_list(Endpoint *endpoint);

	extern NMBoolean	valid_endpoint(Endpoint *endpoint);

	extern NMSInt32 	count_endpoints_of_type(NMType type);

	extern void 		module_management_idle_time(void);

#endif	// __MODULE_MANAGEMENT__

