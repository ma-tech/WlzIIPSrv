#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _ImageMap_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         ImageMap.cc
* \author       Bill Hill
* \date         July 2012
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
* \brief	Provides the image map handling functions for
* 		the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#include "ViewParameters.h"

using namespace std;

/*!
* \return	Mapping function string.
* \ingroup	WlzIIPServer
* \brief	Returns an image mapping string representation of the
* 		mapping function grey transform.
* \param	type			Grey transform type.
*/
std::string	
ImageMap::mapTypeToString(WlzGreyTransformType type)
{
  std::string	str;

  switch(type)
  {
    case WLZ_GREYTRANSFORMTYPE_IDENTITY:
      str = "IDENTITY";
      break;
    case WLZ_GREYTRANSFORMTYPE_LINEAR:
      str = "LINEAR";
      break;
    case WLZ_GREYTRANSFORMTYPE_GAMMA:
      str = "GAMMA";
      break;
    case WLZ_GREYTRANSFORMTYPE_SIGMOID:
      str = "SIGMOID";
      break;
    default:
      str = "INVALID";
      break;
  }
  return(str);
}

/*!
* \return	Non zero value if the parser fails.
* \ingroup	WlzIIPServer
* \brief	Parses the given string for value mapping parameters setting
*		the map values.
* 		The map string must have the form:
* 		  \verbatim MAP=mspec[,mspec[,mspec,mspec]]] \endverbatim
*		There may be 1, 2, 3, or 4 map specifications used with a
*		map command. A single map specification means a simple
*		grey value mapping, two specifications mean grey and alpha,
*		three mean RGB and four RGB\f$\alpha\f$.
*		with
*		  \verbatim mspec= t,il,iu,ol,ou[,p0[,p1]] \endverbatim
* 		and the map specification parameters are:
*	  	<ul>
*	  	  <li> \verbatim t \endverbatim The mapping function:
*	  	    <ul>
*	  	      <li> \verbatim IDENTITY \endverbatim
*	  	      <li> \verbatim LINEAR \endverbatim
*	  	      <li> \verbatim GAMMA \endverbatim
*	  	      <li> \verbatim SIGMOID \endverbatim
*  	  	    </ul>
*	  	  <li> \verbatim il \endverbatim Input lower grey value.
*	  	  <li> \verbatim iu \endverbatim Input upper grey value.
*	          <li> \verbatim ol \endverbatim Output lower grey value.
*	          <li> \verbatim ou \endverbatim Output upper grey value.
*	          <li> \verbatim p0 \endverbatim Gamma (\f$\gamma\f$) or
*	                                         sigmoid (\f$\mu\f$).
*	          <li> \verbatim p1 \endverbatim Sigmoid (\f$\sigma\f$).
*  	  	</ul>
* \param	argStr			Given string.
*/
int
ImageMap::parse(const char *argStr)
{
  
  int		i = 0,
  		err = 1;
  char		*sav,
  		*str;
  char		*pS[7];
  int		pI[5];
  double	pD[2];

  str = AlcStrDup(argStr);
  if(str)
  {
    (void )WlzStringWhiteSpSkip(str);
    (void )WlzStringToUpper(str);
  }
  if(str && ((pS[0] = strtok_r(str, ",", &sav)) != NULL))
  {
    do
    {
      if((i > 3) ||
	 (WlzStringMatchValue(&(pI[0]), pS[0],
			      "IDENTITY", WLZ_GREYTRANSFORMTYPE_IDENTITY,
			      "LINEAR",   WLZ_GREYTRANSFORMTYPE_LINEAR,
			      "GAMMA",    WLZ_GREYTRANSFORMTYPE_GAMMA,
			      "SIGMOID",  WLZ_GREYTRANSFORMTYPE_SIGMOID,
			      NULL) == 0))
      {
        err = 1;
      }
      else
      {
	pD[0] = pD[1] = 0.0;
	switch(pI[0])
	{
	  case WLZ_GREYTRANSFORMTYPE_IDENTITY: /* FALLTHROUGH */
	  case WLZ_GREYTRANSFORMTYPE_LINEAR:   /* FALLTHROUGH */
	  case WLZ_GREYTRANSFORMTYPE_GAMMA:    /* FALLTHROUGH */
	  case WLZ_GREYTRANSFORMTYPE_SIGMOID:
            err = 0;
	    break;
	  default:
	    err = 1;
	    break;
	}
        if(err == 0)
	{
	  if(((pS[1] = strtok_r(NULL, ",", &sav)) == NULL) ||
	     ((pS[2] = strtok_r(NULL, ",", &sav)) == NULL) ||
	     ((pS[3] = strtok_r(NULL, ",", &sav)) == NULL) ||
	     ((pS[4] = strtok_r(NULL, ",", &sav)) == NULL) ||
	     (sscanf(pS[1], "%d", &(pI[1])) != 1) ||
	     (sscanf(pS[2], "%d", &(pI[2])) != 1) ||
	     (sscanf(pS[3], "%d", &(pI[3])) != 1) ||
	     (sscanf(pS[4], "%d", &(pI[4])) != 1) ||
	     (pI[3] < 0) || (pI[3] > 255) ||
	     (pI[4] < 0) || (pI[4] > 255))
	  {
	    err = 1;
	  }
	  else if((pI[0] == WLZ_GREYTRANSFORMTYPE_GAMMA) ||
		  (pI[0] == WLZ_GREYTRANSFORMTYPE_SIGMOID))
	  {
	    if(((pS[5] = strtok_r(NULL, ",", &sav)) == NULL) ||
	       (sscanf(pS[5], "%lg", &(pD[0])) != 1))
	    {
	      err = 1;
	    }
	    else if(pI[0] == WLZ_GREYTRANSFORMTYPE_SIGMOID)
	    {
	      if(((pS[6] = strtok_r(NULL, ",", &sav)) == NULL) ||
		 (sscanf(pS[6], "%lg", &(pD[1])) != 1))
	      {
		err = 1;
	      }
	    }
	  }
	}
        if(err == 0)
	{
	  this->chan[i].type = (WlzGreyTransformType )pI[0];
	  this->chan[i].il = pI[1];
	  this->chan[i].iu = pI[2];
	  this->chan[i].ol = pI[3];
	  this->chan[i].ou = pI[4];
	  this->chan[i].p0 = pD[0];
	  this->chan[i].p1 = pD[1];
	  ++i;
	  pS[0] = strtok_r(NULL, ",", &sav);
	}
      }
    } while(pS[0] && !err);
    err = (err || (i < 1) || (i > 4));
    this->nChan = (err)? 0: i;
  }
  AlcFree(str);
  return(err);
}

