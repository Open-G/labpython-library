/*
 * LabVIEW Python Library Interface
 * --------------------------------
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
 *
 *
 * $Log: lvpython.c,v $
 * Revision 1.23  2010/06/27 23:35:58  labviewer
 * minor cosmetic changes
 *
 * Revision 1.22  2007/12/14 00:59:36  labviewer
 * Some fixes for GCC compilation
 *
 * Revision 1.21  2007/11/14 21:35:51  labviewer
 * Fixed loading the actual lvpython DLL from the script server DLL
 *
 * Revision 1.20  2007/11/02 11:41:25  labviewer
 * *** empty log message ***
 *
 * Revision 1.19  2006/10/25 10:43:02  labviewer
 * Various modifications
 *
 * Revision 1.18  2006/10/21 21:06:01  labviewer
 * Changed VI library to use a different version of the shared library
 *
 * Revision 1.17  2006/10/21 20:58:36  labviewer
 * Changed VI library to use a different version of the shared library
 *
 * Revision 1.16  2006/10/20 12:35:36  labviewer
 * Separated the LabVIEW math script functions into a separate file
 *
 * Revision 1.15  2006/10/19 20:57:52  labviewer
 * Added a link wrapper to the script DLL
 *
 * Revision 1.14  2004/10/21 00:07:35  labviewer
 * Fixed a few typos and removed superfluous white space
 *
 * Revision 1.13  2002/10/23 22:26:18  labviewer
 * small linux changes and try outs
 *
 * Revision 1.12  2002/10/21 09:31:56  labviewer
 * Fixed problem with returning matrices.
 *
 * Revision 1.11  2002/10/20 22:22:35  labviewer
 * Several changes to make compiling under Linux possible
 *
 * Revision 1.10  2002/10/07 22:12:58  labviewer
 * Fixed small problem with Complex Matrix type
 *
 * Revision 1.9  2002/10/07 21:55:59  labviewer
 * Some changes to compile under Linux
 *
 * Revision 1.8  2002/09/24 11:58:13  labviewer
 * Fixed problems with array data types
 *
 * Revision 1.7  2002/09/12 14:44:39  labviewer
 * Added some code to support vectors and arrays
 *
 * Revision 1.6  2002/09/11 06:33:06  labviewer
 * Changed uninitialize and initialize procedure when changing the Pyuthon DLL path.
 *
 * Revision 1.5  2002/09/06 21:31:46  labviewer
 * Changed comment style
 *
 * Revision 1.2  2002/09/04 rolfk
 *	- added pysnScriptVariable() function to inquire about available variables
 *	  in a session.
 *
 * Revision 1.1  2002/09/02 rolfk
 *	- added pysnServerSupportTypes() function for direct LabVIEW VI access
 *
 * Revision 1.0  2002/08/30 rolfk
 *	- cleaned up everything and removed useless script node interface functions
 *	- figured out how to get the Python headers behave correctly if importing
 *	  our Python functions ourself explicitedly
 *	- fixed a serious buffer overflow bug in PyObjectToLVStr:
 *	  PyObject returned strings are not always properly zero terminated when
 *	  the original object is not a string. So instead of StrCpy use StrNCpy to
 *	  explicitedly copy the correct amount of characters.
 */
static char *id="@(#) $Id: lvpython.c,v 1.23 2010/06/27 23:35:58 labviewer Exp $";

#define LVSNAPI_EXPORTS 1
#define COBJMACROS
#include "lvtypedef.h"
#include "lvpython.h"
#include "pytscript.h"

#if !DEBUG
 int DebugPrintf(CStr fmt, ...) {
  return 0;
 }
#endif

#include "Python.h"
#include "grammar.h"
#include "errcode.h"
#include "compile.h"
#include "eval.h"

#ifdef PY_NO_IMPORT_LIB
extern long * _Py_RefTotal_Ptr;
extern PyObject * _Py_NoneStruct_Ptr;
extern PyObject * _Py_FalseStruct_Ptr;
extern PyObject * _Py_TrueStruct_Ptr;
extern PyTypeObject * PyComplex_Type_Ptr;
extern PyTypeObject * PyFloat_Type_Ptr;
extern PyTypeObject * PyInt_Type_Ptr;
extern PyTypeObject * PyList_Type_Ptr;
extern PyTypeObject * PyLong_Type_Ptr;
extern PyTypeObject * PyString_Type_Ptr;
extern PyTypeObject * PyTuple_Type_Ptr;
#endif

typedef struct tagSESS	*PySession;

typedef struct tagSESS {
	int32     magic;
	PySession next;
	CStr      text;
	PyObject *co;
	PyObject *dl;
	PyThreadState *tstate;
	WindowPtr pWnd;
} PySessionRec;

#define kPyMagic	123456

static PySession	gHead = NULL;

static PyThreadState *PyOpenHost(Bool32 reinit);
static Bool32 PyCloseHost(Bool32 full);
static Bool32 PyRestoreSession(PySession session);
static PySession PyCheckSession(lvsnInstance sess);
static Bool32 PyReleaseSession(PySession session);
static CStr PyObject_LVTypeString(PyObject *value);

static Bool32 PyCleanSession(PySession session, Bool32 full);
static Bool32 PyErrorInfo(int32 *pErrStart, int32 *pErrEnd, CStr lpErrText);
static PyObject *PyCreateData(int16 *tdp, int32 off, UPtr data);
static Bool32 PyConvertData(PyObject *value, int16 *tdp, int32 off, UPtr data);

