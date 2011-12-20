/*
 * Python to LabVIEW Script Node Interface
 * ---------------------------------------
 *
 * Import library to allow full dynamic linking to an external Python shared
 * library.
 *
 * Created by: Rolf Kalbermatter
 *
 *
 * Copyright (C) 2002-2007 Rolf Kalbermatter
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
static char *id="@(#) $Id: pyimport.c,v 1.16 2007/12/14 02:15:56 labviewer Exp $";
 
#include "lvpython.h"

#include "Python.h"
#include "compile.h"


/* This module looked like a good idea but Python is not really designed to link
   dynamically to its core DLL (yet). The whole internal layout of an object is not
   only fully exposed to the client, but it is virtually impossible to use Python
   objects without directly accessing that layout, which makes version independant
   applications basically totally impossible.
   For instance the debugging and release version use a differnt object size layout
   and there is for instance no API function to incref or decref the refcount of an
   object. It's all done with macros, tying the client thightly to the server.
   Also the special import conditions below are directly related to this difference.
   _Py_Dealloc and Py_RefTotal are both referenced by the Py_INCREF and Py_DECREF
   macros in debug mode. So using a non-debug Python DLL with this client compiled
   in debug mode will fail since the two symbols are not exposed by the non-debug DLL.
   That is not the worst. Mixing DLL and client debug mode will actually also surely
   result in crashes as the above mentioned incref and decref macros will cause
   modifications of parts of the object structure which are not related to the
   refcounting. The same is true for when different Python version would add
   information to the basic object layout
   This could easily be resolved if there would have been two exported functions such
   as PyObject_IncRef() and PyObject_DecRef() and although there might be good reasons
   to tie Python clients to the Python core DLL it seems not necessary to me to enforce
   that in such a strong manner. */

#ifdef PY_NO_IMPORT_LIB

#include "dynlib.h"

#ifdef Py_TRACE_REFS
 #if MSWin
  #define SHLIBpath "python25_d.dll"
 #elif Mac
  #define SHLIBpath "python25_d.shlb"
 #elif Unix
  #define SHLIBpath "libpython2.5_d.so"
 #endif
#else
 #if MSWin
  #define SHLIBpath "python25.dll"
 #elif Mac
  #define SHLIBpath "python25.shlb"
 #elif Unix
  #define SHLIBpath "libpython2.5.so"
 #endif
#endif

static ExtLib gLib = NULL;

