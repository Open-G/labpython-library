/*
 * Platform independant shared library module
 * ------------------------------------------
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
 */
static char *id="@(#) $Id: dynlib.c,v 1.4 2007/12/14 02:15:56 labviewer Exp $";

#include "dynlib.h"

ExtLib LoadExternalLib(ConstCStr path) {
#if MSWin
  return (ExtLib)LoadLibraryA((LPCSTR)path);
#elif FragInterface
  return NULL;
#elif DLInterface
  return (ExtLib)dlopen ((char*)path, RTLD_LAZY);
#else
  return NULL;
#endif
}

Bool32 FreeExternalLib(ExtLib lib) {
#if MSWin
  return FreeLibrary(lib);
#elif FragInterface
  return FALSE;
#elif DLInterface
  return dlclose(lib);
#else
  return FALSE;
#endif
}

ProcPtr LoadExternalSym(ExtLib lib, CStr name) {
#if MSWin
  return (ProcPtr)GetProcAddress(lib, name);
#elif FragInterface
  return NULL;
#elif DLInterface
  return (ProcPtr)dlsym(lib, (char*)name);
#else
  return NULL;
#endif
}

Bool32 LoadFuncIfNeeded(ExtLib lib, ProcPtr *ptr, CStr name) {
	if (!lib)
	  return FALSE;

	if (!*ptr)
	{
	  if (!(*ptr = LoadExternalSym(lib, name)))
	  {
	    DbgPrintf("Function %s could not be loaded!", name);
	    return FALSE;
	  }
	}
	return TRUE;
}
