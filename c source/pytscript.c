/*
 * LabVIEW Python Script Node Interface Wrapper
 * --------------------------------------------
 *
 * Allows to dynamically link to the Python Script Node DLL in resources/script
 * from VIs, without causing conflicts when sharing the script DLL by a script
 * node or a VI.
 *
 * Created by: Rolf Kalbermatter
 *
 *
 * Copyright (C) 2006-2007 Rolf Kalbermatter
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
 *
 *
 * $Log: pytscript.c,v $
 * Revision 1.4  2007/11/14 21:35:51  labviewer
 * Fixed loading the actual lvpython DLL from the script server DLL
 *
 * Revision 1.3  2007/11/02 11:41:26  labviewer
 * *** empty log message ***
 *
 * Revision 1.2  2006/10/25 10:43:02  labviewer
 * Various modifications
 *
 * Revision 1.1  2006/10/20 12:35:36  labviewer
 * Added a link wrapper to the script DLL
 *
 *
 * Revision 1.0  2006/10/19 rolfk
 * Initial
 */
static char *id="@(#) $Id: pytscript.c,v 1.4 2007/11/14 21:35:51 labviewer Exp $";

#define LVSNAPI_EXPORTS 1
#define COBJMACROS
#include "dynlib.h"
#include "lvtypedef.h"
#include "lvsnapi.h"

/********************************************************************************/
/*																				*/
/* Shared library entry point													*/
/*																				*/
/* This is fully platform dependant. Other platforms will need a differnt		*/
/* implemenation.					 											*/
/*																				*/
/********************************************************************************/

static ExtLib gLib = NULL;

#if MSWin
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
    {
	  case DLL_PROCESS_ATTACH:
		break;
	  case DLL_PROCESS_DETACH:
		if (gLib)
		{
		  FreeExternalLib(gLib);
		}
		gLib = NULL;
		break;
    }
    return TRUE;
}
static uChar LVPythonLib[] = "\014lvpython.dll";
#elif Unix
static uChar LVPythonLib[] = "\013lvpython.so";
#elif MacOSX
static uChar LVPythonLib[] = "\013lvpython.framework";
#elif Mac
static uChar LVPythonLib[] = "\016lvpython.shlib";
#endif

Bool32 LoadFuncIfNeededEx(ExtLib *lib, ProcPtr *ptr, CStr name)
{
	if (!*lib)
	{
      Debugger();
	  
	  /* Try to load the lib name only. This should succeed in an
         executable where the lib has been placed in the same directory
         as the executable. */
      *lib = LoadExternalLib(&LVPythonLib[1]);
	  if (!*lib)
      {
	    MgErr err;
	    Path path = FEmptyPath(NULL);

        /* We couldn't load the lib, try to load it from the default location */
	    if (!path)
	      return FALSE;

        err = FAppPath(path);
	    if (!err)
	      err = FAppendName(path, "\010user.lib");
	    if (!err)
	      err = FAppendName(path, "\012_OpenG.lib");
	    if (!err)
	      err = FAppendName(path, "\011labpython");
	    if (!err)
	      err = FAppendName(path, LVPythonLib);

	    if (!err)
	    {
	      LStrHandle p = NULL;

	      err = FPathToDSString(path, &p);
	      if (!err)
	      {
	        DSSetHandleSize(p, LStrLen(*p) + 1);
	        LStrBuf(*p)[LStrLen(*p)] = 0;
	        *lib = LoadExternalLib(LStrBuf(*p));
            if (!*lib)
              DbgPrintf("Could not load library: %H!", p);
            DSDisposeHandle(p);
	      }
	    }
	    FDisposePath(path);
      }
	}

	if (*lib)
    {
      if (!*ptr)
	    *ptr = LoadExternalSym(*lib, name);

      if (*ptr)
        return TRUE;

	  DbgPrintf("Function %s could not be loaded!", name);
	}
	return FALSE;
}
/********************************************************************************/
/*																				*/
/* Functions implementation														*/
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
lvsnAPI(Bool32) lvsnImplementsVersion(uInt32 version) {
	if (version == 1) {
		return lvsnSUCCESS;
	}
	return lvsnFAILURE;
}

/********************************************************************************/
/*																				*/
/* uInt32 lvsnFirstImplementedVersion(void)	(optional)							*/
/* uInt32 lvsnLastImplementedVersion(void) (optional)							*/
/*																				*/
/* These two functions allow to indicate a range of script interface versions	*/
/* which might be supported by this library. Currently the only version is 1.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(uInt32) lvsnFirstImplementedVersion(void) {
	return 1;
}

lvsnAPI(uInt32) lvsnLastImplementedVersion(void) {
	return 1;
}

/********************************************************************************/
/*																				*/
/* CStr lvsnGetScriptName(void)													*/
/*																				*/
/* Returns the name of the script interface implemented by this DLL.		 	*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
static char scriptName[12];

lvsnAPI(CStr) lvsnGetScriptName(void) {
	CStr str = (CStr)scriptName;
	StrCpy(scriptName, "Python");
#if 0
	if (PyOpenHost(FALSE)) {
		str += StrLen(scriptName);
		StrNCpy(str, (CStr)Py_GetVersion(), 3);
	}
#endif
	return (CStr)scriptName;
}

DefineExtFuncEx(&gLib, Bool32, lvsnServerSupportTypes, (int16*** typedefs, CStr **names, int32 *num), (typedefs, names, num));
DefineExtFuncEx(&gLib, lvsnInstance, lvsnInitNew, (uInt32 version), (version));
DefineExtFuncExVoid(&gLib, lvsnCleanup, (lvsnInstance inst), (inst));
DefineExtFuncEx(&gLib, Bool32, lvsnSetScript, (lvsnInstance inst, CStr lpScriptText), (inst, lpScriptText));
DefineExtFuncEx(&gLib, Bool32, lvsnGetScript, (lvsnInstance inst, CStr lpScriptText, int32 slen), (inst, lpScriptText, slen));
DefineExtFuncEx(&gLib, Bool32, lvsnSetLabVIEWData, (lvsnInstance inst, CStr name, UPtr data, int16 *tdp), (inst, name, data, tdp));
DefineExtFuncEx(&gLib, Bool32, lvsnGetLabVIEWData, (lvsnInstance inst, CStr name, UPtr data, int16 *tdp), (inst, name, data, tdp));
DefineExtFuncEx(&gLib, Bool32, lvsnCompile, (lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText), (inst, eStart, eEnd, eText));
DefineExtFuncEx(&gLib, Bool32, lvsnExecute, (lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText), (inst, eStart, eEnd, eText));
