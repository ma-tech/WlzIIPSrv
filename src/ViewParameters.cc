#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _ViewParameters_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         ViewParameters.cc
* \author       Zsolt Husz,Bill Hill
* \date         June 2008
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C), [2016],
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
* \brief	Storage for session dependent, Woolz specific parameters.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "ViewParameters.h"

/*!
* \ingroup	WlzIIPServer
* \brief	Default constructor.
*/
ViewParameters::
ViewParameters()
{
  dist            = 0.0;
  yaw             = 0.0;
  pitch           = 0.0;
  roll            = 0.0;
  scale           = 1.0;
  mode            = WLZ_UP_IS_UP_MODE;
  fixed.vtX       = fixed.vtY  = fixed.vtZ  = 0.0;
  fixed2.vtX      = fixed2.vtY = fixed2.vtZ = 0.0;
  up.vtX          = up.vtY = 0.0;
  up.vtZ          = -1.0;
  depth           = 0.0;
  rmd	          = RENDERMODE_SECT;
  x               = 0;
  y               = 0;
  tile            = 0;
  queryPointType  = QUERYPOINTTYPE_NONE;
  queryPoint.vtX  = 0;
  queryPoint.vtY  = 0;
  queryPoint.vtZ  = 0;
  alpha           = false;
  selector        = NULL;
  lastsel         = NULL;
}

/*! 
* \ingroup	WlzIIPServer
* \brief	Copy constructor
*/
ViewParameters::
ViewParameters(const ViewParameters& viewParameters)
{
  dist            = viewParameters.dist;
  yaw             = viewParameters.yaw;
  pitch           = viewParameters.pitch;
  roll            = viewParameters.roll;
  scale           = viewParameters.scale;
  mode            = viewParameters.mode;
  fixed           = viewParameters.fixed;
  fixed2          = viewParameters.fixed2;
  up              = viewParameters.up;
  depth           = viewParameters.depth;
  rmd	      	  = viewParameters.rmd;
  x               = viewParameters.x;
  y               = viewParameters.y;
  tile            = viewParameters.tile;
  queryPointType  = viewParameters.queryPointType;
  queryPoint      = viewParameters.queryPoint;
  alpha           = viewParameters.alpha;
  lastsel = NULL;
  selector = NULL;

  //copy selection structure
  CompoundSelector * iter = viewParameters.selector;
  while(iter)
  {
    if(!lastsel)
    {
      lastsel = new CompoundSelector;
      selector = lastsel;
    }
    else
    {
      lastsel->next = new CompoundSelector;
      lastsel = lastsel->next;
    };
    *lastsel = *iter;
    WlzExpAssign(iter->expression);
    iter = iter->next;
  }
  if(lastsel)
  {
    lastsel->next = NULL;
  }
}

/*!
* \ingroup	WlzIIPServer
* \brief	Assignement operator.
*/
const ViewParameters&
ViewParameters:: operator=(const ViewParameters& viewParameters)
{
  dist            = viewParameters.dist;
  yaw             = viewParameters.yaw;
  pitch           = viewParameters.pitch;
  roll            = viewParameters.roll;
  scale           = viewParameters.scale;
  mode            = viewParameters.mode;
  fixed           = viewParameters.fixed;
  fixed2          = viewParameters.fixed2;
  up              = viewParameters.up;
  depth           = viewParameters.depth;
  rmd             = viewParameters.rmd;
  x               = viewParameters.x;
  y               = viewParameters.y;
  tile            = viewParameters.tile;
  queryPointType  = viewParameters.queryPointType;
  queryPoint      = viewParameters.queryPoint;
  alpha           = viewParameters.alpha;

  if (selector)
      delete selector;

  lastsel = NULL;
  selector = NULL;

  //copy selection structure
  CompoundSelector * iter = viewParameters.selector;
  while(iter)
  {
    if(!lastsel)
    {
      lastsel = new CompoundSelector;
      selector = lastsel;
    }
    else
    {
      lastsel->next = new CompoundSelector;
      lastsel = lastsel->next;
    };
    *lastsel = *iter;
    WlzExpAssign(iter->expression);
    iter = iter->next;
  }
  if(lastsel)
  {
    lastsel->next = NULL;
  }
  return(*this);
}