static MgErr PyObject_ToLVNumeric(PyObject *value, int16 *tdp, int32 off, UPtr data);
static MgErr PyObject_ToLVArray(PyObject *value, int16 *tdp, int32 off, UPtr data);
static MgErr PyObject_ToLVString(PyObject *value, int16 *tdp, int32 off, UPtr data);

static MgErr PyArray_ToLVData(PyObject *value, int16 *tdp, int32 off, UPtr data);
static MgErr PyString_ToLVData(PyObject *value, int16 *tdp, int32 off, UPtr data);

static PyObject *PyBool_FromLVData(UPtr data);
static PyObject *PyString_FromLVData(UPtr data);
static PyObject *PyArray_FromLVData(int16 *tdp, int32 off, UPtr data);

static int16 PyObject_LVType(PyObject *value, int32 *sub);

static PyThreadState *gtstate = NULL;
/********************************************************************************/
/*																				*/
/* Shared library entry point													*/
/*																				*/
/* This is fully platform dependant. Other platforms will need a differnt		*/
/* implemenation.					 											*/
/*																				*/
/********************************************************************************/
#if MSWin
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
    {
	  case DLL_PROCESS_DETACH:
        PyCloseHost(TRUE);
    }
    return TRUE;
}
#elif Unix

#elif Mac

#endif

/********************************************************************************/
/*																				*/
/* Functions implementation														*/
/*																				*/
/********************************************************************************/

/********************************************************************************/
/*																				*/
/* Bool32 lvsnServerSupportType(int16*** typedefs, CStr **names, int32 *num)	*/
/*																				*/
/* Returns a list of supported data types and their names.					 	*/
/*																				*/
/*	typedefs	is an array of pointers to 16 bit LabVIEW typedefs vectors		*/
/*	names		is an array of pointers to C strings							*/
/*	num			is filled with the number of types refered to in the first two	*/
/*				arrays															*/
/*																				*/
/* Return lvsnSUCESS if successful, lvsnFAILURE otherwise 						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
static int16 varTypeBool[] = {0x4, boolCode};
static int16 varTypeInt32[] = {0x4, iL};
static int16 varTypeFloat64[] = {0x4, fD};
static int16 varTypeComplex64[] = {0x4, cD};
static int16 varTypeString[] = {0x8, stringCode, -1, -1};
static int16 varTypeStringVect[] = {0x12, arrayCode, 0x1, -1, -1, 0x8, stringCode, -1, -1};
static int16 varTypeOccurrence[] = {0x6, refNumCode, 0x4};
static int16 varTypeVIReference[] = {0xC, refNumCode, 0x8, 0x2, 0x4, 0x0};
static int16 varTypeInt32Vect[] = {0xE, arrayCode, 0x1, -1, -1, 0x4, iL};
static int16 varTypeFloat64Vect[] = {0xE, arrayCode, 0x1, -1, -1, 0x4, fD};
static int16 varTypeComplex64Vect[] = {0xE, arrayCode, 0x1, -1, -1, 0x4, cD};
static int16 varTypeInt32Matx[] = {0x12, arrayCode, 0x2, -1, -1, -1, -1, 0x4, iL};
static int16 varTypeFloat64Matx[] = {0x12, arrayCode, 0x2, -1, -1, -1, -1, 0x4, fD};
static int16 varTypeComplex64Matx[] = {0x12, arrayCode, 0x2, -1, -1, -1, -1, 0x4, cD};

int16 *varTypes[] = {varTypeBool, varTypeInt32, varTypeFloat64, varTypeComplex64,
					 varTypeString, varTypeStringVect,
					 varTypeInt32Vect, varTypeFloat64Vect, varTypeComplex64Vect,
					 varTypeInt32Matx, varTypeFloat64Matx, varTypeComplex64Matx,
					 varTypeOccurrence, varTypeVIReference};

CStr  varNames[] = {(CStr)"Boolean", (CStr)"Integer", (CStr)"Float", (CStr)"Complex",
					(CStr)"String", (CStr)"String Vector",
					(CStr)"Integer Vector", (CStr)"Float Vector", (CStr)"Complex Vector",
					(CStr)"Integer Matrix", (CStr)"Float Matrix", (CStr)"Complex Matrix",
					(CStr)"Occurrence", (CStr)"VIReference"};

typedef enum {
	kTDBoolean,
	kTDInteger,
	kTDFloat,
	kTDComplex,
	kTDString,
	kTDStringVect,
	kTDIntegerVect,
	kTDFloatVect,
	kTDComplexVect,
	kTDIntegerMatx,
	kTDFloatMatx,
	kTDComplexMatx,
	kTDOccurrence,
	kTDVIReference,
	/* We do not yet have any infrastructure in place
	   to support the other types properly yet. */
	kNumTypes = kTDComplexMatx + 1
} kTypes;

lvsnAPI(Bool32) lvsnServerSupportTypes(int16*** typedefs, CStr **names, int32 *num) {
	if (!typedefs || !names || !num)
	  return lvsnFAILURE;
	*typedefs = varTypes;
	*names = varNames;
	*num = kNumTypes;
	return lvsnSUCCESS;
}