/*!
* \return	String representation of the given map.
* \ingroup	WlzIIPServer
* \brief	Creates a string representation of the map although
* 		with decimal code(s) (from enum values) for the mapping
* 		function(s). This is intended for use in generating hash
*		keys etc.
*/
std::string
ImageMap::toString()
const
{
  int 		totLen,
  		maxLen;
  char		*buf;

  maxLen = 256 * ((nChan > 0)? nChan: 1);
  if((buf = (char *)AlcMalloc(maxLen * sizeof(char))) != NULL)
  {
    char *pos = buf;
    int totLen = sprintf(buf, "MAP=%d", nChan);
    for(int i = 0; i < nChan; ++i)
    {
      int 	len;
      pos = buf + totLen;
      len = snprintf(pos, 256, ",%d,%d,%d,%d,%d,%g,%g",
		     chan[i].type, chan[i].il, chan[i].iu,
		     chan[i].ol, chan[i].ou,
		     chan[i].p0, chan[i].p1);
      if(len > 256)
      {
	len = 256;
      }
      totLen += len;
    }
    buf[maxLen - 1] = '\0';
  }
  std::string str(buf);
  AlcFree(buf);
  return(str);
}

/*!
* \return	Look up table object.
* \ingroup	WlzIIPServer
* \brief	Creates a value map look up table object for the image map.
* \param	dstErr			Destination error pointer, may be NULL.
*/
WlzObject
*ImageMap::createLUT(WlzErrorNum *dstErr)
const
{
  WlzObject	*lutObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if((nChan < 1) || (nChan > 4))
  {
    errNum = WLZ_ERR_PARAM_DATA;
  }
  // Create map string(s) and get LUT object(s) trying the cache first
  if(errNum == WLZ_ERR_NONE)
  {
    WlzObject	*lObj[4];

    lObj[0] = lObj[1] = lObj[2] = lObj[3] = NULL;
    for(int i = 0; i < nChan; ++i)
    {
      WlzGreyV gol, gou;
      gol.inv = chan[i].ol;
      gou.inv = chan[i].ou;
      lObj[i] = WlzLUTGreyTransformNew(chan[i].type, WLZ_GREY_INT,
				       chan[i].il, chan[i].iu, gol, gou,
				       chan[i].p0, chan[i].p1, &errNum);
      if(errNum != WLZ_ERR_NONE)
      {
	break;
      }
    }
    if(errNum == WLZ_ERR_NONE)
    {
      switch(nChan)
      {
	case 1:
	  lutObj = lObj[0];
	  lObj[0] = NULL;
	  break;
	case 2:
	  lutObj = WlzLUTMergeToRGBA(lObj[0], lObj[0], lObj[0], lObj[1],
				     &errNum);
	  break;
	case 3:
	  lutObj = WlzLUTMergeToRGBA(lObj[0], lObj[1], lObj[2], NULL,
				     &errNum);
	  break;
	case 4:
	  lutObj = WlzLUTMergeToRGBA(lObj[0], lObj[1], lObj[2], lObj[3],
				     &errNum);
	  break;
	default:
	  break;
      }
    }
    for(int i = 0; i < 4; ++i)
    {
      (void )WlzFreeObj(lObj[i]);
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(lutObj);
}
