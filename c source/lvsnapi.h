/*
 * LabVIEW Script Node Definition
 * ------------------------------
 *
 * Compiling a shared library module with these source code and installing it
 * in the <labview directory>/resource/scripts directory will allow LabVIEW
 * to see it and allow you to make use of the Python interpreter in a LabVIEW
 * script node.
 * Script Nodes are theoretically platform independant but no tests have been
 * done yet, that this feature is actually implemented or enabled at all in
 * other LabVIEW platforms than Win32. Even though there may be no script node
 * interface available for your platform, LabVIEW may still support it!	Only
 * trial and error will show ;-).
 *
 * Created by: Rolf Kalbermatter
 *
 * @(#) $Id: lvsnapi.h,v 1.4 2007/12/14 00:59:36 labviewer Exp $";
 *
 * Copyright (C) 2000 - 2007 Rolf Kalbermatter
 * License: GNU LGPL
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License is included in this
 * distribution in the file COPYING.LIB. If you did not receive this copy
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA.
 */
#ifndef LVSNAPI_H
#define LVSNAPI_H

#include "extcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MSWin
# ifdef _DEBUG
#  define DEBUG	1
#  if ProcessorType==kX64
#	define Debugger()	__debugbreak()
#  else
#	define Debugger()	{__asm{int 3}}
#  endif
#  define DebugPrintf		DbgPrintf
# else
#  define DEBUG	0
#  define Debugger()
   int DebugPrintf(CStr fmt, ...);
# endif

# ifdef LVSNAPI_EXPORTS
#  define lvsnAPI(type)	__declspec( dllexport ) type __cdecl
# else
#  define lvsnAPI(type)	type __cdecl
# endif

# define WindowPtr HWND

#elif Unix
# if defined(DEBUG)
#  undef DEBUG
#  define DEBUG 1
#  if Linux 
#   define Debugger()	__asm__ ("int3")
#  else
#   define Debugger()	{something}
#  endif
#  define DebugPrintf		DbgPrintf
# else
#  define DEBUG	0
#  define Debugger()
   int DebugPrintf(CStr fmt, ...);
# endif

# ifdef LVSNAPI_EXPORTS
#  define lvsnAPI(type)	type /*__attribute__ ((dllexport)) */
# else
#  define lvsnAPI(type)	type /* __attribute__ ((dllimport)) */
# endif

# define WindowPtr int32

#else /* Mac, .... */
# error Compiler defines not evaluated yet

// WindowPtr is already defined in MacOS

#endif

#define lvsnSUCCESS	1
#define lvsnFAILURE	0
#define kMaxErrStringLength 0x400

typedef struct
{
	int32 len;
	LStrHandle elm[4];
} **LStrArrayHdl;

#if !defined(LV_PRIVATE_POINTER)
 #define LV_PRIVATE_POINTER(p) PrivateP(p)            // >= LV 8.5
#endif

LV_PRIVATE_POINTER(lvsnInstance);

/********************************************************************************/
/*																				*/
/* Function prototypes															*/
/*																				*/
/* These functions need to be exported from the shared library which is placed 	*/
/* into the "<labview directory>/resource/scripts directory.					*/
/*																				*/
/********************************************************************************/