/********************************************************************************/
/*																				*/
/* Bool32 pysnServerSupportType(LStrHandle version, LStrArrayHdl names)			*/
/*																				*/
/* Returns the version information of the linked to Python shared library and	*/
/* a list of supported data types and their names.							 	*/
/*																				*/
/*	version		is a LabVIEW string handle to fill in the version information	*/
/*	names		is an array of LabVIEW string handles to fill in the supported	*/
/*				type names														*/
/*																				*/
/* Return lvsnSUCESS if successful, lvsnFAILURE otherwise 						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) pysnServerSupportTypes(LStrHandle version, LStrArrayHdl names)
{
	int32 i, num, len;
	LStrHandle *pstr;

	DebugPrintf((CStr)"Before Debugger");
	Debugger();
	DebugPrintf((CStr)"After Debugger");
	if (!names)
	{
	  return lvsnFAILURE;
	}
	DebugPrintf((CStr)"Before PyOpenHost");
	if (PyOpenHost(FALSE))
	  LStrPrintf(version, (CStr)"Python %s", (CStr)Py_GetVersion());

	num = kNumTypes;
	if (SetArraySize(&varTypes[kTDString], 0, 1L, (UHandle*)&names, num))
	{
	  return lvsnFAILURE;
	}

	for (i = 0, pstr = (*names)->elm; i < kNumTypes; i++, pstr++)
	{
	  len = StrLen(varNames[i]);
	  if (NumericArrayResize(uB, 1L, (UHandle*)pstr, len))
	  {
	    return lvsnFAILURE;
	  }
	  StrNCpy(LStrBuf(**pstr), varNames[i], len);
	  LStrLen(**pstr) = len;
	}
	(*names)->len = kNumTypes;
	return lvsnSUCCESS;
}

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
lvsnAPI(lvsnInstance) lvsnInitNew(uInt32 version) {
	Debugger();
	if (version == 1)
	{
		if (PyOpenHost(FALSE)) {
			PySession session = (PySession)DSNewPClr(sizeof(PySessionRec));
			if (session) {
				PyEval_AcquireLock();
				session->magic = kPyMagic;
				session->tstate = Py_NewInterpreter();
				session->next = (PySession)gHead;
				gHead = session;
				PyReleaseSession(session);
			}
			return (lvsnInstance)session;
		}
	}
	return NULL;
}

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
lvsnAPI(void) lvsnCleanup(lvsnInstance inst) {
	PySession session = PyCheckSession(inst);
	if (session) {
		Debugger();
		PyCleanSession(session, TRUE);
		/* Current thread is already zapped */
		PyEval_ReleaseLock();
	}
}

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
lvsnAPI(Bool32) lvsnSetScript(lvsnInstance inst, CStr lpScriptText) {
	PySession session = PyCheckSession(inst);
	if (session) {
		int32 len = StrLen(lpScriptText);

		if (session->text)
			DSDisposePtr(session->text);
		session->text = (CStr)DSNewPClr(len + 1);
		MoveBlock(lpScriptText, session->text, len);
		if (session->co) {
			PyObject_Decref(session->co);
			session->co = NULL;
		}
		/* We need a local variable dictionary anyhow at one point */
		if (!session->dl) {
			session->dl = PyDict_New();
		}
		return PyReleaseSession(session);
	}
	return lvsnFAILURE;
}

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
lvsnAPI(int32) lvsnGetScript(lvsnInstance inst, CStr lpScriptText, int32 slen) {
	PySession session = PyCheckSession(inst);
	if (session) {
		int32 len = 0;
		if (session->text) {
			len = StrLen(session->text);
			if (lpScriptText) {
				if (len <= slen) {
					StrCpy(lpScriptText, session->text);
					len = 1;
				}
				else {
					len = -1;
				}
			}
		}
		PyReleaseSession(session);
		return len;
	}
	return 0;
}

/********************************************************************************/
/*																				*/
/* Bool32 lvsnSetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td)*/
/*																				*/
/* Set the data for the variable.												*/
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
lvsnAPI(Bool32) lvsnSetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *tdp) {
	Bool32 success = lvsnFAILURE;
	PySession session = PyCheckSession(inst);
	if (session) {
		Debugger();
		if (!session->dl) {
		/* If no local variable dictionary create new one */
			session->dl = PyDict_New();
		}
		if (session->dl) {
			PyObject *value = PyCreateData(tdp, 0, data);
			if (value) {
				/* Add the input variable to the local variable dictionary */
				PyDict_SetItemString(session->dl, (char*)name, value);
				PyObject_Decref(value);
				/* Return success/failure */
				success = PyErr_Occurred() ? lvsnFAILURE : lvsnSUCCESS;
			}
		}
		PyReleaseSession(session);
	}
	return success;
}

/********************************************************************************/
/*																				*/
/* Bool32 lvsnGetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *td)*/
/*																				*/
/* Get the data for the variable.												*/
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
lvsnAPI(Bool32) lvsnGetLabVIEWData(lvsnInstance inst, CStr name, UPtr data, int16 *tdp) {
	Bool32 success = lvsnFAILURE;
	PySession session = PyCheckSession(inst);
	if (session) {
		Debugger();
		if (session->dl) {
			PyObject *value;

			/* Retrieve variable value */
			if ((value = PyDict_GetItemString(session->dl, (char*)name))) {
				/* Value is borrowed reference from dict */
				success = PyConvertData(value, tdp, 0, data);
			}
		}
		PyReleaseSession(session);
	}
	return success;
}