#ifdef Py_TRACE_REFS
DefineExtFuncVoid(gLib, _Py_Dealloc, (PyObject *o), (o));
#endif
DefineExtFuncVoid(gLib, Py_Initialize, (void), ());
DefineExtFuncVoid(gLib, Py_Finalize, (void), ());
DefineExtFunc(gLib, PyThreadState*, Py_NewInterpreter, (void), ());
DefineExtFuncVoid(gLib, Py_EndInterpreter, (PyThreadState *t), (t));
DefineExtFunc(gLib, const char*, Py_GetVersion, (void), ());
//DefineExtFuncVarArg(gLib, int, PyArg_Parse, (PyObject *o, char *a, ...), (o, a, &va));
DefineExtFunc(gLib, PyObject*, Py_CompileStringFlags, (const char* s, const char* f, int st, PyCompilerFlags *fl), (s,f,st,fl));  
DefineExtFunc(gLib, PyObject*, PyComplex_FromDoubles, (double r, double i), (r,i));
DefineExtFunc(gLib, double, PyComplex_ImagAsDouble, (PyObject *o), (o));
DefineExtFunc(gLib, double, PyComplex_RealAsDouble, (PyObject *o), (o));
DefineExtFunc(gLib, int, PyDict_SetItemString, (PyObject *o, const char* k, PyObject *i), (o,k,i));
DefineExtFunc(gLib, PyObject*, PyDict_New, (void), ());
DefineExtFunc(gLib, PyObject*, PyDict_GetItem, (PyObject *o, PyObject *k), (o,k));
DefineExtFunc(gLib, PyObject*, PyDict_GetItemString, (PyObject *o, const char* k), (o,k));
DefineExtFunc(gLib, int, PyDict_Size, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyDict_Keys, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyDict_Values, (PyObject *o), (o));
DefineExtFuncVoid(gLib, PyEval_InitThreads, (void), ());
DefineExtFuncVoid(gLib, PyEval_AcquireLock, (void), ());
DefineExtFuncVoid(gLib, PyEval_ReleaseLock, (void), ());
//DefineExtFunc(gLib, void, PyEval_AcquireThread, (PyThreadState *t), (t));
//DefineExtFunc(gLib, void, PyEval_ReleaseThread, (PyThreadState *t), (t));
DefineExtFuncVoid(gLib, PyEval_RestoreThread, (PyThreadState *t), (t));
DefineExtFunc(gLib, PyThreadState*, PyEval_SaveThread, (void), ());
DefineExtFunc(gLib, PyObject*, PyEval_EvalCode, (PyCodeObject *c, PyObject *g, PyObject *l), (c,g,l));
DefineExtFunc(gLib, PyObject*, PyEval_GetBuiltins, (void), ());
DefineExtFunc(gLib, PyObject*, PyErr_Occurred, (void), ());
DefineExtFuncVoid(gLib, PyErr_Fetch, (PyObject **o1, PyObject **o2, PyObject **o3), (o1,o2,o3));
DefineExtFunc(gLib, double, PyFloat_AsDouble, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyFloat_FromDouble, (double d), (d));
DefineExtFunc(gLib, PyObject*, PyImport_AddModule, (const char *n), (n));
DefineExtFunc(gLib, PyObject*, PyInt_FromLong, (long l), (l));
DefineExtFunc(gLib, long, PyInt_AsLong, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyList_New, (int s), (s));
DefineExtFunc(gLib, PyObject*, PyList_GetItem, (PyObject *o, int i), (o,i));
DefineExtFunc(gLib, int, PyList_SetItem, (PyObject *l, int i, PyObject *o), (l,i,o));
DefineExtFunc(gLib, int, PyList_Size, (PyObject *l), (l));
DefineExtFunc(gLib, long, PyLong_AsLong, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyLong_FromDouble, (double d), (d));
DefineExtFunc(gLib, PyObject*, PyModule_GetDict, (PyObject *o), (o));
//DefineExtFunc(gLib, PyObject*, PyNumber_Long, (PyObject *o), (o));
//DefineExtFunc(gLib, PyObject*, PyNumber_Float, (PyObject *o), (o));
//DefineExtFunc(gLib, int, PyNumber_Check, (PyObject *o), (o)); 
//DefineExtFunc(gLib, int, PyObject_AsCharBuffer, (PyObject *o, const char **a, int *l), (o,a,l));
DefineExtFunc(gLib, PyObject*, PyObject_GetAttrString, (PyObject *o, const char *a), (o,a));
DefineExtFunc(gLib, PyObject*, PyObject_Str, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyObject_Type, (PyObject *o), (o));
DefineExtFunc(gLib, PyObject*, PyString_FromString, (const char* s), (s));
DefineExtFunc(gLib, PyObject*, PyString_FromStringAndSize, (const char* s, int i), (s,i));
DefineExtFunc(gLib, char*, PyString_AsString, (PyObject* o), (o));
DefineExtFunc(gLib, int, PyString_Size, (PyObject* o), (o));
DefineExtFunc(gLib, PyThreadState*, PyThreadState_Swap, (PyThreadState *t), (t));
DefineExtFunc(gLib, int, PyType_IsSubtype, (PyTypeObject *o1, PyTypeObject *o2), (o1, o2));

/* If the DLL is a debug version it exports this data variable symbol which
   can be used to check if any objects have been left allocated. If the DLL is
   non-DEBUG this symbol is not available and not exported. We do use this as a
   check to make sure that our module has the same "DEBUGness" as the DLL we are
   linking to. Otherwise if the DEBUGness would not match we fail catastrophally
   in our INCREF and DECREF macros.*/
long * _Py_RefTotal_Ptr = NULL;

PyObject * _Py_NoneStruct_Ptr = NULL;
PyIntObject * _Py_ZeroStruct_Ptr = NULL;
PyIntObject * _Py_TrueStruct_Ptr = NULL;
PyTypeObject * PyComplex_Type_Ptr = NULL;
PyTypeObject * PyFloat_Type_Ptr = NULL;
PyTypeObject * PyInt_Type_Ptr = NULL;
PyTypeObject * PyList_Type_Ptr = NULL;
PyTypeObject * PyLong_Type_Ptr = NULL;
PyTypeObject * PyString_Type_Ptr = NULL;
PyTypeObject * PyTuple_Type_Ptr = NULL;

#define kCfgPath 5

PStr gServerPath = (PStr)"\014PythonServer";

/* LabVIEW helper */
MgErr CfgGetDefault(int32 type, PStr token, UPtr value);
MgErr CfgWrite(int32 type, PStr token, UPtr value);
MgErr CfgRemove(int32 type, PStr token);

