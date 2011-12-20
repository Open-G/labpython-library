/*
 * LabVIEW Tyepedef Handling Interface Definition
 * ----------------------------------------------
 *
 * LabVIEW uses an involved but flexible system to describe its datatypes in a
 * single vector. The whole thing can be highly recursive.
 *
 * Created by: Rolf Kalbermatter
 *
 * @(#) $Id: lvtypedef.h,v 1.7 2007/11/02 11:41:26 labviewer Exp $";
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
#ifndef LVTYPEDEF_H
#define LVTYPEDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extcode.h"

/* For the numeric typecodes check the extcode.h file */

#define boolU16Code		0x20
#define boolCode		0x21

#define stringCode		0x30
#define handleCode		0x31
#define pathCode		0x32
#define pictCode		0x33

#define tagCode			0x37

#define arrayCode		0x40

#define clusterCode		0x50
#define oldVariantCode	0x52	// LabVIEW 5.x OLE varian
#define variantCode		0x53	// LabVIEW 6.x generic variant
#define timeStampCode	0x54	// LabVIEW 7.x 128 bit timestamp


#define refNumCode		0x70

#define functionCode	0xF0

/* Numeric types */
#define Type(c)			((uInt8)(c))
#define IsPureReal(c)	(Type(c) && Type(c) <= (uInt8)fX)

/* Numeric classes */
enum {	integerClass, unsignedClass, floatClass, complexClass };
#define Phys		0x10

#define IsNumeric(c)	(Type(c) && Type(c) <= Type(cX|Phys))

/* LabVIEW actually exports this */
extern int32 NumSize(int32 tc);
extern int32 NumClass(int32 tc);
extern int32 DataSize(int16 *tdp);

#define IsSigned(c)		(IsNumeric(c) && NumClass(c) == integerClass)
#define IsUnsigned(c)	(IsNumeric(c) && NumClass(c) == unsignedClass)
#define IsInteger(c)	(IsNumeric(c) && NumClass(c) <= unsignedClass)
#define IsReal(c)		(IsNumeric(c) && NumClass(c) == floatClass)
#define IsFloat(c)		(IsNumeric(c) && NumClass(c) >= floatClass)
#define IsComplex(c)	(IsNumeric(c) && NumClass(c) == complexClass)

#define SgnToUnsgn(c)	(c + 4)
#define UnsgnToSgn(c)	(c - 4)
#define CplxToFlt(c)	(c - 3)
#define FltToCplx(c)	(c + 3)

#define CopyNumVal(c, s, d)		MoveBlock(s, d, NumSize(c))

#define TDPtr(tdp, o)			((int16*)((UPtr)(tdp)+(o)))		/* td pointer */
#define TDSize(tdp, o)			(TDPtr(tdp, o)[0])				/* td size */
#define TDType(tdp, o)			(Type(TDPtr(tdp, o)[1]))		/* td type */
#define NextTD(tdp, o)			(o + TDInt(tdp, o))				/* offset of next td */

#define EnumTDNItms(tdp, o)		(TDPtr(tdp, o)[2])
#define EnumTDItmPtr(tdp, o)	((UPtr)TDPtr(tdp, o)[3])

#define ArrayTDDims(tdp, o)		(TDPtr(tdp, o)[2])				/* td array dims */
#define ArrayTDHdrSize(tdp, o)	(ArrayTDDims(tdp, o) * 4)		/* size of dimsize header */
#define ArrEltTDOffset(tdp, o)	(o + 6 + ArrayTDHdrSize(tdp, o))

#define ClustTDNElts(tdp, o)	(TDPtr(tdp, o)[2])				/* td first cluster elements */
#define ClustEltTDOffset(tdp, o)	((o)+6)						/* td next cluster elements */

#define RefNumTDSubType(tdp, o)	(TDPtr(tdp, o)[2])
#define RefNumTDOffset(tdp, o)	((o)+6)

MgErr SetArraySize(int16 **tdp, int32 off, int32 dims, UHandle *p, int32 size);
MgErr FPathToText(Path, LStrPtr);

#ifdef __cplusplus
}
#endif

#endif