/********************************************************************************/
/*																				*/
/* Bool32 lvsnCompile(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText) (optional) */
/*																				*/
/* Compile the script associated with the instance and return errors if any		*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnCompile(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText) {
	Bool32 success = lvsnFAILURE;
	PySession session = PyCheckSession(inst);
	if (session) {
		if (!session->co) {

			/* Compile the script */
			session->co = Py_CompileStringFlags((char*)session->text, "<LabVIEW>", Py_file_input, 0);

			/* Extract eventual error information if any */
			success = PyErrorInfo(eStart, eEnd, eText);
		}
		PyReleaseSession(session);
	}
	return success;
}

/********************************************************************************/
/*																				*/
/* Bool32 lvsnExecute(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText)*/
/*																				*/
/* Execute the script associated with the session and return errors if any		*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise.						*/
/* The LabVIEW script node execution can call this function from any thread.	*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) lvsnExecute(lvsnInstance inst, int32 *eStart, int32 *eEnd, CStr eText) {
	Bool32 success = lvsnFAILURE;
	PySession session = PyCheckSession(inst);
	if (session) {
		if (!session->co) {
			/* Compile the script */
			session->co = Py_CompileStringFlags((char*)session->text, "<LabVIEW>", Py_file_input, 0);
		}
		if (session->co) {
			PyObject *v, *m, *dg;

			/* Extract the global dictionary object */
			if (!(m = PyImport_AddModule("__main__")))
				goto errOut;
			if (!(dg = PyModule_GetDict(m))) {
				goto errOut;
			}
			PyObject_Incref(dg);
			if (PyDict_GetItemString(dg, "__builtins__") == NULL) {
				if (PyDict_SetItemString(dg, "__builtins__", PyEval_GetBuiltins()) != 0) {
					PyObject_Decref(dg);
					goto errOut;
				}
			}
			/* Make sure we have a valid local dictionary to eventually retrieve output variables
			   later on. This might not be so if either no local input variables have been set for
			   the script (and the script then better doesn't need any) or if the server path has
			   been changed while a session was up, which might result in execution errors because
			   of not available input variables, but I figured this is better than risking a crash! */
			if (!session->dl) {
				if (!(session->dl = PyDict_New())) {
					PyObject_Decref(dg);
					goto errOut;
				}
			}

			/* Execute the script, passing in the LabVIEW arguments as local variables */
			if ((v = PyEval_EvalCode((PyCodeObject *)session->co, dg, session->dl))) {
				PyObject_Decref(v);
			}
			PyObject_Decref(dg);
		}
		/* Extract eventual error information if any */
		success = PyErrorInfo(eStart, eEnd, eText);
errOut:
		PyReleaseSession(session);
	}
	return success;
}

/********************************************************************************/
/*																				*/
/* Bool32 pysnScriptVariables(lvsnInstance inst, LStrArrayHdl names, LStrArrayHdl types) */
/*																				*/
/* Returns a list of variable names and their types currently available in the	*/
/* instance																		*/
/*																				*/
/*	inst		is the reference indicating the session							*/
/*	names		is an array of LabVIEW string handles to fill in the available	*/
/*				variable names													*/
/*	types		is an array of LabVIEW string handles to fill in the types of	*/
/*				available variable names										*/
/*																				*/
/* Return lvsnSUCESS if successful, lvsnFAILURE otherwise 						*/
/* The LabVIEW script node execution will call this functions from the UI		*/
/* thread.																		*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) pysnScriptVariables(lvsnInstance inst, LStrArrayHdl names, LStrArrayHdl types)
{
	PySession session = PyCheckSession(inst);
	if (session)
	{
	  /* Don't try to do anything if not both names and types are valid */
	  if (names && types)
	  {
	    if (session->dl)
	    {
	      LStrHandle *pstr, *tstr;
	      int32 i, num, len;
	      PyObject *kl, *key, *val;
	      CStr str;

	      num = (int32)PyDict_Size(session->dl);
	      if (SetArraySize(&varTypes[kTDString], 0, 1L, (UHandle*)&names, num))
	      {
	        goto errOut;
	      }
	      if (SetArraySize(&varTypes[kTDString], 0, 1L, (UHandle*)&types, num))
	      {
	        goto errOut;
	      }

	      kl = PyDict_Keys(session->dl);
	      for (i = 0, pstr = (*names)->elm, tstr = (*types)->elm;
	           i < num; i++, pstr++, tstr++)
	      {
	        if ((key = PyList_GetItem(kl, i)))
	        {
	          /* borrowed reference to the key item */
	          if (PyObject_TypeCheckNew(key, PyString_Type))
	          {
	            str = (CStr)PyString_AsString(key);
	            len = StrLen(str);
	            if (NumericArrayResize(uB, 1L, (UHandle*)pstr, len))
	            {
	              goto errOut;
	            }
	            StrNCpy(LStrBuf(**pstr), str, len);
	            LStrLen(**pstr) = len;
	            if ((val = PyDict_GetItem(session->dl, key)))
	            {
	              /* borrowed reference to the value item */
	              str = PyObject_LVTypeString(val);
	              len = StrLen(str);
	              if (NumericArrayResize(uB, 1L, (UHandle*)tstr, len))
	              {
	                goto errOut;
	              }
	              StrNCpy(LStrBuf(**tstr), str, len);
	              LStrLen(**tstr) = len;
	            }
	            else if (*tstr)
	            {
	              LStrLen(**tstr) = 0;
	            }
	          }
	          else if (*pstr)
	          {
	            LStrLen(**pstr) = 0;
	          }
	        }
	      }
	      PyObject_Decref(kl);
	      (*names)->len = num;
	      (*types)->len = num;
	    }
	    else
	    {
	      (*names)->len = 0;
	      (*types)->len = 0;
	    }
	  }
	  return PyReleaseSession(session);
	}