Bool32 PyLoadServer(void)
{
	if (!gLib)
	{
	  Path path = FEmptyPath(NULL);
	  LStrHandle lstr = NULL;
	  CStr name;

	  if (!CfgGetDefault(kCfgPath, gServerPath, (UPtr)path))
	  {
	    FPathToDSString(path, &lstr);
	    if (DSSetHandleSize(lstr, sizeof(int32) + LStrLen(*lstr) + 1))
	    {
	      DSDisposeHandle(lstr);
	      FDisposePath(path);
	      return lvsnFAILURE;
	    }
	    name = LStrBuf(*lstr);
	    name[LStrLen(*lstr)] = '\0';
	  }
	  else
	  {
	    name = (CStr)SHLIBpath;
	  }
	  FDisposePath(path);

	  if (!(gLib = LoadExternalLib(name)))
	    return FALSE;

	  if (lstr)
	    DSDisposeHandle(lstr);

	  LoadExtDataNoFail(gLib, long, _Py_RefTotal);
#ifdef Py_TRACE_REFS
	  if (!_Py_RefTotal_Ptr)
	  {
	    /* We are compiled for debug but the DLL is not! */
	    DbgPrintf("We need a DEBUG DLL to work without crash!");
	    goto errOut;
	  }
#else
	  if (_Py_RefTotal_Ptr)
	  {
	    /* The DLL is DEBUG compiled while we are not.
	       We can't continue because of differences in offsets to object members. */
	    DbgPrintf("We are not compiled to work with a DEBUG DLL!");
	    goto errOut;
	  }
#endif
	  LoadExtData(gLib, PyObject, _Py_NoneStruct);
	  LoadExtData(gLib, PyIntObject, _Py_ZeroStruct);
	  LoadExtData(gLib, PyIntObject, _Py_TrueStruct);
	  LoadExtData(gLib, PyTypeObject, PyComplex_Type);
	  LoadExtData(gLib, PyTypeObject, PyFloat_Type);
	  LoadExtData(gLib, PyTypeObject, PyInt_Type);
	  LoadExtData(gLib, PyTypeObject, PyList_Type);
	  LoadExtData(gLib, PyTypeObject, PyLong_Type);
	  LoadExtData(gLib, PyTypeObject, PyString_Type);
	  LoadExtData(gLib, PyTypeObject, PyTuple_Type);
	  return TRUE;
	}
errOut:
	return PyUnloadServer();
}

Bool32 PyUnloadServer(void) {
	if (gLib)
	{
	  ClearExtFunc(Py_Initialize);
	  ClearExtFunc(Py_Finalize);
	  ClearExtFunc(Py_NewInterpreter);
	  ClearExtFunc(Py_EndInterpreter);
	  ClearExtFunc(Py_GetVersion);
//	  ClearExtFunc(PyArg_Parse);
	  ClearExtFunc(Py_CompileStringFlags);  
	  ClearExtFunc(PyComplex_FromDoubles);
	  ClearExtFunc(PyComplex_ImagAsDouble);
	  ClearExtFunc(PyComplex_RealAsDouble);
	  ClearExtFunc(PyDict_SetItemString);
	  ClearExtFunc(PyDict_New);
	  ClearExtFunc(PyDict_GetItem);
	  ClearExtFunc(PyDict_GetItemString);
	  ClearExtFunc(PyDict_Size);
	  ClearExtFunc(PyDict_Keys);
	  ClearExtFunc(PyDict_Values);
	  ClearExtFunc(PyEval_InitThreads);
	  ClearExtFunc(PyEval_AcquireLock);
	  ClearExtFunc(PyEval_ReleaseLock);
//	  ClearExtFunc(PyEval_AcquireThread);
//	  ClearExtFunc(PyEval_ReleaseThread);
	  ClearExtFunc(PyEval_RestoreThread);
	  ClearExtFunc(PyEval_SaveThread);
	  ClearExtFunc(PyEval_EvalCode);
	  ClearExtFunc(PyEval_GetBuiltins);
	  ClearExtFunc(PyErr_Occurred);
	  ClearExtFunc(PyErr_Fetch);
	  ClearExtFunc(PyFloat_AsDouble);
	  ClearExtFunc(PyFloat_FromDouble);
	  ClearExtFunc(PyImport_AddModule);
	  ClearExtFunc(PyInt_FromLong);
	  ClearExtFunc(PyInt_AsLong);
	  ClearExtFunc(PyList_New);
	  ClearExtFunc(PyList_GetItem);
	  ClearExtFunc(PyList_SetItem);
	  ClearExtFunc(PyList_Size);
	  ClearExtFunc(PyLong_AsLong);
	  ClearExtFunc(PyLong_FromDouble);
	  ClearExtFunc(PyModule_GetDict);
//	  ClearExtFunc(PyNumber_Long);
//	  ClearExtFunc(PyNumber_Float);
//	  ClearExtFunc(PyNumber_Check);
//	  ClearExtFunc(PyObject_AsCharBuffer);
	  ClearExtFunc(PyObject_GetAttrString);
	  ClearExtFunc(PyObject_Str);
	  ClearExtFunc(PyObject_Type);
	  ClearExtFunc(PyString_FromString);
	  ClearExtFunc(PyString_FromStringAndSize);
	  ClearExtFunc(PyString_AsString);
	  ClearExtFunc(PyString_Size);
	  ClearExtFunc(PyThreadState_Swap);
	  ClearExtFunc(PyType_IsSubtype);
	  FreeExternalLib(gLib);
	  gLib = NULL;
	}
	return FALSE;
}

Bool32 PyChangePath(CStr token, Path path) {
	if (!token || !*token)
	  /* empty string use default token */
	  token = gServerPath;

	if (!path || FIsEmptyPath(path))
	{
	  return (CfgRemove(kCfgPath, token) == noErr);
	}
	return (CfgWrite(kCfgPath, token, (UPtr)path) == noErr);
}
#endif //PY_NO_IMPORT_LIB
