# Modified: $Date$
# Revision: $Id$

#where the makefile is
TOP = $(PWD)

#we building debug or release?
ifneq ($(OP_BUILD_TYPE),release)
	OP_BUILD_TYPE = debug
	OP_DEBUG_DEFINES = -DDEBUG -DDEBUGCALLCHAIN
endif

#store intermediates and targets based on architecture and machine
OSTYPE_DIR		= $(OSTYPE)-$(HOSTTYPE)
TARGET_DIR		= $(TOP)/../../Targets/posix/$(OSTYPE_DIR)/$(OP_BUILD_TYPE)
MODULES_DIR		= $(TOP)/../../Targets/posix/$(OSTYPE_DIR)/$(OP_BUILD_TYPE)/OpenPlay\ Modules
DOXYGEN_OP_DIR	= $(TOP)/../../Documentation/html

#stick object files in the dir for this os
OBJECT_DIR = objs_$(OSTYPE_DIR)-$(OP_BUILD_TYPE)
OUTPUT_OPTION = -o $(OBJECT_DIR)/$@

ifeq ($(OSTYPE),darwin)
	CC = cc	
	CXX = c++
	INOSTYPE = os_darwin
	GLOBAL_FLAGS = -Dmacho_build -Wno-four-char-constants
	OP_SHLIB_NAME = $(TARGET_DIR)/libopenplay.dylib
	APPFLAGS =	 -L$(TARGET_DIR)  -lopenplay -lpthread -lstdc++ 	
else
	CC = gcc
	INOSTYPE = os_posix_generic
	OP_SHLIB_NAME = $(TARGET_DIR)/libopenplay.so
	APPFLAGS = 	-L$(TARGET_DIR)  -ldl -lopenplay -lpthread -lstdc++	
endif

TCP_MODULE_NAME	= $(MODULES_DIR)/libtcp_ip.so
ENUM_TEST_NAME = $(TARGET_DIR)/openumtest
NSP_TEST_NAME = $(TARGET_DIR)/nsptest
OP_EXAMPLE1_NAME = $(TARGET_DIR)/opexample1
NSP_EXAMPLE1_NAME = $(TARGET_DIR)/nspexample1
MINI_PLAY_NAME = $(TARGET_DIR)/miniplay
OP_DOWNLOADHTTP_NAME = $(TARGET_DIR)/opdownloadhttp

#where to find sources/headers/libs  
HPATH1 = $(TOP)/../../Interfaces
HPATH2 = $(TOP)/../../Source/OpenPlayLib/Common
HPATH3 = $(TOP)/../../Source/OpenPlayLib/posix
HPATH4 = $(TOP)/../../Source/Utilities
HPATH5 = $(TOP)/../../Source/Utilities/posix
HPATH6 = $(TOP)/../../Source/OPNetModules
HPATH7 = $(TOP)/../../Source/OPNetModules/Common
HPATH8 = $(TOP)/../../Source/OPNetModules/posix/TCP_IP
HPATH9 = $(TOP)/../../Source/Demos/OPEnumTest
HPATH10 = $(TOP)/../../Source/NetSprocketLib
HPATH11 = $(TOP)/../../Source/Demos/NSpTestApp
HPATH12 = $(TOP)/../../Source/Demos/OPMiniDemo
HPATH13 = $(TOP)/../../Source/Demos/OPExample1
HPATH14 = $(TOP)/../../Source/Demos/OPDownloadHTTP
		
HINCLUDES = $(HPATH1) $(HPATH2) $(HPATH3) $(HPATH4) $(HPATH5) $(HPATH6)\
	$(HPATH7) $(HPATH8) $(HPATH9) $(HPATH10) $(HPATH11) $(HPATH12) $(HPATH13) $(HPATH14)
	
#where make looks for sources of any sort (besides the default obj dir)
#.PATH: $(HINCLUDES) $(OBJECT_DIR)
VPATH = $(HINCLUDES) $(OBJECT_DIR)

#summarize our flags to pass to the compiler
INCLUDE_FLAGS = -I$(HPATH1) -I$(HPATH2) -I$(HPATH3) -I$(HPATH4)\
	-I$(HPATH5) -I$(HPATH6) -I$(HPATH7) -I$(HPATH8) -I$(HPATH9) -I$(HPATH10) -I$(HPATH11) -I$(HPATH12) -I$(HPATH13) -I$(HPATH14)
CFLAGS = -g -fPIC -fno-common $(GLOBAL_FLAGS) -D$(INOSTYPE) $(OP_DEBUG_DEFINES) $(INCLUDE_FLAGS)
CXXFLAGS = $(CFLAGS)

#all objects we require for openplayLib
OP_LIB_OBJECTS =	module_management.o\
					op_endpoint.o\
					op_hi.o\
					op_module_mgmt.o\
					op_packet.o\
					openplay.o\
					openplay_posix.o\
					find_files_posix.o\
					dll_utils_posix.o\
					OPUtils.o\
					CATEndpoint_OP.o\
					CIPEndpoint_OP.o\
					CEndpoint_OP.o\
					EntryPoint.o\
					NetSprocketLib.o\
					NSpGame.o\
					NSpGameMaster.o\
					NSpGamePrivate.o\
					NSpGameSlave.o\
					NSpLists_OP.o\
					NSpProtocolRef.o\
					NSp_InterruptSafeList.o\
					machine_lock.o\
					ByteSwapping.o\
					String_Utils.o\
					ERObject.o\
					TPointerArray.o\
					DebugPrint.o\
					op_dlopen.o
				
TCP_MODULE_OBJECTS = 		tcp_module_communication.o\
							tcp_module_config.o\
							tcp_module_enumeration.o\
							tcp_module_gui.o\
							tcp_module_main.o\
							ip_enumeration.o\
							configuration.o\
							OPUtils.o\
							DebugPrint.o\
							machine_lock.o