bool
ViewParameters:: operator==(const ViewParameters& vp) const
{
  bool eq;

  eq =  vp.dist  &&
	yaw   == vp.yaw   &&
	pitch == vp.pitch &&
	roll  == vp.roll  &&
	scale == vp.scale &&
	mode  == vp.mode  &&
	alpha == vp.alpha &&
	WlzGeomVtxEqual3D(fixed, vp.fixed, 0)  &&
	WlzGeomVtxEqual3D(fixed2, vp.fixed2, 0)  &&
	WlzGeomVtxEqual3D(up, vp.up ,0) ;
  return(eq);
}
/*!
* \return	Woolz error code.
* \ingroup	WlzIIPServer
* \brief	Sets the sectioning mode.
* \param	m			Required section mode as
* 					case-insensitive string. Accepted
* 					values are: STATUE, UP_IS_UP,
* 					FIXED_LINE, ZERO_ZETA and ZETA
*/
WlzErrorNum 
ViewParameters::
setMode(string m)
{ 
  WlzErrorNum errNum = WLZ_ERR_NONE;

  //make it uppercase
  transform( m.begin(), m.end(), m.begin(), ::toupper );
  if (m=="STATUE")
  {
    mode = WLZ_STATUE_MODE;
  }
  else if (m=="UP_IS_UP")
  {
    mode = WLZ_UP_IS_UP_MODE;
  }
  else if (m=="FIXED_LINE")
  {
    mode = WLZ_FIXED_LINE_MODE;
  }
  else if (m=="ZERO_ZETA")
  {
    mode = WLZ_ZERO_ZETA_MODE;
  }
  else if (m=="ZETA")
  {
    mode = WLZ_ZETA_MODE;
  }
  else 
  {
    mode = WLZ_UP_IS_UP_MODE;
    errNum = WLZ_ERR_PARAM_DATA;
  }
  return(errNum);
}

/*!
* \return	Woolz error code.
* \ingroup	WlzIIPServer
* \brief	Set the render depth using the given render depth string.
* \param	ds			Render depth.
*/
WlzErrorNum 
ViewParameters::
setRenderDepth(string ds)
{
  WlzErrorNum  errNum = WLZ_ERR_PARAM_DATA;
  double	   d = 0.0;
  const double eps = -1.0e-06;

  if((sscanf(ds.c_str(), "%lg", &d) == 1) || (d > eps))
  {
    depth = d;
    errNum = WLZ_ERR_NONE;
  }
  return(errNum);
}

/*!
* \return	Woolz error code.
* \ingroup	WlzIIPServer
* \brief	Set the render mode using the given render mode string.
* \param	m			Render mode as case-insensitive string.
* 					Accepted values are: SECT, PRJN, PRJD
* 					and PRJV.
*/
WlzErrorNum 
ViewParameters::
setRenderMode(string m)
{
  WlzErrorNum errNum = WLZ_ERR_NONE;

  //make it uppercase
  transform(m.begin(), m.end(), m.begin(), ::toupper);

  if(m == "SECT")
  {
    rmd = RENDERMODE_SECT;
  }
  else if(m == "PRJN")
  {
    rmd = RENDERMODE_PROJ_N;
  }
  else if(m == "PRJD")
  {
    rmd = RENDERMODE_PROJ_D;
  }
  else if(m == "PRJV")
  {
    rmd = RENDERMODE_PROJ_V;
  }
  else 
  {
    rmd = RENDERMODE_SECT;
    errNum = WLZ_ERR_PARAM_DATA;
  }
  return(errNum);
}

/*!
* \ingroup	WlzIIPServer
* \brief	Set a 2D query point relative to a tile.
* \param	xx		Current point x coordinate.
* \param	yy		Current point y coordinate.
* \param	tt		Current tile number.
*/
void
ViewParameters::
setPoint(int xx, int yy, int tt)
{
  x = xx;
  y = yy;
  tile = tt;
  // if query point was set, mark it
  if(queryPointType == QUERYPOINTTYPE_NONE)
  {
    queryPointType = QUERYPOINTTYPE_2D;
  }
}

/*!
* \ingroup	WlzIIPServer
* \brief	Sets the view parameters by computing a best fit plane
* 		through the given point positions.
* \param 	nPos		Number of positions.
* \param        pos		Array of positions.
*/
void
ViewParameters::
setFromPlaneFit(int nPos, WlzDVertex3 *pos)
{
  WlzVertexP	posP;
  WlzDVertex3	pip;
  WlzDVertex3   nrm;
  WlzThreeDViewStruct *vs = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  posP.d3 = pos;
  errNum = WlzFitPlaneSVD(WLZ_VERTEX_D3, nPos, posP, &pip, &nrm);
  if(errNum == WLZ_ERR_NONE)
  {
    WlzDVertex3 up = {0.0};

    vs = Wlz3DViewStructFromNormal(nrm, pip, up, &errNum);
  }
  if(errNum == WLZ_ERR_NONE)
  {
    WlzDVertex3 tmp;

    pitch = vs->phi * 180 / ALG_M_PI;
    yaw = vs->theta * 180 / ALG_M_PI;
    // Set distance leaving the current fixed point as is.
    WLZ_VTX_3_SUB(tmp, pip, fixed);
    dist = WLZ_VTX_3_DOT(tmp, nrm);
    LOG_INFO("ViewParameters::setFromPlaneFit() setting pitch = " << pitch <<
	     ", yaw = " << yaw   <<
	     ", dist = " << dist);
  }
  (void )WlzFree3DViewStruct(vs);
  if(errNum != WLZ_ERR_NONE)
  {
    LOG_WARN("ViewParameters::setFromPlaneFit() failed to set view.");
  }
}