errOut:
	return lvsnFAILURE;
}


/********************************************************************************/
/*																				*/
/* Bool32 pysnSetServerPath(Path path)											*/
/*																				*/
/* Sets the path to the DLL to use for the Python core. Care must be taken that	*/
/* no script is open either through an open VI with a LabVIEW Python script		*/
/* node or explicit VI library calls to this DLL at the time this function is	*/
/* called, as it will otherwise cause bad things to your LabVIEW environment.	*/
/*																				*/
/* Return lvsnSUCCESS if successful, lvsnFAILURE otherwise						*/
/*																				*/
/********************************************************************************/
lvsnAPI(Bool32) pysnSetServerPath(CStr token, Path path)
{
#ifdef PY_NO_IMPORT_LIB
	/* If we are not fully dynamically linked we can't change anything */
	Bool32 success;
	success =  PyChangePath(token, path); 
	if (success && PyCloseHost(FALSE))
	{
	  success = lvsnFAILURE;
	  if (PyOpenHost(TRUE))
	  {
	    PySession pNext;
	    for (pNext = gHead; pNext; pNext = pNext->next)
	    {
	      PyRestoreSession(pNext);
	    }
	    success = lvsnSUCCESS;
	  }
	}
	return success;
#else
	return lvsnFAILURE;
#endif
}

/********************************************************************************/
/********************************************************************************/
/*																				*/
/* Locale function implementations												*/
/*																				*/
/********************************************************************************/
/********************************************************************************/
PyThreadState *PyOpenHost(Bool32 reinit) {
	if (!gtstate && PyLoadServer()) {
		Py_Initialize(); /* Initialize the interpreter */
		/* Create (and acquire) the interpreter lock. Well seems like a cosmetic bug
		   here. If the DLL import library is linked in, the DLL and with it the
		   interpreter lock is never zeroed out (released) and PyEval_InitThreads()
		   does nothing at all!! In fact calling Py_Finalize() */
		if (reinit) {
#ifdef PY_NO_IMPORT_LIB
			PyEval_InitThreads();
#else
			PyEval_AcquireLock();
#endif
		}
		else
			PyEval_InitThreads();

		/* Release the thread state and interpreter lock */
		gtstate = PyEval_SaveThread(); 
	}
	return gtstate;
}

Bool32 PyCloseHost(Bool32 full) {
	if (gtstate) {
		PySession pSess;
		PyEval_RestoreThread(gtstate);
		for (pSess = gHead; pSess; pSess = pSess->next) {
			/* Assign the session thread state to the global state. This
			   is necessary for the release of the interpreter state when
			   cleaning up. */
			PyThreadState_Swap(pSess->tstate);
			PyCleanSession(pSess, full);
		}
#if 0
		/* Assign the main thread state to the global state */
		PyThreadState_Swap(gtstate);
		/* Release the main thread state and global lock */
		PyEval_ReleaseLock();
#endif
		/* Shutdown the Python Server */
		Py_Finalize();
		/* Unload DLL and linkage */
		PyUnloadServer();
		gtstate = NULL;
		return lvsnSUCCESS;
	}
	return lvsnFAILURE;
}

PySession PyCheckSession(lvsnInstance inst) {
	if (inst) {
		PySession session = (PySession)inst;
		if (session->magic == kPyMagic) {
			if (session->tstate)
				PyEval_RestoreThread(session->tstate);
			else
				PyEval_AcquireLock();
			return session;
		}
	}
	return NULL;
}

Bool32 PyReleaseSession(PySession session) {
	if (session->tstate)
		PyEval_SaveThread();
	else
		PyEval_ReleaseLock();
	return TRUE;
}

static Bool32 PyCleanSession(PySession session, Bool32 full) {
	if (session) {
		Debugger();
		if (session->dl) {
			PyObject_Decref(session->dl);
			session->dl = NULL;
		}
		if (session->co) {
			PyObject_Decref(session->co);
			session->co = NULL;
		}
		if (session->tstate) {
			Py_EndInterpreter(session->tstate);
			session->tstate = NULL;
		}
		if (full) {
			PySession *pNext;
			session->magic = 0;
			/* Search the session in our list and remove it */
			for (pNext = &gHead; *pNext; pNext = &(*pNext)->next) {
				if (*pNext == session) {
					*pNext = session->next;
					break;
				}
			}
			if (session->text)
				DSDisposePtr(session->text);
			DSDisposePtr(session);
		}
	}
	return lvsnSUCCESS;
}

Bool32 PyRestoreSession(PySession session) {
	PyEval_AcquireLock();
	session->tstate = Py_NewInterpreter();
	PyReleaseSession(session);
	return lvsnSUCCESS;
}