ENUM_TEST_OBJECTS = OPEnumTest.o

NSP_TEST_OBJECTS = NSpTestApp.o

MINI_PLAY_OBJECTS = miniplay.o

OP_EXAMPLE1_OBJECTS = 	main.o\
						op_network.o\
						OPUtils.o

NSP_EXAMPLE1_OBJECTS =  main.o\
						nsp_network.o
			
OP_DOWNLOADHTTP_OBJECTS = 	OPDownloadHTTP.o\
							OPGetURL.o\
							String_Utils.o
							
################################################################################
#	TARGETS
################################################################################
							
#builds all - default target
MAIN: $(OP_SHLIB_NAME) $(TCP_MODULE_NAME) $(ENUM_TEST_NAME) $(NSP_TEST_NAME) $(OP_EXAMPLE1_NAME) $(NSP_EXAMPLE1_NAME) $(MINI_PLAY_NAME) $(OP_DOWNLOADHTTP_NAME)
	@echo openplay build complete

#clears out object files from the current posix build
clean:
	-rm -rf $(OBJECT_DIR)
	-rm -rf $(TARGET_DIR)
	-rm -rf $(MODULES_DIR)

#clears out all builds (cw, posix, etc)
clean_all: clean
	-rm -rf objs_*
	-rm -rf $(TOP)/../../Targets
	-rm -rf $(TOP)/../ProjectBuilder/OpenPlay.build
	-rm -rf $(TOP)/../CWPro7/*Data
	-rm -rf $(TOP)/../CWPro6/*Data
	-rm -rf $(TOP)/../VC/OpenPlayLib/Release
	-rm -rf $(TOP)/../VC/OpenPlayLib/Debug
	-rm $(TOP)/../VC/OpenPlayLib/*.opt
	-rm $(TOP)/../VC/OpenPlayLib/*.ncb
	-rm -rf $(TOP)/../VC/OPNetModulesWin/Release
	-rm -rf $(TOP)/../VC/OPNetModulesWin/Debug
	-rm $(TOP)/../VC/OPNetModulesWin/*.opt
	-rm $(TOP)/../VC/OPNetModulesWin/*.ncb
	-rm -rf $(TOP)/../VC/OPNetModulesWinOld/Release
	-rm -rf $(TOP)/../VC/OPNetModulesWinOld/Debug
	-rm $(TOP)/../VC/OPNetModulesWinOld/*.opt
	-rm $(TOP)/../VC/OPNetModulesWinOld/*.ncb
	
#construct required directories
build_dirs: 
	@echo creating directories
	@mkdir -p $(OBJECT_DIR)
	@mkdir -p $(TARGET_DIR)
	@mkdir -p $(MODULES_DIR)

#create the documentation using doxygen
docs:
	@rm -rf $(DOXYGEN_OP_DIR)
	@mkdir -p $(DOXYGEN_OP_DIR)
	@doxygen DoxygenConfigOP

#the library itself 
$(OP_SHLIB_NAME): build_dirs $(OP_LIB_OBJECTS)
	@echo updating $(OP_SHLIB_NAME)
ifeq ($(OSTYPE),darwin)
	cd $(OBJECT_DIR); $(CC) -dynamiclib -o $(OP_SHLIB_NAME)  $(OP_LIB_OBJECTS)
else
	cd $(OBJECT_DIR); ld -shared -o $(OP_SHLIB_NAME) $(OP_LIB_OBJECTS)
endif
	
#the ip module
$(TCP_MODULE_NAME):  build_dirs $(TCP_MODULE_OBJECTS)
	@echo updating $(TCP_MODULE_NAME)
ifeq ($(OSTYPE),darwin)	
	cd $(OBJECT_DIR); $(CC) -bundle -flat_namespace -o $(TCP_MODULE_NAME) $(TCP_MODULE_OBJECTS)
else
	cd $(OBJECT_DIR); ld -shared -o $(TCP_MODULE_NAME) $(TCP_MODULE_OBJECTS)
endif

#enumeration test
$(ENUM_TEST_NAME): build_dirs $(ENUM_TEST_OBJECTS)
	@echo updating $(ENUM_TEST_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(ENUM_TEST_NAME) $(ENUM_TEST_OBJECTS)

#nsp test
$(NSP_TEST_NAME): build_dirs $(NSP_TEST_OBJECTS)
	@echo updating $(NSP_TEST_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(NSP_TEST_NAME) $(NSP_TEST_OBJECTS)

#opexample1
$(OP_EXAMPLE1_NAME): build_dirs $(OP_EXAMPLE1_OBJECTS)
	@echo updating $(OP_EXAMPLE1_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(OP_EXAMPLE1_NAME) $(OP_EXAMPLE1_OBJECTS)

#nspexample1
$(NSP_EXAMPLE1_NAME): build_dirs $(NSP_EXAMPLE1_OBJECTS)
	@echo updating $(NSP_EXAMPLE1_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(NSP_EXAMPLE1_NAME) $(NSP_EXAMPLE1_OBJECTS)

#mini-play
$(MINI_PLAY_NAME): build_dirs $(MINI_PLAY_OBJECTS)
	@echo updating $(MINI_PLAY_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(MINI_PLAY_NAME) $(MINI_PLAY_OBJECTS)

#download-http
$(OP_DOWNLOADHTTP_NAME): build_dirs $(OP_DOWNLOADHTTP_OBJECTS)
	@echo updating $(OP_DOWNLOADHTTP_NAME)
	cd $(OBJECT_DIR); $(CC) $(APPFLAGS) -o $(OP_DOWNLOADHTTP_NAME) $(OP_DOWNLOADHTTP_OBJECTS)
