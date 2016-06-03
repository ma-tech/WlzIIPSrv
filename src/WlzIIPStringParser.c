#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzIIPStringParser_c[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzIIPStringParser.c
* \author       Bill Hill
* \date         June 2016
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
* \brief	Functions for parsing strings within the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#include <stdio.h>
#include <string.h>
#include <Wlz.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*!
* \return	Zero when no error or one plus the index into the given
* 		string at which the first error occurred.
* \ingroup	WlzIIPServer
* \brief	Parses comma separated strings of unsigned integer indices
*               and floating point positions, where the floating point
*               positions are enclosed within round brackets and are
*               themselves comma seperated, eg:
*                 3,4,(5.2,-3.1,6),7,(7,6.2,0.3)
* \param	dstNIdx			Destination pointer for the number
* 					of indices parsed.
* \param	idx			Given index buffer. Assumed to be
* 					valid and sufficient for the given
* 					string.
* \param	dstNPos			Destination pointer for the number
* 					of positions parsed.
* \param	pos			Given position buffer. Assumed to be
* 					valid and sufficient for the given
* 					string.
* \param	str			String to parse.
*/
int		WlzIIPStrParseIdxAndPos(int *dstNIdx, int *idx,
					int *dstNPos, WlzDVertex3 *pos,
					const char *str)
{
  int       ok = 1,
	    nPos = 0,
	    nIdx = 0,
	    inNum = 0,
	    inPos = 0,
	    err;
  const char *s,
	    *sN,
	    *sP;

  s = str;
  while(ok && *s)
  {
    if(*s == '(')
    {
      if(inPos || inNum)
      {
	ok = 0;
      }
      else
      {
	inNum = 0;
	inPos = 1;
      }
    }
    else if(*s == ')')
    {
      if(!inNum || (++inPos != 5))
      {
	ok = 0;
      }
      else
      {
	if(sscanf(sP, "%lg,%lg,%lg",
	      &(pos[nPos].vtX), &(pos[nPos].vtY), &(pos[nPos].vtZ)) == 3)
	{
	  ++nPos;
	}
	else
	{
	  ok = 0;
	}
      }
      inNum = 0;
    }
    else if(*s == ',')
    {
      if(inPos)
      {
	if(inPos < 5)
	{
	  ++inPos;
	}
	else if(inPos == 5)
	{
	  inPos = 0;
	}
	else
	{
	  ok = 0;
	}
      }
      else if(inNum)
      {
	if(sscanf(sN, "%d", &(idx[nIdx])) == 1)
	{
	  ++nIdx;
	}
	else
	{
	  ok = 0;
	}
      }
      else
      {
	ok = 0;
      }
      inNum = 0;
    }
    else if(strchr("+-0123456789.eE", *s))
    {
      if(!inNum)
      {
	sN = s;
	inNum = 1;
      }
      if(inPos)
      {
	if(inPos == 1)
	{
	  sP = s;
	  ++inPos;
	}
      }
      else if(strchr("-.eE", *s))
      {
	ok = 0;
      }
    }
    else
    {
      ok = 0;
    }
    ++s;
  }
  if(ok && inNum)
  {
    if(sscanf(sN, "%d", &(idx[nIdx])) == 1)
    {
      ++nIdx;
    }
    else
    {
      ok = 0;
    }
  }
  if(ok)
  {
    err = 0;
    if(dstNIdx)
    {
      *dstNIdx = nIdx;
    }
    if(dstNPos)
    {
      *dstNPos = nPos;
    }
  }
  else
  {
    err = s - str + 1;
  }
  return(err);
}            


#ifdef __cplusplus
}
#endif