static PyObject *PyCreateData(int16 *tdp, int32 off, UPtr data) {
	switch (TDType(tdp, off)) {
		case boolCode:
			return PyBool_FromLVData(data);
		case iL: 
			return PyInt_FromLong(*(int32*)data);
		case fD:
			return PyFloat_FromDouble(*(float64*)data);
		case cD:
			return PyComplex_FromDoubles(((cmplx128*)data)->re, ((cmplx128*)data)->im);
		case stringCode:
			return PyString_FromLVData(data);
		case arrayCode:
			return PyArray_FromLVData(tdp, off, data);
		case refNumCode: {
			int16 subType = RefNumTDSubType(tdp, off);
			/* not implemented as we have no object type support for this yet */
			return NULL;
		}
	}
	return NULL;
}

static Bool32 PyConvertData(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	uInt8 lvType = TDType(tdp, off);

	if (IsNumeric(lvType) || lvType == boolCode) {
		return (PyObject_ToLVNumeric(value, tdp, off, data) == noErr) ? TRUE : FALSE;
	}
	else switch (lvType) {
		case stringCode:
		case pathCode:
			return (PyObject_ToLVString(value, tdp, off, data) == noErr) ? TRUE : FALSE;
		case arrayCode:
			return (PyObject_ToLVArray(value, tdp, off, data) == noErr) ? TRUE : FALSE;
		case refNumCode: {
			int16 subType = RefNumTDSubType(tdp, off);
			/* Not implemented as we have no object type support for this */
			break;
		}
	}
	return lvsnFAILURE;
}

/* This function was snooped and adapted from parse_syntax_error() in pythonrun.c  */
static Bool32 PyErrorInfo(int32 *eStart, int32 *eEnd, CStr eText) {
	PyObject *type, *mess, *traceback;
	LStrHandle lstr = NULL;
	MgErr err;
	CStr p = eText;
	int rest = kMaxErrStringLength - 1;

	*eStart = -1;

	if (!PyErr_Occurred())
		return lvsnSUCCESS;

	PyErr_Fetch(&type, &mess, &traceback);
	if (traceback) {
		PyObject_Decref(traceback);
	}

	if (!(err = PyObject_ToLVString(type, varTypeString, 0, (UPtr)&lstr))) {
		int32 len;
		/* Format string as follows "<exception type>, <message>" */
		len = Min(LStrLen(*lstr), rest);
		StrNCpy(p, LStrBuf(*lstr), len);
		DSDisposeHandle(lstr);
		p += len;
		rest -= len;
		StrNCpy(p, (CStr)", ", rest);
		p += 2;
		rest -= 2;
	}

	if (PyObject_TypeCheckNew(mess, PyString_Type)) {
		CStr t = (CStr)PyString_AsString(mess);
		rest = Min((int32)PyString_Size(mess), rest);
		StrNCpy(p, t, rest);
	}
/*	else if (PyTuple_Check(mess)) {
		// old style errors
		PyObject *message;
		int ret, pos;
		char *filename;

		ret = PyArg_Parse(mess, "(O(ziiz))", &message, &filename,
				   &eStart, &pos, &t);
		PyObject_Decref(message);
		StrNCpy(p, t, rest);
	} */
	else {
		PyObject *v;
		/* new style errors.  `mess' is an instance */

		if ((v = PyObject_GetAttrString(mess, "offset"))) {
			if (Py_isValidObject(v)) {
				long hold = PyInt_AsLong(v);
				if (hold >= 0 && !PyErr_Occurred())
					*eStart = (int32)hold;	
			}
			PyObject_Decref(v);
		}
		if ((v = PyObject_GetAttrString(mess, "text"))) {
			if (Py_isValidObject(v)) {
				rest = Min((int32)PyString_Size(v), rest);
				StrNCpy(eText, (CStr)PyString_AsString(v), rest);
			}
			PyObject_Decref(v);
		}
	}
	PyObject_Decref(type);
	PyObject_Decref(mess);
	*eEnd = *eStart + 1; 
	return lvsnFAILURE;
}

static PyObject *PyBool_FromLVData(UPtr data) {
	PyObject *b = (*(int8*)data) ? (PyObject*)Py_TruePtr : (PyObject*)Py_FalsePtr;
	PyObject_Incref(b);
	return b;
}

static MgErr PyString_ToLVData(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	switch (TDType(tdp, off)) {
/*		case iL:
			*(int32*)data = PyString_AsLong(value);
			break;
		case fD:
			*(float64*)data = PyString_AsDouble(value);
			break;
		case cD:
			*(cmplx128*)data = PyString_AsCComplex(value);
			break; */
		default:
			return bogusError;
	}
	return noErr;
}

static PyObject *PyString_FromLVData(UPtr data) {
	if (*data) { /* LabVIEW can use NULL handles for empty data */
		LStrHandle str = *(LStrHandle *)data;
		return PyString_FromStringAndSize((char*)LStrBuf(*str), LStrLen(*str));
	}
	return PyString_FromString("");
}

typedef struct {
	int32 size;
	uChar buf[1];
} TD1DArr, **TD1DArrHdl;

typedef struct {
	int32 size1;
	int32 size2;
	uChar buf[1];
} TD2DArr, **TD2DArrHdl;

