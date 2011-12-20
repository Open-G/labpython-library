/*
 * LabVIEW Python Library Interface
 * --------------------------------
 *
 * Created by: Rolf Kalbermatter
 *
 * @(#) $Id: lvpython.h,v 1.17 2007/12/14 00:59:36 labviewer Exp $";
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
#ifndef LVPYTHON_H
#define LVPYTHON_H

#include "extcode.h"
#include "hosttype.h"
#include "lvtypedef.h"
#include "lvsnapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Remark this out if you want to create a normal Python extension linking with
   the standard static import library on windows or the python static library
   on other platforms. If you unremark this, the module pyimport.c will actually
   link in the python shared library dynamically, which must have been created
   of course.
*/
#define PY_NO_IMPORT_LIB

#ifdef PY_NO_IMPORT_LIB
/* This macro specifies that the function and data pointers are imported from a DLL,
   which in our case is also true but not in the standard way. We create our own
   import library customizing the referencing of the Python DLL. The Windows Python
   config header pyconfig.h declares all exported symbols as declspec(dllimport)
   if USE_DL_IMPORT is defined. This is not really necessary for the function symbols
   as they are referenced as pointers anyhow but it is absolutely necessary for the
   symbols such as Py_None, Py_Float and such which are in fact data structures.
   Without declspec(dllimport) the C compiler (Visual C, but others I'm sure do the
   same) will translate all the &Py_<Type) references in a direct dereferencing of the
   objects location, whereas the same statement for a symbol declared as
   declspec(dllimport) will result in a direct access as the imported symbol is
   expected to be a reference already, which is what GetProcAddress really returns.*/
 #if defined(_MSC_VER)
  #define MS_NO_COREDLL
  #pragma comment(linker, "/nodefaultlib:\"python25.lib\"")
  #pragma comment(linker, "/nodefaultlib:\"python25_d.lib\"")
 #endif
 /* #define USE_DL_IMPORT */
 #define PyObject_TypeCheckNew(value, type)		\
    PyObject_TypeCheck(value, type##_Ptr)
 #include "Python.h"

 #ifdef Py_REF_DEBUG
  #define PyObject_Incref(op) ((*_Py_RefTotal_Ptr)++, (op)->ob_refcnt++)
  /* under Py_REF_DEBUG: also log negative ref counts after Py_DECREF() !! */
  #define PyObject_Decref(op)							\
       if (--(*_Py_RefTotal_Ptr), 0 < (--((op)->ob_refcnt))) ;			\
       else if (0 == (op)->ob_refcnt) _Py_Dealloc( (PyObject*)(op));	\
       else (void)fprintf( stderr, "%s:%i negative ref count %i\n",	\
                           __FILE__, __LINE__, (op)->ob_refcnt)
 #else /* !Py_REF_DEBUG */
  #define PyObject_Incref(op)	Py_INCREF(op)
  #define PyObject_Decref(op)	Py_DECREF(op)
 #endif
 #define Py_NonePtr		_Py_NoneStruct_Ptr
 #define Py_ZeroPtr		_Py_ZeroStruct_Ptr
 #define Py_FalsePtr	_Py_ZeroStruct_Ptr
 #define Py_TruePtr		_Py_TrueStruct_Ptr
#else
 #define PyObject_TypeCheckNew(value, type)		\
    PyObject_TypeCheck(value, &type)

 #define PyObject_Incref(op)	Py_INCREF(op)
 #define PyObject_Decref(op)	Py_DECREF(op)

 #define Py_NonePtr Py_None
 #define Py_ZeroPtr Py_Zero
 #define Py_FalsePtr Py_False
 #define Py_TruePtr Py_True
#endif

#ifdef  PY_NO_IMPORT_LIB
Bool32	PyLoadServer(void);
Bool32	PyUnloadServer(void);
Bool32	PyChangePath(CStr token, Path path);
#else
/* Just no-ops as the OS takes care to import the correct symbols */ 
# define PyLoadServer() TRUE
# define PyUnloadServer() FALSE
# define PyChangePath(token, path) TRUE
#endif

lvsnAPI(Bool32) pysnSetServerPath(CStr token, Path path);
lvsnAPI(Bool32) pysnScriptVariables(lvsnInstance inst, LStrArrayHdl names, LStrArrayHdl types);
lvsnAPI(Bool32) pysnServerSupportTypes(LStrHandle version, LStrArrayHdl names);

#ifdef __cplusplus
}
#endif

#endif
