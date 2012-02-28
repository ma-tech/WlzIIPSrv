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

#include <stdlib.h>
#include <stdio.h>
#include "WlzExpression.h"
#include "WlzExpParserParam.h"

#ifdef __cplusplus
extern "C"
{
#endif

static WlzObject 		*WlzExpGetObjByIndex(
				  WlzObject *inObj,
				  unsigned int idx,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpObjFromIndices(
				  WlzObject *inObj,
				  unsigned int u0,
				  unsigned int u1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpIntersect(
				  WlzObject *inObj0,
				  WlzObject *inObj1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpUnion(
				  WlzObject *inObj0,
				  WlzObject *inObj1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpDilation(
				  WlzObject *inObj,
				  unsigned int rad,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpErosion(
				  WlzObject *inObj,
				  unsigned int rad,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpDiff(
				  WlzObject *o0,
				  WlzObject *o1,
				  WlzErrorNum *dstErr);
static WlzObject 		*WlzExpThreshold(
				  WlzObject *inObj,
				  unsigned int val,
				  WlzExpCmpType cmp,
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
    if((e->param = AlcCalloc(nParam, sizeof(WlzExp))) == NULL)
    {
      free(e);
      e = NULL;
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
* \brief	Parses the given string and builds a morphological expression.
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

  if(yylex_init(&(p.scanner)) == 0)
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
* \brief	Makes a new difference expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeDiff(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_DIFF;;
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
* \brief	Makes a new erosion expression.
* \param	e0			Expression for object to be eroded.
* \param	rad			Erosion radius.
*/
WlzExp		*WlzExpMakeErosion(WlzExp *e0, unsigned int rad)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_EROSION;;
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
* \brief	Makes a new dilation expression.
* \param	e0			Expression for object to be dilated.
* \param	rad			Erosion radius.
*/
WlzExp		*WlzExpMakeDilation(WlzExp *e0, unsigned int rad)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_DILATION;;
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
* \brief	Makes a new union expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeUnion(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_UNION;;
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
* \brief	Makes a new intersect expression.
* \param	e0			Expression for first object.
* \param	e1			Expression for second object.
*/
WlzExp		*WlzExpMakeIntersect(WlzExp *e0, WlzExp *e1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_INTERSECT;;
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
* \brief	Makes a new index expression. This may have two different
* 		index values representing a range of indices.
* \param	i0			First index value.
* \param	i1			Second index value.
*/
WlzExp		*WlzExpMakeIndex(unsigned int i0, unsigned int i1)
{
  WlzExp	*e;

  if((e = WlzExpMake(2)) != NULL)
  {
    e->type = WLZ_EXP_OP_INDEX;;
    e->param[0].type = WLZ_EXP_PRM_UINT;
    e->param[0].val.u = i0;
    e->param[1].type = WLZ_EXP_PRM_UINT;
    e->param[1].val.u = i1;
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
* \brief	Evaluates a morphological expression with reference to the
* 		given object.
* \param	iObj			Given object.
* \param	e			Morphological expression to be
* 					evaluated using the given object.
* \param	dstErr			Destination error pointer, may be NULL.
*/
WlzObject      *WlzExpEval(WlzObject *iObj, WlzExp *e, WlzErrorNum *dstErr)
{
  unsigned int	u0,
  		u1;
  WlzObject	*o0,
  		*o1;
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
	u1 = e->param[1].val.u;
	rObj = WlzExpObjFromIndices(iObj, u0, u1, &errNum);
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
      case WLZ_EXP_OP_UNION:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	o1 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[1].val.exp, &errNum), NULL);
	rObj = WlzExpUnion(o0, o1, &errNum);
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
      case WLZ_EXP_OP_EROSION:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = e->param[1].val.u;
	rObj = WlzExpErosion(o0, u0, &errNum);
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
      case WLZ_EXP_OP_THRESHOLD:
	o0 = WlzAssignObject(
	     WlzExpEval(iObj, e->param[0].val.exp, &errNum), NULL);
	u0 = e->param[1].val.u;
	c = e->param[2].val.cmp;
	rObj = WlzExpThreshold(o0, u0, c, &errNum);
	(void )WlzFreeObj(o0);
	break;
      default:
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
* \brief	Computes a string representing the given morphological
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
      case WLZ_EXP_OP_INTERSECT: /* FALLTHROUGH */
      case WLZ_EXP_OP_UNION: /* FALLTHROUGH */
      case WLZ_EXP_OP_DIFF:
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
	  switch(e->type)
	  {
	    case WLZ_EXP_OP_INTERSECT:
	      (void )sprintf(s2, "intersect(%s,%s)", s0, s1);
	      break;
	    case WLZ_EXP_OP_UNION:
	      (void )sprintf(s2, "union(%s,%s)", s0, s1);
	      break;
	    case WLZ_EXP_OP_DIFF:
	      (void )sprintf(s2, "diff(%s,%s)", s0, s1);
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

static WlzObject *WlzExpGetObjByIndex(WlzObject *inObj, unsigned int i,
				      WlzErrorNum *dstErr)
{
  WlzCompoundArray *cObj;
  WlzObject	*rObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(inObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    switch(inObj->type)
    {
      case WLZ_2D_DOMAINOBJ: /* FALLTHROUGH */
      case WLZ_3D_DOMAINOBJ:
        if(i != 0)
	{
	  errNum = WLZ_ERR_PARAM_DATA;
	}
	else
	{
	  rObj = inObj;
	}
	break;
      case WLZ_COMPOUND_ARR_1: /* FALLTHROUGH */
      case WLZ_COMPOUND_ARR_2:
	cObj = (WlzCompoundArray *)inObj;
        if((i < 0) || (i >= cObj->n))
	{
	  errNum = WLZ_ERR_PARAM_DATA;
	}
	else
	{
	  rObj = cObj->o[i];
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

static WlzObject *WlzExpObjFromIndices(WlzObject *inObj,
			               unsigned int u0, unsigned int u1,
				       WlzErrorNum *dstErr)
{
  int		i,
  		n;
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((u0 < 0) || ((n = u1 - u0 + 1) <= 0))
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else if(u0 == u1)
  {
    rObj = WlzExpGetObjByIndex(inObj, u0, &errNum);
  }
  else
  {
    WlzCompoundArray *cObj;

    cObj = WlzMakeCompoundArray(WLZ_COMPOUND_ARR_2, 1, n, NULL, WLZ_NULL,
                                &errNum);
    if(errNum == WLZ_ERR_NONE)
    {
      for(i = 0; i < n; ++i)
      {
	cObj->o[i] = WlzAssignObject(
		     WlzExpGetObjByIndex(inObj, u0 + i, &errNum), NULL);
        if(errNum != WLZ_ERR_NONE)
	{
	  break;
	}
      }
      if(errNum == WLZ_ERR_NONE)
      {
        rObj = (WlzObject *)cObj;
      }
      else
      {
        (void )WlzFreeObj((WlzObject *)cObj);
      }
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

static WlzObject *WlzExpIntersect(WlzObject *inObj0, WlzObject *inObj1,
			          WlzErrorNum *dstErr)
{
  int		i;
  WlzCompoundArray *cObj;
  WlzObject	*objs[2],
  		*inObjs[2];
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((inObj0 == NULL) || (inObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    objs[0] = NULL;
    objs[1] = NULL;
    inObjs[0] = inObj0;
    inObjs[1] = inObj1;
    for(i = 0; i < 2; ++i)
    {
      switch(inObjs[i]->type)
      {
	case WLZ_2D_DOMAINOBJ:
	case WLZ_3D_DOMAINOBJ:
	  objs[i] = WlzAssignObject(inObjs[i], NULL);
	  break;
	case WLZ_COMPOUND_ARR_1:
	case WLZ_COMPOUND_ARR_2:
	  cObj = (WlzCompoundArray *)(inObjs[i]);
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

static WlzObject *WlzExpUnion(WlzObject *inObj0, WlzObject *inObj1,
                              WlzErrorNum *dstErr)
{
  int		i;
  WlzCompoundArray *cObj;
  WlzObject	*objs[2],
  		*inObjs[2];
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((inObj0 == NULL) || (inObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    objs[0] = NULL;
    objs[1] = NULL;
    inObjs[0] = inObj0;
    inObjs[1] = inObj1;
    for(i = 0; i < 2; ++i)
    {
      switch(inObjs[i]->type)
      {
	case WLZ_2D_DOMAINOBJ:
	case WLZ_3D_DOMAINOBJ:
	  objs[i] = WlzAssignObject(inObjs[i], NULL);
	  break;
	case WLZ_COMPOUND_ARR_1:
	case WLZ_COMPOUND_ARR_2:
	  cObj = (WlzCompoundArray *)(inObjs[i]);
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

static WlzObject *WlzExpDilation(WlzObject *inObj, unsigned int rad,
                                 WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL,
  		*sObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(inObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(rad < 0)
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else
  {
    switch(inObj->type)
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
      rObj = WlzStructDilation(inObj, sObj, &errNum);
    }
  }
  (void )WlzFreeObj(sObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

static WlzObject *WlzExpErosion(WlzObject *inObj, unsigned int rad,
                                WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL,
  		*sObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if(inObj == NULL)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(rad < 0)
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  else
  {
    switch(inObj->type)
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
      rObj = WlzStructErosion(inObj, sObj, &errNum);
    }
  }
  (void )WlzFreeObj(sObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

static WlzObject *WlzExpDiff(WlzObject *inObj0, WlzObject *inObj1,
                             WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if((inObj0 == NULL) || (inObj1 == NULL))
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    rObj = WlzDiffDomain(inObj0, inObj1, &errNum);
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }          
  return(rObj);
}            

static WlzObject *WlzExpThreshold(WlzObject *inObj, unsigned int val,
				  WlzExpCmpType cmp, WlzErrorNum *dstErr)
{
  WlzObject     *rObj = NULL;
  WlzPixelV	thrVal;
  WlzThresholdType hiLo[2];
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  thrVal.type = WLZ_GREY_INT;
  hiLo[0] = hiLo[1] = WLZ_THRESH_LOW;
  if(inObj == NULL)
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
    rObj = WlzThreshold(inObj, thrVal, hiLo[0], &errNum);
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

#ifdef __cplusplus
}
#endif