static MgErr PyArray_Attributes(PyObject *value, int32 *elms, int32 *dims) {
	if (PyObject_TypeCheckNew(value, PyList_Type)) {
		*elms = (int32)PyList_Size(value);
		*dims = 1;
		if (*elms) {
			PyObject *item = PyList_GetItem(value, 0);
			/* item is borrowed reference */

			if (PyObject_TypeCheckNew(item, PyList_Type)) {
				*elms *= (int32)PyList_Size(item);
				if (*elms) {
					PyObject *o = PyList_GetItem(item, 0);
					if (o && PyObject_TypeCheckNew(o, PyList_Type)) {
						/* we don't support more than 2 dimensions */
						return mgArgErr;
					}
				}
				*dims = 2;
			}
		}
	}
	else {
		*dims = 0;
		*elms = 1;
	}
	return noErr;
}

static MgErr PyArray_Convert(PyObject *list, UPtr *data, int16 *tdp, int32 off, int32 dsize, int32 *dims) {
	int32 idx;

	for (idx = 0; idx < *dims; idx++) {
		PyObject *o = PyList_GetItem(list, idx);
		if (o) {
			if (PyObject_TypeCheckNew(o, PyList_Type)) {
				MgErr err;
				if ((err = PyArray_Convert(o, data, tdp, off, dsize, dims + 1))) {
					return err;
				}
			}
			else {
				if (PyConvertData(o, tdp, off, *data) == lvsnFAILURE)
					return bogusError;
				*data += dsize;
			}
		}
	}
	return noErr;
}

/* Be aware that dimensionality may not exactly match. We should allow to store a
   lower dim array into a higher dim array but most probably not vice versa. */
static MgErr PyArray_ToLVData(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	MgErr err;
	int32 elms, dims, lvDims = ArrayTDDims(tdp, off);

	if (!(err = PyArray_Attributes(value, &elms, &dims))) {
		if (lvDims >= dims) {
			int32 elmoff = ArrEltTDOffset(tdp, off);
			if (!(err = SetArraySize(&tdp, elmoff, lvDims, (UHandle*)data, elms)) && *data) {
				int32 i, *sp;
				PyObject *list = value;

				sp = **(int32***)data;

				for (i = 0; i < lvDims; i++, sp++) {
					if (i < dims) {
						*sp = (int32)PyList_Size(list);
						list = PyList_GetItem(list, 0);
					}
					else {
						*sp = 1;
					}
				}
				if ((err = PyArray_Convert(value, (UPtr*)&sp, tdp, elmoff, DataSize(TDPtr(tdp, elmoff)), **(int32***)data))) {
					return err;
				}
			}
		}
		else {
			err = mgNotSupported;
		}
	}
	return err;
}

static PyObject *PyArray_FromLVData(int16 *tdp, int32 off, UPtr data) {
	PyObject *o = NULL;
	int32 dims = ArrayTDDims(tdp, off);

	if (*data) { /* LabVIEW can use NULL handles for empty data */
		UPtr ptr;
		int32 dataSize, elmoff, i, j, size1, size2;

		elmoff = ArrEltTDOffset(tdp, off);
		dataSize = DataSize(TDPtr(tdp, elmoff));

		switch (dims) {
			case 1:
				/* ALERT: More efficient would be to use the array object in
						  arraymodule.c but it has no exported C interface!
						  There seems to be a "numarray" module which should
						  help doing this in a decent way and a new development
						  called "numpy" which is supposed to replace numarray
						  one day. */
				ptr = (**(TD1DArrHdl*)data)->buf;
				size1 = (**(TD1DArrHdl*)data)->size;

				o = PyList_New(size1);
				for (i = 0; i < size1; i++, ptr += dataSize) {
					PyList_SetItem(o, i, PyCreateData(tdp, elmoff, ptr));
				}
				break;
			case 2:
				/* ALERT: Implementing the 2D array as an array of arrays is
						  although possible, probably not a very straight forward
						  and sensible approach. */
				ptr = (**(TD2DArrHdl*)data)->buf;
				size1 = (**(TD2DArrHdl*)data)->size1;
				size2 = (**(TD2DArrHdl*)data)->size2;
				o = PyList_New(size1);
				for (i = 0; i < size1; i++) {
					PyObject *o2 = PyList_New(size2);
					for (j = 0; j < size2; j++, ptr += dataSize) {
						PyList_SetItem(o2, j, PyCreateData(tdp, elmoff, ptr));
					}
					PyList_SetItem(o, i, o2);
				}
				break;
		}
	}
	else {
		switch (dims) {
			case 1:
				o = PyList_New(0);
				break;
			case 2:
				o = PyList_New(1);
				if (o) {
					PyObject *o2 = PyList_New(0);
					PyList_SetItem(o2, 0, o);
					o = o2;
				}
				break;
		}
	}
	return o;
}