/********************************************************************************/
/*																				*/
/* Bool32 lvsnImplementsVersion(uInt32 version)									*/
/*																				*/
/* Returns lvsnSUCESS if the requested version number is supported, lvsnFAILED	*/
/* otherwise.  Currently the only possible version is 1.						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnImplementsVersion(uInt32 version);

/********************************************************************************/
/*																				*/
/* uInt32 lvsnFirstImplementedVersion(void)	(optional)							*/
/* uInt32 lvsnLastImplementedVersion(void) (optional)							*/
/*																				*/
/* These two functions allow to indicate a range of script interface versions	*/
/* which might be supported by this library. Currently the only version is 1.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(uInt32) lvsnFirstImplementedVersion(void);
lvsnAPI(uInt32) lvsnLastImplementedVersion(void);

/********************************************************************************/
/*																				*/
/* CStr lvsnGetScriptName(void)													*/
/*																				*/
/* Returns the name of the script interface implemented by this DLL.		 	*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(CStr) lvsnGetScriptName(void);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnServerSupportType(int16*** typedefs, CStr **names, int32 *num)	*/
/*																				*/
/* Returns a list of supported data types and their names.					 	*/
/*																				*/
/*	typedefs is an array of pointers to 16 bit LabVIEW typedefs vectors			*/
/*	names is an array of pointers to C strings									*/
/*	num is filled with the number of types refered to in the first two arrays	*/
/*																				*/
/* Return lvsnSUCESS if successful, lvsnFAILURE otherwise 						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnServerSupportTypes(int16*** typedefs, CStr **names, int32 *num); 

/********************************************************************************/
/*																				*/
/* lvsnInstance lvsnInitNew(uInt32 version)										*/
/*																				*/
/* Create a new session.													 	*/
/*																				*/
/*	version indicates the script node interface version to use. Currently the	*/
/*  only defined version is 1.													*/
/*																				*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(lvsnInstance) lvsnInitNew(uInt32 version);

/********************************************************************************/
/*																				*/
/* void lvsnBeginExecute(lvsnInstance inst) (optional)							*/
/* void lvsnEndExecute(lvsnInstance inst) (optional)							*/
/*																				*/
/* Called before and after LabVIEW executes a script. Usually this is used to 	*/
/* protect the DLL from reentrant execution through multiple LabVIEW threads.	*/
/*																				*/
/* If your interface is fully reentrant, you do not need to implement these		*/
/* functions at all.															*/
/*																				*/
/* The LabVIEW script node execution can call these functions from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(void) lvsnBeginExecute(lvsnInstance inst);
lvsnAPI(void) lvsnEndExecute(lvsnInstance inst);

/********************************************************************************/
/*																				*/
/* void lvsnCleanup(lvsnInstance inst)											*/
/*																				*/
/* Dispose of a session and all its resources created with lvsnInitNew.			*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(void) lvsnCleanup(lvsnInstance inst);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnSetScript(lvsnInstance inst, CStr lpScriptText)					*/
/*																				*/
/* Set the script text for the session.											*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnSetScript(lvsnInstance inst, CStr lpScriptText);

/********************************************************************************/
/*																				*/
/* int32 lvsnGetScript(lvsnInstance inst, CStr lpScriptText, int32 len) (optional) */
/*																				*/
/* Retrieves the script text associated with the passed in lvsnInstance. If		*/
/* lpScriptText is NULL return the length of the desired string to fill in the	*/
/* script text, otherwise return 0 if successfull or -1 in case of any failure.	*/
/*																				*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(int32) lvsnGetScript(lvsnInstance inst, CStr lpScriptText, int32 slen);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnSetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td)*/
/*																				*/
/* Set the data for the variable.											 	*/
/*																				*/
/*	name is the name of the variable											*/
/*	data is the actual LabVIEW data												*/
/*		for fixed size data it is directly a pointer to the data, for variable	*/
/*		sized data it is the pointer to a Handle								*/
/*	td is the LabVIEW type descriptor											*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnSetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnGetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td)*/
/*																				*/
/* Get the data for the variable.											 	*/
/*																				*/
/*	name is the name of the variable											*/
/*	data is the actual LabVIEW data												*/
/*		for fixed size data it is directly a pointer to the data, for variable	*/
/*		sized data it is the pointer to a Handle								*/
/*	td is the LabVIEW type descriptor											*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnGetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnCompile(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText) (optional) */
/*																				*/
/* Compile the script associated with the instance and return errors if any	 	*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnCompile(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText);

/********************************************************************************/
/*																				*/
/* Bool32 lvsnExecute(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText)*/
/*																				*/
/* Execute the script associated with the session and return errors if any	 	*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnExecute(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText);

#ifdef __cplusplus
}
#endif

#endif
