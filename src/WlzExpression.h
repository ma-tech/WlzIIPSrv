#ifndef WLZ_EXPRESSION_H
#define WLZ_EXPRESSION_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzExpression_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzExpression.h
* \author       Bill Hill
* \date         October 2011
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C), [2012],
* The University Court of the University of Edinburgh,
* Old College, Edinburgh, UK.
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be
* useful but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA  02110-1301, USA.
* \brief	Constants, type definitions and function prototypes for
* 		parsing Woolz expressions. This file has been written in
* 		ANSI C partly to avoid the problems encountered with using
* 		flex and bison (or lex and yacc) with C++.
* \ingroup	WlzIIPServer
*/


#ifdef __cplusplus
extern "C"
{
#endif

#include <Wlz.h>

typedef enum _WlzExpOpType
{
  WLZ_EXP_OP_NONE	= 0,
  WLZ_EXP_OP_INDEX,
  WLZ_EXP_OP_INDEXLST,
  WLZ_EXP_OP_INDEXRNG,
  WLZ_EXP_OP_BACKGROUND,
  WLZ_EXP_OP_DIFF,
  WLZ_EXP_OP_DILATION,
  WLZ_EXP_OP_DOMAIN,
  WLZ_EXP_OP_EROSION,
  WLZ_EXP_OP_FILL,
  WLZ_EXP_OP_INTERSECT,
  WLZ_EXP_OP_OCCUPANCY,
  WLZ_EXP_OP_SETVALUE,
  WLZ_EXP_OP_THRESHOLD,
  WLZ_EXP_OP_TRANSFER,
  WLZ_EXP_OP_UNION
} WlzExpOpType;

typedef enum _WlzExpCmpType
{
  WLZ_EXP_CMP_NONE	= 0,
  WLZ_EXP_CMP_LT,
  WLZ_EXP_CMP_LE,
  WLZ_EXP_CMP_EQ,
  WLZ_EXP_CMP_GE,
  WLZ_EXP_CMP_GT
} WlzExpCmpType;

typedef enum _WlzExpParamType
{
  WLZ_EXP_PRM_NONE	= 0,
  WLZ_EXP_PRM_UINT,
  WLZ_EXP_PRM_CMP,
  WLZ_EXP_PRM_EXP,
  WLZ_EXP_PRM_OBJ
} WlzExpParamType;

typedef union _WlzExpOpParamVal
{
  void		*v;
  unsigned int   u;
  WlzExpCmpType	 cmp;
  WlzObject	 *obj;
  struct _WlzExp *exp;
} WlzExpOpParamVal;

typedef struct _WlzExpOpParam
{
  WlzExpParamType  type;
  WlzExpOpParamVal val;
} WlzExpOpParam;

typedef struct _WlzExp
{
  WlzExpOpType 	type;
  int		linkcount;
  unsigned int	nParam;
  WlzExpOpParam	*param;
}WlzExp;

typedef WlzExp WlzExpIdxArray;

typedef WlzExp WlzExpIdxList;

typedef WlzExp WlzExpIdxRange;

extern WlzExp          		*WlzExpMake(
				  unsigned int nParam);
extern WlzExp			*WlzExpAssign(
				  WlzExp *exp);
extern void            		WlzExpFree(
				  WlzExp *e);
extern WlzExp			*WlzExpParse(
				  const char *str,
				  unsigned int *dstNPar,
				  unsigned int *par);
extern WlzExp          		*WlzExpMakeIndex(
				  unsigned int idx0);
extern WlzExp          		*WlzExpMakeIndexRange(
				  unsigned int idx0,
				  unsigned int idx1);
extern WlzExp			*WlzExpMakeIndexList(
				  WlzExp *e0,
				  WlzExp *e1);
extern WlzExpCmpType		WlzExpCmpFromStr(
				  const char *str);
extern const char      		*WlzExpCmpToStr(
				  WlzExpCmpType cmp);
extern WlzObject      		*WlzExpEval(
				  WlzObject *inObj,
				  WlzExp *e,
				  WlzErrorNum *dstErr);
extern char            		*WlzExpStr(
				  WlzExp *e,
				  int *dstStrLen,
				  WlzErrorNum *dstErr);

extern WlzExp          		*WlzExpMakeBackground(
				  WlzExp *e0,
				  unsigned int val);
extern WlzExp          		*WlzExpMakeDiff(
				  WlzExp *e0,
				  WlzExp *e1);
extern WlzExp          		*WlzExpMakeDilation(
				  WlzExp *e0,
				  unsigned int val);
extern WlzExp			*WlzExpMakeDomain(
                                  WlzExp *e0);
extern WlzExp          		*WlzExpMakeErosion(
				  WlzExp *e0,
				  unsigned int val);
extern WlzExp			*WlzExpMakeFill(
                                  WlzExp *e0);
extern WlzExp          		*WlzExpMakeIntersect(
				  WlzExp *e0,
				  WlzExp *e1);
extern WlzExp          		*WlzExpMakeOccupancy(
				  WlzExp *e0);
extern WlzExp          		*WlzExpMakeSetValue(
				  WlzExp *e,
				  unsigned int val);
extern WlzExp          		*WlzExpMakeThreshold(
				  WlzExp *e,
				  unsigned int val,
				  WlzExpCmpType cmp);
extern WlzExp          		*WlzExpMakeTransfer(
				  WlzExp *e0,
				  WlzExp *e1);
extern WlzExp          		*WlzExpMakeUnion(
				  WlzExp *e0,
				  WlzExp *e1);

#ifdef __cplusplus
}
#endif

#endif /* WLZ_EXPRESSION_H */
