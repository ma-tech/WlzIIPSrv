#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzExpression_c[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzExpression.c
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
* \brief	Functions for parsing and evaluation Woolz expression
* 		strings for the Woolz IIP server. This file has been written
* 		in ANSI C partly to avoid the problems encountered with using
* 		flex and bison (or lex and yacc) with C++.
* \ingroup	WlzIIPServer
*/

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "WlzExpression.h"
#include "WlzExpParserParam.h"

#ifdef __cplusplus
extern "C"
{
#endif

static int			WlzExpIndexArraySortFn(
				  const void *v0,
				  const void *v1);
static int			WlzExpIndexListFlatten(
				  WlzExp *nE,
				  int pos,
				  WlzExp *e);
static int			WlzExpIndexListCount(
				  WlzExp *e);
static char			*WlzExpIndexListToStr(
				  WlzExp *e,
				  int *sLen,
				  WlzErrorNum *dstErr);
static WlzExp 			*WlzExpIndexListToIndexArray(
				  WlzExp *e,
                                  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpGetObjByIndex(
				  WlzObject *iObj,
				  unsigned int idx,
				  WlzErrorNum *dstErr);
static WlzObject		*WlzExpIndexListToCmpObj(
				  WlzObject *iObj,
				  WlzExp *e,
				  WlzErrorNum *dstErr);

static WlzObject 		*WlzExpBackground(
				  WlzObject *o0,
				  unsigned int idx,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpDiff(
				  WlzObject *o0,
				  WlzObject *o1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpDilation(
				  WlzObject *iObj,
				  unsigned int rad,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpDomain(
				  WlzObject *iObj,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpErosion(
				  WlzObject *iObj,
				  unsigned int rad,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpFill(
				  WlzObject *iObj,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpIntersect(
				  WlzObject *iObj0,
				  WlzObject *iObj1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpOccupancy(
				  WlzObject *gObj,
				  WlzObject *roiObj,
                                  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpSetvalue(
				  WlzObject *iObj,
				  unsigned int val,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpThreshold(
				  WlzObject *iObj,
				  unsigned int val,
				  WlzExpCmpType cmp,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpTransfer(
				  WlzObject *dObj,
				  WlzObject *vObj,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpUnion(
				  WlzObject *iObj0,
				  WlzObject *iObj1,
				  WlzErrorNum *dstErr);

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Allocates an expression with space for the requested number
* 		of parameters.
* \param	nParam			Requested number of parameters.
*/
WlzExp 		*WlzExpMake(unsigned int nParam)
{
  WlzExp	*e;
  
  if((e = AlcCalloc(1, sizeof(WlzExp)) ) != NULL)
  {
    e->nParam = nParam;
    if(nParam > 0)
    {
      if((e->param = AlcCalloc(nParam, sizeof(WlzExp))) == NULL)
      {
	free(e);
	e = NULL;
      }
    }
  }
  return(e);
}

/*!
* \return	Assigned expression or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Assigns the given expression by incrementing it's linkcount.
* \param	exp			Given expression.
*/
WlzExp		*WlzExpAssign(WlzExp *exp)
{
  if(exp)
  {
    if(exp->linkcount < 0)
    {
      exp = NULL;
    }
    else
    {
      ++(exp->linkcount);
    }
  }
  return(exp);
}

/*!
* \ingroup	WlzIIPServer
* \brief	Frees the given expression and any sub expressioons.
* \param	e			The given expression.
*/
void 		WlzExpFree(WlzExp *e)
{
  if(e)
  {
    int		idx;
    WlzExpOpParam  *p;

    if(e->linkcount > 1)
    {
      --(e->linkcount);
    }
    else if(e->linkcount >= 0)
    {
      p = e->param;
      e->linkcount = -1;
      for(idx = 0; idx < e->nParam; ++idx)
      {
	if(p->type == WLZ_EXP_PRM_EXP)
	{
	  WlzExpFree(p->val.exp);
	}
	++p;
      }
      if((e->nParam > 0) && (e->param))
      {
	free(e->param);
      }
      free(e);
    }
  }
}

/*!
* \return	New expression or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Parses the given string and builds a image processing
* 		expression.
* \param	str			Given string to be parsed.
* \param	dstNPar			Destination pointer for the number
* 					of parameters parsed.
* \param	par			Buffer for the parsed parameters with
* 					room for four unsigned ints.
*/
WlzExp		*WlzExpParse(const char *str,
                             unsigned int *dstNPar, unsigned int *par)
{
  unsigned int	i; 
  WlzExp        *e = NULL;
  WlzExpParserParam p;
  YY_BUFFER_STATE state;

  if(str && dstNPar && par && (yylex_init(&(p.scanner)) == 0))
  {
    p.exp = NULL;
    state = yy_scan_string(str, p.scanner);
    if(yyparse(&p))
    {
      /* error parsing */
    }
    else
    {
      if(dstNPar && par)
      {
        *dstNPar = p.nPar;
	for(i = 0; i < p.nPar; ++i)
	{
	  par[i] = p.par[i];
	}
      }
      yy_delete_buffer(state, p.scanner);
      yylex_destroy(p.scanner);
      e = p.exp;
    }
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new background expression.
* \param	e0			Expression for object.
* \param	val			Background value.
*/
WlzExp		*WlzExpMakeBackground(WlzExp *e0, unsigned int val)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_BACKGROUND;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = val;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new difference expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeDiff(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_DIFF;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_EXP;
    e->param[1].val.exp = e1;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new dilation expression.
* \param	e0			Expression for object to be dilated.
* \param	rad			Erosion radius.
*/
WlzExp		*WlzExpMakeDilation(WlzExp *e0, unsigned int rad)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_DILATION;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = rad;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new domain expression.
* \param	e0			Expression for object.
*/
WlzExp		*WlzExpMakeDomain(WlzExp *e0)
{
  WlzExp	*e;

  if((e = WlzExpMake(1)) != NULL)
  {
    e->type = WLZ_EXP_OP_DOMAIN;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new erosion expression.
* \param	e0			Expression for object to be eroded.
* \param	rad			Erosion radius.
*/
WlzExp		*WlzExpMakeErosion(WlzExp *e0, unsigned int rad)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_EROSION;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = rad;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new fill expression.
* \param	e0			Expression for object to be filled.
*/
WlzExp		*WlzExpMakeFill(WlzExp *e0)
{
  WlzExp	*e;

  if((e = WlzExpMake(1)) != NULL)
  {
    e->type = WLZ_EXP_OP_FILL;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new intersect expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeIntersect(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_INTERSECT;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_EXP;
    e->param[1].val.exp = e1;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new occupancy expression.
* \param	e0			Expression which may either be:
* 					NULL implying all domains,
* 					an expression that evaluate to a
* 					domains implying that the occupancy
* 					should only be evaluated within the
* 					expression domain or an index range
* 					which implies that the evaluation
* 					hould be restricted to the domains
* 					within the index range.
*/
WlzExp		*WlzExpMakeOccupancy(WlzExp *e0)
{
  int		n;
  WlzExp	*e;

  n = (e0 != NULL)? 1: 0;
  if((e = WlzExpMake(n)) != NULL)
  {
    e->type = WLZ_EXP_OP_OCCUPANCY;
    if(n != 0)
    {
      e->param[0].type = WLZ_EXP_PRM_EXP;
      e->param[0].val.exp = e0;
    }
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new setvalue expression.
* \param	e0			Expression for object to have
* 					value set.
* \param	val			Value to set.
*/
WlzExp		*WlzExpMakeSetvalue(WlzExp *e0, unsigned int val)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_SETVALUE;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = val;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new threshold expression.
* \param	e0			Expression for object to be
* 					thresholded.
* \param	val			Threshold value.
* \param	cmp			Threshold comparison value.
*/
WlzExp		*WlzExpMakeThreshold(WlzExp *e0, unsigned int val,
				     WlzExpCmpType cmp)
{
  WlzExp	*e;

  if((e = WlzExpMake(3)) != NULL)
  {
    e->type = WLZ_EXP_OP_THRESHOLD;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = val;
    e->param[2].type = WLZ_EXP_PRM_CMP;
    e->param[2].val.cmp = cmp;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new transfer expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeTransfer(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_TRANSFER;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_EXP;
    e->param[1].val.exp = e1;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new union expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeUnion(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_UNION;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_EXP;
    e->param[1].val.exp = e1;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new index expression.
* \param	i0			The index value.
*/
WlzExp		*WlzExpMakeIndex(unsigned int i0)
{
  WlzExp	*e;

  if((e = WlzExpMake(1)) != NULL)
  {
    e->type = WLZ_EXP_OP_INDEX;
    e->param[0].type = WLZ_EXP_PRM_UINT;
    e->param[0].val.u = i0;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new index list expression.
* \param	i0			First index value.
* \param	i1			Second index value.
*/
WlzExp		*WlzExpMakeIndexList(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_INDEXLST;
    e->param[0].type = WLZ_EXP_PRM_EXP;
    e->param[0].val.exp = e0;
    e->param[1].type = WLZ_EXP_PRM_EXP;
    e->param[1].val.exp = e1;
  }
  return(e);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Makes a new index range expression.
* \param	u0			First index value.
* \param	u1			Second index value.
*/
WlzExp		*WlzExpMakeIndexRange(unsigned int u0, unsigned int u1)
{
  WlzExp	*e = NULL;

  if(u1 < u0)
  {
    unsigned int u2;

    u2 = u1;
    u1 = u0;
    u0 = u2;
  }
  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_INDEXRNG;
    e->param[0].type = WLZ_EXP_PRM_UINT;
    e->param[0].val.u = u0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = u1;
  }
  return(e);
}

/*!
* \return	Comparison type which will be WLZ_EXP_CMP_NONE on error.
* \ingroup	WlzIIPServer
* \brief	Computes a comparison type from a comparison string.
* \param	str			Comparison string.
*/
WlzExpCmpType	WlzExpCmpFromStr(const char *str)
{
  WlzExpCmpType	cmp = WLZ_EXP_CMP_NONE;

  if(str)
  {
    if(strncmp(str, "lt", 2) == 0)
    {
      cmp = WLZ_EXP_CMP_LT;
    }
    else if(strncmp(str, "le", 2) == 0)
    {
      cmp = WLZ_EXP_CMP_LE;
    }
    else if(strncmp(str, "eq", 2) == 0)
    {
      cmp = WLZ_EXP_CMP_EQ;
    }
    else if(strncmp(str, "ge", 2) == 0)
    {
      cmp = WLZ_EXP_CMP_GE;
    }
    else if(strncmp(str, "gt", 2) == 0)
    {
      cmp = WLZ_EXP_CMP_GT;
    }
  }
  return(cmp);
}

/*!
* \return	String corresponding to the given comparison, this will be
* 		a pointer to a constant string or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes a string from the given comparison.
* \param	cmp			Given comparison.
*/
const char	*WlzExpCmpToStr(WlzExpCmpType cmp)
{
  const char	*str = NULL;
  const char	*strLT = "lt",
  		*strLE = "le",
		*strEQ = "eq",
		*strGE = "ge",
		*strGT = "gt";

  switch(cmp)
  {
    case WLZ_EXP_CMP_LT:
      str = strLT;
      break;
    case WLZ_EXP_CMP_LE:
      str = strLE;
      break;
    case WLZ_EXP_CMP_EQ:
      str = strEQ;
      break;
    case WLZ_EXP_CMP_GE:
      str = strGE;
      break;
    case WLZ_EXP_CMP_GT:
      str = strGT;
      break;
    default:
      break;
  }
  return(str);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Evaluates a image processing expression with reference to the
* 		given object.
* \param	iObj			Given object.
* \param	e			Morphological expression to be
* 					evaluated using the given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
WlzObject      *WlzExpEval(WlzObject *iObj, WlzExp *e, WlzErrorNum *dstErr)
{
  unsigned int	u0;
  WlzObject	*o0 = NULL,
  		*o1 = NULL;
  WlzExpCmpType	c;
  WlzObject 	*rObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    switch(e->type)
    {
      case WLZ_EXP_OP_NONE:
	break;
      case WLZ_EXP_OP_INDEX:
	u0 = e->param[0].val.u;
	rObj = WlzExpGetObjByIndex(iObj, u0, &errNum);
	break;
      case WLZ_EXP_OP_INDEXRNG: /* FALLTHROUGH */
      case WLZ_EXP_OP_INDEXLST:
	rObj = WlzExpIndexListToCmpObj(iObj, e, &errNum);
	break;
      case WLZ_EXP_OP_BACKGROUND:
        o0 = WlzAssignObject(     
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = (unsigned int)(e->param[1].val.u);
        rObj = WlzExpBackground(o0, u0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_DIFF:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	o1 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[1].val.exp, &errNum), NULL);
	rObj = WlzExpDiff(o0, o1, &errNum);
	(void )WlzFreeObj(o0);
	(void )WlzFreeObj(o1);
	break;
      case WLZ_EXP_OP_DILATION:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = (unsigned int)(e->param[1].val.u);
	rObj = WlzExpDilation(o0, u0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_DOMAIN:
        o0 = WlzAssignObject(     
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
        rObj = WlzExpDomain(o0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_EROSION:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = e->param[1].val.u;
	rObj = WlzExpErosion(o0, u0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_FILL:
        o0 = WlzAssignObject(     
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
        rObj = WlzExpFill(o0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_INTERSECT:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	o1 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[1].val.exp, &errNum), NULL);
	rObj = WlzExpIntersect(o0, o1, &errNum);
	(void )WlzFreeObj(o0);
	(void )WlzFreeObj(o1);
	break;
      case WLZ_EXP_OP_OCCUPANCY:
	switch(e->nParam)
	{
	  case 0:
	    /* Get occupancy wrt all domains. */
	    rObj = WlzExpOccupancy(iObj, NULL, &errNum);
	    (void )WlzFreeObj(o0);
	    break;
	  case 1:
	    if(e->param[0].type != WLZ_EXP_PRM_EXP)
	    {
	      errNum = WLZ_ERR_PARAM_DATA;
	    }
	    else
	    {
	      WlzExp 	*e0;
	      WlzObject *cObj = NULL;

	      e0 = e->param[0].val.exp;
	      switch(e0->type)
	      {
		case WLZ_EXP_OP_INDEX:    /* FALLTHROUGH */
		case WLZ_EXP_OP_INDEXRNG: /* FALLTHROUGH */
		case WLZ_EXP_OP_INDEXLST:
		  cObj = WlzAssignObject(
		         WlzExpIndexListToCmpObj(iObj, e0, &errNum), NULL);
		  rObj = WlzExpOccupancy(cObj, NULL, &errNum); 
		  (void )WlzFreeObj(cObj);
		  break;
		default:
	          errNum = WLZ_ERR_PARAM_DATA;
		  break;
	      }
	    }
	    break;
	  default:
	    errNum = WLZ_ERR_PARAM_DATA;
	    break;
        }
	break;
      case WLZ_EXP_OP_TRANSFER:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	o1 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[1].val.exp, &errNum), NULL);
	rObj = WlzExpTransfer(o0, o1, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_SETVALUE:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = e->param[1].val.u;
	rObj = WlzExpSetvalue(o0, u0, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_THRESHOLD:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = e->param[1].val.u;
	c = e->param[2].val.cmp;
	rObj = WlzExpThreshold(o0, u0, c, &errNum);
	(void )WlzFreeObj(o0);
	break;
      case WLZ_EXP_OP_UNION:
	o0 = WlzAssignObject(
	     (e->param[0].val.v == NULL)?
	     WlzMakeEmpty(&errNum):
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum),
	     NULL);
	o1 = WlzAssignObject(
	     (e->param[1].val.v == NULL)?
	     WlzMakeEmpty(&errNum):
	     WlzExpEval(iObj, e->param[1].val.exp, &errNum),
	     NULL);
	rObj = WlzExpUnion(o0, o1, &errNum);
	(void )WlzFreeObj(o0);
	(void )WlzFreeObj(o1);
	break;
      default:
	errNum = WLZ_ERR_PARAM_TYPE;
	break;
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	String or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes a string representing the given image processing
* 		expression. This is the inverse of parsing a string for
* 		the expression. The returned string should be destroyed
* 		using AlcFree().
* \param	e			Morphological expression to be
* 					evaluated using the given object.
* \param	dstStrLen		Destination pointer for the length
* 					of the string, may be NULL.
* \param	dstErr			Destination error pointer, may be NULL.
*/
char	      	*WlzExpStr(WlzExp *e, int *dstStrLen, WlzErrorNum *dstErr)
{
  int		sLen0,
		sLen1,
		sLen2;
  unsigned int	u0,
  		u1;
  WlzExpCmpType	c;
  char		*s0 = NULL,
  		*s1 = NULL,
		*s2 = NULL;
  const int	sInc = 64; /* Maximum string increment per sub-expresson. */
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(e == NULL)
  {
    if((s2 = AlcStrDup("")) == NULL)
    {
      errNum = WLZ_ERR_MEM_ALLOC;
    }
  }
  else
  {
    switch(e->type)
    {
      case WLZ_EXP_OP_NONE:
	break;
      case WLZ_EXP_OP_INDEX:
	sLen2 = sInc * e->nParam;
	if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	{
	  errNum = WLZ_ERR_MEM_ALLOC;
	}
	else
	{
	  int	i;

	  s1 = s2;
	  for(i = 0; i < (e->nParam - 1); ++i)
	  {
	    u0 = e->param[i].val.u;
	    s1 += sprintf(s1, "%d,", u0);
	  }
	  if(i < e->nParam)
	  {
	    u0 = e->param[i].val.u;
	    s1 += sprintf(s1, "%d", u0);
	  }
	  sLen2 = s1 - s2;
	}
	break;
      case WLZ_EXP_OP_INDEXRNG:
	u0 = e->param[0].val.u;
	u1 = e->param[1].val.u;
	sLen2 = sInc;
	if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	{
	  errNum = WLZ_ERR_MEM_ALLOC;
	}
	else
	{
	  if(u0 == u1)
	  {
	    (void )sprintf(s2, "%d", u0);
	  }
	  else
	  {
	    (void )sprintf(s2, "%d-%d", u0, u1);
	  }
	}
	break;
      case WLZ_EXP_OP_INDEXLST:
	s2 = WlzExpIndexListToStr(e, &sLen2, &errNum);
	break;
      case WLZ_EXP_OP_BACKGROUND:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  sLen2 = sLen0 + sInc;
	  u0 = e->param[1].val.u;
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  (void )sprintf(s2, "background(%s,%d)", s0, u0);
	}
	AlcFree(s0);
        break;
      case WLZ_EXP_OP_DOMAIN:    /* FALLTHROUGH */
      case WLZ_EXP_OP_FILL:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  sLen2 = sLen0 + sInc;
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  switch(e->type)
	  {
	    case WLZ_EXP_OP_DOMAIN:
	      (void )sprintf(s2, "domain(%s)", s0);
	      break;
	    case WLZ_EXP_OP_FILL:
	      (void )sprintf(s2, "fill(%s)", s0);
	      break;
	    default:
	      break;
	  }
	}
	AlcFree(s0);
        break;
      case WLZ_EXP_OP_DIFF:      /* FALLTHROUGH */
      case WLZ_EXP_OP_INTERSECT: /* FALLTHROUGH */
      case WLZ_EXP_OP_TRANSFER:  /* FALLTHROUGH */
      case WLZ_EXP_OP_UNION:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  s1 = WlzExpStr(e->param[1].val.exp, &sLen1, &errNum);
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  sLen2 = sInc + sLen0 + sLen1;
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  int	tst;

	  switch(e->type)
	  {
	    case WLZ_EXP_OP_DIFF:
	      (void )sprintf(s2, "diff(%s,%s)", s0, s1);
	      break;
	    case WLZ_EXP_OP_INTERSECT:
	      (void )sprintf(s2, "intersect(%s,%s)", s0, s1);
	      break;
	    case WLZ_EXP_OP_TRANSFER:
	      (void )sprintf(s2, "transfer(%s,%s)", s0, s1);
	      break;
	    case WLZ_EXP_OP_UNION:
	      tst = (((s1 != NULL) && (*s1 != '\0')) << 1) |
	             ((s0 != NULL) && (*s0 != '\0'));
	      switch(tst)
	      {
	        case 1:
	          (void )sprintf(s2, "union(%s)", s0);
		  break;
		case 2:
	          (void )sprintf(s2, "union(%s)", s1);
		  break;
		case 3:
	          (void )sprintf(s2, "union(%s,%s)", s0, s1);
		  break;
	        default:
		  (void )sprintf(s2, "union()");
		  break;
	      }
	      break;
	    default:
	      break;
	  }
	}
	AlcFree(s0);
	AlcFree(s1);
	break;
      case WLZ_EXP_OP_DILATION: /* FALLTHROUGH */
      case WLZ_EXP_OP_EROSION:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  u0 = e->param[1].val.u;
	  sLen2 = sInc + sLen0;
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  switch(e->type)
	  {
            case WLZ_EXP_OP_DILATION:
	      (void )sprintf(s2, "dilation(%s,%d)", s0, u0);
	      break;
            case WLZ_EXP_OP_EROSION:
	      (void )sprintf(s2, "erosion(%s,%d)", s0, u0);
	      break;
	    default:
	      break;
	  }
	}
	AlcFree(s0);
	break;
      case WLZ_EXP_OP_OCCUPANCY:
	switch(e->nParam)
	{
	  case 0:
	    if((s2 = AlcStrDup("occupancy()")) == NULL)
	    {
	      errNum = WLZ_ERR_MEM_ALLOC;
	    }
	    break;
	  default:
	    s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	    if(errNum == WLZ_ERR_NONE)
	    {
	      sLen2 = sInc + sLen0;
	      if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	      {
		errNum = WLZ_ERR_MEM_ALLOC;
	      }
	      else
	      {
	        (void )sprintf(s2, "occupancy(%s)", s0);
	      }
	    }
	    AlcFree(s0);
	    break;
	}
	break;
      case WLZ_EXP_OP_SETVALUE:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  sLen2 = sInc + sLen0;   
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  u0 = e->param[1].val.u;
	  (void )sprintf(s2, "setvalue(%s,%d)", s0, u0);
	}
	AlcFree(s0);
	break;
      case WLZ_EXP_OP_THRESHOLD:
	s0 = WlzExpStr(e->param[0].val.exp, &sLen0, &errNum);
	if(errNum == WLZ_ERR_NONE)
	{
	  u0 = e->param[1].val.u;
	  c = e->param[2].val.cmp;
	  switch(c)
	  {
            case WLZ_EXP_CMP_NONE:
	      s1 = "";
	      break;
            case WLZ_EXP_CMP_LT:
	      s1 = "lt";
	      break;
            case WLZ_EXP_CMP_LE:
	      s1 = "le";
	      break;
            case WLZ_EXP_CMP_EQ:
	      s1 = "eq";
	      break;
            case WLZ_EXP_CMP_GE:
	      s1 = "ge";
	      break;
            case WLZ_EXP_CMP_GT:
	      s1 = "gt";
	      break;

	  }
	  sLen2 = sInc + sLen0;   
	  if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
	  {
	    errNum = WLZ_ERR_MEM_ALLOC;
	  }
	}
	if(errNum == WLZ_ERR_NONE)
	{
	  (void )sprintf(s2, "threshold(%s,%d,%s)", s0, u0, s1);
	}
	AlcFree(s0);
	break;
      default:
	errNum = WLZ_ERR_PARAM_DATA;
	break;
    }
  }
  if(errNum == WLZ_ERR_NONE)
  {
    if(dstStrLen)
    {
      *dstStrLen = strlen(s2);
    }
  }
  else
  {
    AlcFree(s2);
    s2 = NULL;
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(s2);
}

/*!
* \return	Expression index list string.
* \ingroup	WlzIIPServer
* \brief	Computes an index list string using
* 		WlzExpIndexListToIndexArray() to get a sorted duplicate-free
* 		index list.
* \param	e			The given index list expression, see
* 					WlzExpIndexListToIndexArray().
* \param	sLen			Destination pointer for the string
* 					length, may be NULL.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static char	*WlzExpIndexListToStr(WlzExp *e, int *sLen,
				      WlzErrorNum *dstErr)
{
  int		sLen2 = 0,
		n = 0;
  char		*s1,
  		*s2 = NULL;
  WlzExp  	*a = NULL;
  const int	dpi = 12;         /* > maximum decimal digits per index + 1. */
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  a = WlzExpIndexListToIndexArray(e, &errNum);
  if(errNum == WLZ_ERR_NONE)
  {
    n = a->nParam;
    sLen2 = dpi * n;
    if((s2 = AlcMalloc(sizeof(char) * sLen2)) == NULL)
    {
      errNum = WLZ_ERR_MEM_ALLOC;
    }
    else
    {
      int	i;

      s1 = s2;
      for(i = 0; i < (n - 1); ++i)
      {
        s1 += sprintf(s1, "%u,", a->param[i].val.u);
      }
      if(i < n)
      {
        s1 += sprintf(s1, "%u", a->param[i].val.u);
      }
      sLen2 = s2 - s1;
    }
  }
  WlzExpFree(a);
  if(sLen)
  {
    *sLen = sLen2;
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(s2);
}

/*!
* \return	Woolz compound array object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes a compound array from the given index list
* 		expression using WlzExpIndexListToIndexArray() to get a
* 		sorted duplicate-free index list and the given object.
* \param	iObj			Given object.
* \param	e			The given index list expression, see
* 					WlzExpIndexListToIndexArray().
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpIndexListToCmpObj(WlzObject *iObj,
				          WlzExp *e, WlzErrorNum *dstErr)
{
  int		i,
  		n = 0;
  WlzExp	*a = NULL;
  WlzCompoundArray *cObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  a = WlzExpIndexListToIndexArray(e, &errNum);
  if(errNum == WLZ_ERR_NONE)
  {
    int		m = 1;

    i = a->nParam - 1;
    if((iObj->type == WLZ_COMPOUND_ARR_1) ||
       (iObj->type == WLZ_COMPOUND_ARR_2))
    {
      m = ((WlzCompoundArray *)iObj)->n;
    }
    while((a->param[i].val.u > m) && (i >= 0))
    {
      --i;
    }
    if(i < 0)
    {
      errNum = WLZ_ERR_PARAM_DATA;
    }
    else
    {
      n = i + 1;
      cObj = WlzMakeCompoundArray(WLZ_COMPOUND_ARR_2, 1, n, NULL, WLZ_NULL,
                                &errNum);
      if(errNum == WLZ_ERR_NONE)
      {
	for(i = 0; i < n; ++i)
	{
	  cObj->o[i] = WlzAssignObject(
		       WlzExpGetObjByIndex(iObj, a->param[i].val.u,
		                           &errNum), NULL);
	}
      }
    }
  }
  WlzExpFree(a);
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return((WlzObject *)cObj);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Given an expression with indices, index ranges or any
* 		other form of index list, this function computes a
* 		flat index array expression, ie none of the expresions
* 		parameters is itself an expression and the expression
* 		will have a parameter per index. Index ranges are expanded
* 		(ie 1-4 -> 1,2,3,4) and the reulting indices are sorted
* 		and debuplicated.
* \param	e			Given index expression.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzExp *WlzExpIndexListToIndexArray(WlzExp *e, WlzErrorNum *dstErr)
{
  int		n = 0;
  WlzExp	*nE = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if((e == NULL) || (e->nParam < 0))
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else
  {
    /* Create flat index array from index list. */
    n = WlzExpIndexListCount(e);
    if((nE = WlzExpMake(n)) == NULL)
    {
      errNum = WLZ_ERR_MEM_ALLOC;
    }
    else
    {
      nE->type = WLZ_EXP_OP_INDEX;
      (void )WlzExpIndexListFlatten(nE, 0, e);
    }
  }
  if(errNum == WLZ_ERR_NONE)
  {
    int		i,
    		j;

    /* Sort the indices and then remove any duplicates. */
    n = nE->nParam;
    if(n > 0)
    {
      qsort(nE->param, n, sizeof(WlzExpOpParam), WlzExpIndexArraySortFn);
      j = 0;
      for(i = 0; i < n; ++i)
      {
	if(nE->param[j].val.u != nE->param[i].val.u)
	{
	  ++j;
	  nE->param[j].val.u = nE->param[i].val.u;
	}
      }
      nE->nParam = j + 1;
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(nE);
}

/*!
* \return	The number of indices in the expression.
* \ingroup	WlzIIPServer
* \brief	Counts the number of indice in the given index expression.
* 		The count will include duplicates if exist in the given
* 		expresion. This i a recursive function.
* \param	e			The given index expresion.
*/
static int	WlzExpIndexListCount(WlzExp *e)
{
  int		i,
  		n = 0;

  switch(e->type)
  {
    case WLZ_EXP_OP_INDEX:
      n = e->nParam;
      break;
    case WLZ_EXP_OP_INDEXRNG:
      n = e->param[1].val.u - e->param[0].val.u + 1;
      break;
    case WLZ_EXP_OP_INDEXLST:
      for(i = 0; i < e->nParam; ++i)
      {
        if(e->param[i].type == WLZ_EXP_PRM_EXP)
	{
	  n += WlzExpIndexListCount(e->param[i].val.exp);
	}
      }
      break;
    default:
      break;
  }
  return(n);
}

/*!
* \return	New expression.
* \ingroup	WlzIIPServer
* \brief	Flattens the given index expression so that all it's
* 		parameters are indices (ie none are expressions). This
* 		is a recurive function.
* \param	nE			New expression with room for the
* 					appropriate number of parameters
* 					that will result from flattening
* 					the given expression.
* \param	pos			Position with respect to the first
*					index of the new expression.
* \param	e			Given expresion to be flattened.
*/
static int	WlzExpIndexListFlatten(WlzExp *nE, int pos, WlzExp *e)
{
  int		i,
  		n;

  switch(e->type)
  {
    case WLZ_EXP_OP_INDEX:
      n = e->nParam;
      for(i = 0; i < n; ++i)
      {
	nE->param[pos + i].type = WLZ_EXP_PRM_UINT;
        nE->param[pos + i].val.u = e->param[i].val.u;
      }
      pos += n;
      break;
    case WLZ_EXP_OP_INDEXRNG:
      n = e->param[1].val.u - e->param[0].val.u + 1;
      for(i = 0; i < n; ++i)
      {
	nE->param[pos + i].type = WLZ_EXP_PRM_UINT;
        nE->param[pos + i].val.u = e->param[0].val.u + i;
      }
      pos += n;
      break;
    case WLZ_EXP_OP_INDEXLST:
      n = e->nParam;
      for(i = 0; i < n; ++i)
      {
        pos = WlzExpIndexListFlatten(nE, pos, e->param[i].val.exp);
      }
      break;
    default:
      break;
  }
  return(pos);
}

/*!
* \return	Signed integer.
* \ingroup	WlzIIPServer
* \brief	Sort function for qsort() which sorts the given index
* 		parameters by their unsigned interger value.
* \param	v0			Pointer to first parameter.
* \param	v1			Pointer to second parameter.
*/
static int	WlzExpIndexArraySortFn(const void *v0, const void *v1)
{
  int		cmp;
  WlzExpOpParam	*p0,
  		*p1;

  p0 = (WlzExpOpParam *)v0;
  p1 = (WlzExpOpParam *)v1;
  cmp = p0->val.u - p1->val.u;
  return(cmp);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Gets the Woolz object indexed by the given index value
* 		in the given compound array object. If the given object
* 		is a domain object then index 0 will return that object.
* \param	iObj			Given compound array (or domain)
* 					object.
* \param	idx			Given index.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpGetObjByIndex(WlzObject *iObj, unsigned int idx,
				      WlzErrorNum *dstErr)
{
  WlzCompoundArray *cObj;
  WlzObject	*rObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    switch(iObj->type)
    {
      case WLZ_2D_DOMAINOBJ: /* FALLTHROUGH */
      case WLZ_3D_DOMAINOBJ:
        if(idx != 0)
	{
	  errNum = WLZ_ERR_PARAM_DATA;
	}
	else
	{
	  rObj = iObj;
	}
	break;
      case WLZ_COMPOUND_ARR_1: /* FALLTHROUGH */
      case WLZ_COMPOUND_ARR_2:
	cObj = (WlzCompoundArray *)iObj;
        if((idx < 0) || (idx >= cObj->n))
	{
	  errNum = WLZ_ERR_PARAM_DATA;
	}
	else
	{
	  rObj = cObj->o[idx];
	}
	break;
      default:
	errNum = WLZ_ERR_OBJECT_TYPE;
        break;
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes an object with background value set to the given
* 		value. Because a object can only have a background value
* 		if it has foreground values, foreground values will be
* 		created if required and set to the background value too.
* \param	gObj			Given object.
* \param	bgdI			Background value as an integer.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpBackground(WlzObject *gObj, unsigned int bgdI,
                                   WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzPixelV     bgdV;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(gObj== NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(gObj->domain.core == NULL)
  {
    errNum = WLZ_ERR_DOMAIN_NULL;
  }
  else
  {
    if(bgdI <= 255)
    {
      bgdV.type = WLZ_GREY_UBYTE;
      bgdV.v.ubv = bgdI;
    }
    else
    {
      bgdV.type = WLZ_GREY_INT;
      bgdV.v.inv = bgdI;
    }
    rObj = WlzSetBackGroundNewObj(gObj, bgdV, &errNum);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes the difference between the domains of the
* 		given objects.
* \param	iObj0			First object.
* \param	iObj1			Second object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpDiff(WlzObject *iObj0, WlzObject *iObj1,
                             WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((iObj0 == NULL) || (iObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    rObj = WlzDiffDomain(iObj0, iObj1, &errNum);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes the dilation of the given object. 
* \param	iObj			Given object.
* \param	rad			Radius of structuring element.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpDilation(WlzObject *iObj, unsigned int rad,
                                 WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL,
  		*sObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(rad < 0)
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else
  {
    switch(iObj->type)
    {
      case WLZ_2D_DOMAINOBJ:
	sObj = WlzMakeSphereObject(WLZ_2D_DOMAINOBJ, rad, 0.0, 0.0, 0.0,
	                           &errNum);
        break;
      case WLZ_3D_DOMAINOBJ:
	sObj = WlzMakeSphereObject(WLZ_3D_DOMAINOBJ, rad, 0.0, 0.0, 0.0,
	                           &errNum);
        break;
      default:
        break;
    }
    if(errNum == WLZ_ERR_NONE)
    {
      rObj = WlzStructDilation(iObj, sObj, &errNum);
    }
  }
  (void )WlzFreeObj(sObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Creates a new object which has the domain of the given
* 		object but NULL values .
* \param	iObj			Given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpDomain(WlzObject *iObj, WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    WlzValues	val;

    val.core = NULL;
    rObj = WlzMakeMain(iObj->type, iObj->domain, val, NULL, NULL, &errNum);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes the erosion of the given object. 
* \param	iObj			Given object.
* \param	rad			Radius of structuring element.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpErosion(WlzObject *iObj, unsigned int rad,
                                WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL,
  		*sObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(rad < 0)
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else
  {
    switch(iObj->type)
    {
      case WLZ_2D_DOMAINOBJ:
	sObj = WlzMakeSphereObject(WLZ_2D_DOMAINOBJ, rad, 0.0, 0.0, 0.0,
	                           &errNum);
        break;
      case WLZ_3D_DOMAINOBJ:
	sObj = WlzMakeSphereObject(WLZ_3D_DOMAINOBJ, rad, 0.0, 0.0, 0.0,
	                           &errNum);
        break;
      default:
        break;
    }
    if(errNum == WLZ_ERR_NONE)
    {
      rObj = WlzStructErosion(iObj, sObj, &errNum);
    }
  }
  (void )WlzFreeObj(sObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes a new object in which the domain is that of
* 		the given object filled so that there are no holes
* 		connected to the outside.
* \param	iObj			Given object with domain to be filled.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpFill(WlzObject *iObj, WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  rObj = WlzDomainFill(iObj, &errNum);
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes the intersection of the given objects (which
* 		may be compound array objects).
* \param	iObj0			First given object.
* \param	iObj1			Second given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpIntersect(WlzObject *iObj0, WlzObject *iObj1,
			          WlzErrorNum *dstErr)
{
  int		i;
  WlzCompoundArray *cObj;
  WlzObject	*objs[2],
  		*iObjs[2];
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((iObj0 == NULL) || (iObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    objs[0] = NULL;
    objs[1] = NULL;
    iObjs[0] = iObj0;
    iObjs[1] = iObj1;
    for(i = 0; i < 2; ++i)
    {
      switch(iObjs[i]->type)
      {
	case WLZ_2D_DOMAINOBJ:
	case WLZ_3D_DOMAINOBJ:
	  objs[i] = WlzAssignObject(iObjs[i], NULL);
	  break;
	case WLZ_COMPOUND_ARR_1:
	case WLZ_COMPOUND_ARR_2:
	  cObj = (WlzCompoundArray *)(iObjs[i]);
	  objs[i] = WlzIntersectN(cObj->n, cObj->o, 0, &errNum);
	default:
	  break;
      }
      if(errNum != WLZ_ERR_NONE)
      {
        break;
      }
    }
    if(errNum == WLZ_ERR_NONE) 
    {
      rObj = WlzIntersectN(2, objs, 0, &errNum);
    }
    (void )WlzFreeObj(objs[0]);
    (void )WlzFreeObj(objs[1]);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Occupancy object.
* \ingroup	WlzIIPServer
* \brief	Computes the occupancy of the domains in the given compound
* 		array as a grey valued object. If given the occupancy is
* 		computed within the supplied ROI domain, otherwise the
* 		occupancy is computed within the union of the domains
* 		in the compound array.
* \param	gObj			Object containing the domains for
* 					which the occupancy is to be computed.
* 					Can either be a compound array or a
* 					spatial domain object.
* \param	roiObj			Object with a domain for the region of
* 					interest, may be NULL.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpOccupancy(WlzObject *gObj,
				  WlzObject *roiObj,
                                  WlzErrorNum *dstErr)
{
  WlzObject	*cObj = NULL,
  	     	*rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(gObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    switch(gObj->type)
    {
      case WLZ_2D_DOMAINOBJ:
      case WLZ_3D_DOMAINOBJ:
	if(roiObj)
	{
	  cObj = WlzAssignObject(
	  	 WlzIntersect2(gObj, roiObj, &errNum), NULL);
	}
	else
	{
	  cObj = WlzAssignObject(gObj, NULL);
	}
	break;
      case WLZ_COMPOUND_ARR_1:
      case WLZ_COMPOUND_ARR_2:
	if(roiObj)
	{
	  WlzCompoundArray *gCObj,
	  		   *nCObj = NULL;

	  gCObj = (WlzCompoundArray *)(gObj);
	  nCObj = WlzMakeCompoundArray(WLZ_COMPOUND_ARR_2, 1, gCObj->n, NULL,
	  			       WLZ_NULL, &errNum);
	  if(errNum == WLZ_ERR_NONE)
	  {
	    int		i;

	    for(i = 0; i < gCObj->n; ++i)
	    {
	      nCObj->o[i] = (gCObj->o[i] == NULL)?
			    NULL:
	                    WlzAssignObject(
	                    WlzIntersect2(roiObj, gCObj->o[i], &errNum), NULL);
	    }
	    cObj = (WlzObject *)nCObj;
	  }
	  else
	  {
	    (void )WlzFreeObj((WlzObject *)nCObj);
	    nCObj = NULL;
	  }
	}
	else
	{
	  cObj = WlzAssignObject(gObj, NULL);
	}
      default:
	break;
    }
  }
  if(errNum == WLZ_ERR_NONE)
  {
    rObj = WlzDomainOccupancy(cObj, &errNum);
  }
  (void )WlzFreeObj((WlzObject *)cObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Sets the grey values of an object.
* 		given objects.
* \param	iObj			Given object.
* \param	val			Threshold value.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpSetvalue(WlzObject *iObj, unsigned int val,
				  WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    int		ival = (int )val;
    WlzObjectType tType;
    WlzPixelV	bgdV,
    		fgdV;

    if((ival >= 0) && (ival <= 255))
    {
      fgdV.type = bgdV.type = WLZ_GREY_UBYTE;
      bgdV.v.ubv = 0;
      fgdV.v.ubv = (WlzUByte )ival;
    }
    else if((ival >= SHRT_MIN) && (ival <= SHRT_MAX))
    {
      fgdV.type = bgdV.type = WLZ_GREY_SHORT;
      bgdV.v.shv = 0;
      fgdV.v.shv = (short )ival;
    }
    else
    {
      fgdV.type = bgdV.type = WLZ_GREY_INT;
      bgdV.v.inv = 0;
      fgdV.v.inv = ival;
    }
    tType = WlzGreyTableType(WLZ_GREY_TAB_RAGR, fgdV.type, &errNum);
    if(errNum == WLZ_ERR_NONE)
    {
      rObj = WlzNewObjectValues(iObj, tType, bgdV, 1, fgdV, &errNum);
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes an above, below or at threshold value object
* 		given an object with grey values, a threhold value and
* 		a threhold comparison operator.
* 		given objects.
* \param	iObj			Given object.
* \param	val			Threshold value.
* \param	cmp			Threshold comparison operator.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpThreshold(WlzObject *iObj, unsigned int val,
				  WlzExpCmpType cmp, WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzPixelV	thrVal;
  WlzThresholdType hiLo[2];
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  thrVal.type = WLZ_GREY_INT;
  hiLo[0] = hiLo[1] = WLZ_THRESH_LOW;
  if(iObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    switch(cmp)
    {
      case WLZ_EXP_CMP_LT:
        break;
      case WLZ_EXP_CMP_LE:
	val = val + 1;
        break;
      case WLZ_EXP_CMP_EQ:
	val = val + 1;
        hiLo[1] = WLZ_THRESH_HIGH;
        break;
      case WLZ_EXP_CMP_GE:
        hiLo[0] = WLZ_THRESH_HIGH;
        break;
      case WLZ_EXP_CMP_GT:
        hiLo[0] = WLZ_THRESH_HIGH;
	val = val + 1;
        break;
      default:
        errNum = WLZ_ERR_PARAM_DATA;
	break;
    }
  }
  if(errNum == WLZ_ERR_NONE)
  {
    thrVal.v.inv = (int )val;
    rObj = WlzThreshold(iObj, thrVal, hiLo[0], &errNum);
  }
  if((errNum == WLZ_ERR_NONE) && (cmp == WLZ_EXP_CMP_EQ))
  {
    WlzObject	*tObj;

    thrVal.v.inv = (int )val - 1;
    tObj = WlzThreshold(rObj, thrVal, hiLo[1], &errNum);
    (void )WlzFreeObj(rObj);
    rObj = tObj;
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Creates a new object with the domain of the first given
* 		object and (within the intersection of the two object's
* 		domains) the values of the second given object.
* \param	dObj			First given object.
* \param	vObj			Second given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpTransfer(WlzObject *dObj, WlzObject *vObj,
                                 WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((dObj == NULL) || (vObj == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if((dObj->domain.core == NULL) || (vObj->domain.core == NULL))
  {
    errNum = WLZ_ERR_DOMAIN_NULL;
  }
  else if(vObj->values.core == NULL)
  {
    errNum = WLZ_ERR_VALUES_NULL;
  }
  else
  {
    rObj = WlzGreyTransfer(dObj, vObj, 0, &errNum);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(rObj);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Computes the union of the given objects (which may be
* 		compound array objects).
* \param	iObj0			First given object.
* \param	iObj1			Second given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
static WlzObject *WlzExpUnion(WlzObject *iObj0, WlzObject *iObj1,
                              WlzErrorNum *dstErr)
{
  int		i;
  WlzCompoundArray *cObj;
  WlzObject	*objs[2],
  		*iObjs[2];
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((iObj0 == NULL) || (iObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    objs[0] = NULL;
    objs[1] = NULL;
    iObjs[0] = iObj0;
    iObjs[1] = iObj1;
    for(i = 0; i < 2; ++i)
    {
      switch(iObjs[i]->type)
      {
	case WLZ_EMPTY_OBJ:    /* FALLTHROUGH */
	case WLZ_2D_DOMAINOBJ: /* FALLTHROUGH */
	case WLZ_3D_DOMAINOBJ:
	  objs[i] = WlzAssignObject(iObjs[i], NULL);
	  break;
	case WLZ_COMPOUND_ARR_1:
	case WLZ_COMPOUND_ARR_2:
	  cObj = (WlzCompoundArray *)(iObjs[i]);
	  objs[i] = WlzUnionN(cObj->n, cObj->o, 0, &errNum);
	default:
	  break;
      }
      if(errNum != WLZ_ERR_NONE)
      {
        break;
      }
    }
    if(errNum == WLZ_ERR_NONE) 
    {
      rObj = WlzUnionN(2, objs, 0, &errNum);
    }
    (void )WlzFreeObj(objs[0]);
    (void )WlzFreeObj(objs[1]);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            


#ifdef __cplusplus
}
#endif

