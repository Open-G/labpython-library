

#ifndef DYNLIB_H
#define DYNLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extcode.h"
#include "hosttype.h"

#if Unix || MacOSX
#define DLInterface 1
#elif Mac
#define FragInterface 1
#endif

#if DLInterface
# include <dlfcn.h>
#elif FragInterface
# include <CodeFragments.h>
#endif

#if MSWin
#define IMPORT_DECORATION _imp__
typedef HMODULE ExtLib;
#elif DLInterface
#define IMPORT_DECORATION _
typedef void* ExtLib;
#else /* Mac */
#error Define the platform specific external library access methods and types
#endif

typedef int32 (*ProcPtr) (int32);

ExtLib LoadExternalLib(ConstCStr path);
Bool32 FreeExternalLib(ExtLib lib);
ProcPtr LoadExternalSym(ExtLib lib, CStr name);
Bool32 LoadFuncIfNeeded(ExtLib lib, ProcPtr *ptr, CStr name);

#define DefineExtFunc(lib, rettype, name, arglist, arglistt)         \
typedef rettype (*name##_ProcPtr) arglist;                           \
static name##_ProcPtr name##_Ptr = NULL;                             \
                                                                     \
rettype name arglist {                                               \
	if (LoadFuncIfNeeded(lib, (ProcPtr*)&name##_Ptr, (CStr)#name))   \
	  return (name##_Ptr) arglistt;                                  \
	return 0;                                                        \
}

#define DefineExtFuncVoid(lib, name, arglist, arglistt)              \
typedef void (*name##_ProcPtr) arglist;                              \
static name##_ProcPtr name##_Ptr = NULL;                             \
                                                                     \
void name arglist {                                                  \
	if (LoadFuncIfNeeded(lib, (ProcPtr*)&name##_Ptr, (CStr)#name))   \
	  name##_Ptr arglistt;                                           \
	return;                                                          \
}

#define DefineExtFuncEx(lib, rettype, name, arglist, arglistt)       \
typedef rettype (*name##_ProcPtr) arglist;                           \
static name##_ProcPtr name##_Ptr = NULL;                             \
                                                                     \
rettype name arglist {                                               \
	if (LoadFuncIfNeededEx(lib, (ProcPtr*)&name##_Ptr, (CStr)#name)) \
	  return (name##_Ptr) arglistt;                                  \
	return 0;                                                        \
}

#define DefineExtFuncExVoid(lib, name, arglist, arglistt)            \
typedef void (*name##_ProcPtr) arglist;                              \
static name##_ProcPtr name##_Ptr = NULL;                             \
                                                                     \
void name arglist {                                                  \
	if (LoadFuncIfNeededEx(lib, (ProcPtr*)&name##_Ptr, (CStr)#name)) \
	  name##_Ptr arglistt;                                           \
	return;                                                          \
}

#define ClearExtFunc(name)	name##_Ptr = NULL;

#define LoadExtData(lib, vartype, name)                              \
if (!(name##_Ptr = (vartype*)LoadExternalSym(lib, (CStr)#name))) {   \
	DbgPrintf("Couldn't load symbol " #name);                        \
	goto errOut;                                                     \
}

#define LoadExtDataNoFail(lib, vartype, name)                  \
(name##_Ptr = (vartype*)LoadExternalSym(lib, (CStr)#name))

#ifdef __cplusplus
}
#endif

#endif