static MgErr PyObject_ToLVNumeric(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	uInt8 lvType = TDType(tdp, off);

	switch (PyObject_LVType(value, NULL)) {
		case iL: {
			int32 l = PyLong_AsLong(value);
			switch (lvType) {
				case iL:
					*(int32*)data = l;
					break;
				case fD:
					*(float64*)data = l;
					break;
				case cD:
					((cmplx128*)data)->re = l;
					((cmplx128*)data)->im = 0.0;
					break;
				case boolCode:
					*(int8*)data = (l != 0);
					break;
				default:
					return bogusError;
			}
			return noErr;
			}
		case fD: {
			float64 d = PyFloat_AsDouble(value);
			switch (lvType) {
				case iL:
					*(int32*)data = (int32)d;
					break;
				case fD:
					*(float64*)data = d;
					break;
				case cD:
					((cmplx128*)data)->re = d;
					((cmplx128*)data)->im = 0.0;
					break;
				case boolCode:
					*(int8*)data = (d != 0.0);
					break;
				default:
					return bogusError;
			}
			return noErr;
			}
		case cD: {
			cmplx128 v;
			v.re = PyComplex_RealAsDouble(value);
			v.im = PyComplex_ImagAsDouble(value);
			switch (lvType) {
				case iL:
					*(int32*)data = (int32)(hypot(v.re, v.im));
					break;
				case fD:
					*(float64*)data = hypot(v.re, v.im);
					break;
				case cD:
					*(cmplx128*)data = v;
					break;
				default:
					return bogusError;
			}
			return noErr;
			}
		case stringCode:
			return PyString_ToLVData(value, tdp, off, data);
		case arrayCode:
			return PyArray_ToLVData(value, tdp, off, data);
		default: {
			MgErr err = bogusError;
			PyObject *s = PyObject_Str(value);
			if (s)
				err = PyString_ToLVData(s, tdp, off, data);
			PyObject_Decref(s);
			return err;
		}
	}
	return bogusError;
}

/* Oh well this can get messy. First find out if the PyObject is a scalar and if so
   create a single element array of the desired data type. Otherwise convert the
   entire array into the desired LabVIEW type array. Any other object type is rejected
   to make things not overly complicated. */
static MgErr PyObject_ToLVArray(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	int16 pyType = PyObject_LVType(value, NULL);

	if (IsNumeric(pyType)) {
		MgErr err;
		int16 dims = ArrayTDDims(tdp, off);

		if (!(err = NumericArrayResize(TDType(tdp, off), dims, (UHandle*)data, 1))) {
			int32 i, *p = (int32*)(**(UHandle*)data);
			for (i = 0; i < dims; i++, p++) {
				/* Set the length for each dimension to 1 */
				*p = 1;
			}
			err = PyObject_ToLVNumeric(value, tdp, ArrEltTDOffset(tdp, off), (UPtr)p);
		}
		return err;
	}
	else if (pyType == arrayCode) {
		return PyArray_ToLVData(value, tdp, off, data);
	}
	return bogusError;
}

/* Convert the Python object into a string object and then format it into
   a LabVIEW string. */
static MgErr PyObject_ToLVString(PyObject *value, int16 *tdp, int32 off, UPtr data) {
	MgErr err = noErr;
	PyObject *s;
	int32 len;
	uInt8 lvType;

	if (!(s = PyObject_Str(value))) {
		return mgArgErr;
	}

	if ((len = (int32)PyString_Size(s))) {
		LStrHandle lstr = NULL;

		switch (lvType = TDType(tdp, off)) {
			case stringCode :
				if (*(UHandle *)data) {
					err = DSSetHSzClr(*(UHandle *)data, len + sizeof(int32));
					if (!err) {
						lstr = *(LStrHandle *)data;
					}
					break;
				}
				/* Fall through */
			case pathCode :
				if (!(lstr = (LStrHandle) DSNewHClr(len + sizeof(int32)))) {
					err = mFullErr;
				}
				break;
			default:
				err = mgArgErr;
				break;
		}
		if (!err) {
			StrNCpy(LStrBuf(*lstr), (CStr)PyString_AsString(s), len);

			switch (lvType) {
				case stringCode :
					LStrLen(*lstr) = len;
					if (!(*(LStrHandle*)data)) {
						*(LStrHandle *)data = lstr;
					}
					break;
				case pathCode :
					err = FTextToPath(LStrBuf(*lstr), len, (Path *)data);
					DSDisposeHandle(lstr);
					break;
				/* No other cases, we should have failed above already */
			}
		}
	}
	else if (*(UHandle *)data) {
		LStrLen(**(LStrHandle *)data) = len;
	}
	PyObject_Decref(s);
	return err;
}

static int16 PyObject_LVType(PyObject *value, int32 *sub) {
	if (PyObject_TypeCheckNew(value, PyLong_Type) ||
		PyObject_TypeCheckNew(value, PyInt_Type)) {
		return iL;
	}
	if (PyObject_TypeCheckNew(value, PyFloat_Type)) {
		return fD;
	}
	if (PyObject_TypeCheckNew(value, PyComplex_Type)) {
		return cD;
	}
	if (PyObject_TypeCheckNew(value, PyList_Type)) {
		return arrayCode;
	}
/*	if (PyObject_TypeCheckNew(value, PyArray_Type)) {
		return arrayCode;
	}
	if (PyObject_TypeCheckNew(value, PyRefnum_Type)) {
		return refNumCode;
	} */
	if (PyObject_TypeCheckNew(value, PyString_Type)) {
		return stringCode;
	}
	return 0;
}

static CStr PyObject_LVTypeString(PyObject *value) {
	int32 sub;
	int16 type = PyObject_LVType(value, &sub);
	if (type) {
		int32 i;
		int16 *tdp;

		for (i = 0; i < kNumTypes; i++) {
			tdp = varTypes[i];
			if (TDType(tdp, 0) == type) {
				if (type == refNumCode) {
					if (RefNumTDSubType(tdp, 0) != sub)
						continue;
				}
				else if (type == arrayCode) {
					if (ArrayTDDims(tdp, 0) != sub)
						continue;
				}
				return varNames[i];
			}
		}
	}
	return ((CStr)"Unknown Type");
}
