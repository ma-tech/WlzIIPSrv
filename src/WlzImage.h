#ifndef _WLZIMAGE_H
#define _WLZIMAGE_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzImage_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzImage.h
* \author       Zsolt Husz, Bill Hill
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
* \brief	The WlzImage class to handle 3D Woolz objects sectioning
* 		and expression evaluation for the Woolz IIP server.
* \ingroup	WlzIIPServer
*/
#include "IIPImage.h"
#include <Wlz.h>

#include "ViewParameters.h"

#include "WlzViewStructCache.h"
#include "WlzObjectCache.h"


/*! 
* \brief	Provides the WlzImage classs. Implementation is based on
*               TPTImage class (Tiled Pyramidal TIFF), part of the IIP
*               Server
* \ingroup	WlzIIPServer
* \todo         -
* \bug          None known.
*/
class WlzImage : public IIPImage
{
  protected:
    WlzObject		*wlzObject;         /*!< Current object. */
    WlzThreeDViewStruct *wlzViewStr;        /*!< Current view structure. */
    ViewParameters      *curViewParams;     /*!< Current view parameters,
                                                 partly redundant with
						 wlzViewStr, however used to 
						 minimise compuations. */
    const ViewParameters *viewParams;       /*!< View parameters set by the
    						 user. These might not be
						 reflected yet in wlzViewStr. */
    static WlzObjectCache wlzObjectCache;   /*!< Woolz object cache*/
    WlzUByte	   	*tile_buf;          /*!< Tile data buffer */
    int                 number_of_tiles;    /*!< Number of tiles */
    static const WlzInterpolationType interp ; /*!< Type of interpollation */
    int         	lastTileWidth;      /*!< Width for last column tiles */
    int                 lastTileHeight;     /*!< Height for last row tiles */
    int                 ntlx;               /*!< Number of tiles per row */
    int                 ntly;               /*!< Number of tiles per columns */
    WlzUByte	        background[4];      /*!< Background value */

  public:
    // Constructors and destructor
    WlzImage();
    WlzImage(const WlzImage& image);
    WlzImage(const std::string& path);
    /*!
    * \ingroup      WlzIIPServer
    * \brief        Destructor
    */
    ~WlzImage()
    {
	closeImage();
    };
    // Overloaded IIPImage methods
    void			openImage();
    void			loadImageInfo(
    				  int x,
				  int y )
				throw(std::string);
    void 			closeImage();
    RawTile 			getTile(
    				  int x,
				  int y,
				  unsigned int r,
				  unsigned int t)
      	        		throw(std::string);
    string			getFileName();
    const std::string 		getHash();
    // Woolz operations
    void			prepareObject()
    				throw(std::string);
    void			prepareViewStruct()
    				throw(std::string);
    bool			isViewChanged();
    WlzObject			*WlzImageExpEval(
    				  WlzExp *e);
    // Utility functions
    float 			*getTrueVoxelSize()
    				throw(std::string);
    WlzLong 			getVolume()
    				throw(std::string);
    int 			getGreyValue(
    				  int *points);
    void			getGreyStats(
    				  int &n,
				  WlzGreyType &t,
				  double &gl,
				  double &gu,
			          double &sum,
				  double &ss,
				  double &mean,
			          double &sdev)
				throw(std::string);
    void 			getDepthRange(
    				  double &min,
				  double &max);
    void 			getAngles(
    				  double &theta,
				  double &phi,
				  double &zeta);
    void 			get3DBoundingBox(
    				  WlzIBox3 &box)
				throw(std::string);
    void 			getTransformed3DBBox(
    				  WlzIBox3 &box)
				throw(std::string);
    WlzDVertex3 		getCurrentPointIn3D();
    WlzDVertex3 		getTransformed3DPoint();
    int 			getForegroundObjects(
    				  int *values);
    int 			getCompoundNo();

    /*!
    * \return   Woolz object (with incremented linkcount) if the obejct
    *           matching the string was in the cache, otherwise NULL.
    * \ingroup 	WlzIIPServer
    * \brief	Attempts to get a Woolz object from the cache which matches
    *		the given string.
    * \param	ois			Woolz object identification string.
    */
    WlzObject   		*getObjectFromCache(
    				  std::string ois)
    {
      WlzObject     *obj;

      obj  = WlzAssignObject(wlzObjectCache.get(ois), NULL);
      return(obj);
    }

    /*!
    * \ingroup 	WlzIIPServer
    * \brief	Attempts to add the given Woolz object to the cache using
    *		the given string.
    * \param	obj			Woolz object to add to the cache.
    * 		ois			Woolz object identification string.
    */
    void            		addObjectToCache(
    				  WlzObject *obj,
				  string ois)
    {
      wlzObjectCache.insert(obj , ois);
    }

    /*!
    * \ingroup 	WlzIIPServer
    * \brief	Creates an error message string. Handy for thowing
    * 		exceptions.
    * \param	first			First part of string.
    * \param	errNum			Woolz error number.
    */
    static std::string		makeWlzErrorMessage(
    				  std::string first,
    				  WlzErrorNum errNum)
    {
      const char    *errStr,
	    	    *msgStr;
      stringstream  msg; 

      errStr = WlzStringFromErrorNum(errNum, &msgStr);
      msg <<first << " " << msgStr << " (" << errStr << ").";
      return(msg.str());
    }

    /*!
    * \ingroup 	WlzIIPServer
    * \brief	Creates an error message string. Handy for thowing
    * 		exceptions.
    * \param	first			First part of string.
    * \param	errNum			Woolz error number.
    * \param	last			Last part of string.
    */
    static std::string		makeWlzErrorMessage(
    				  std::string first,
    				  WlzErrorNum errNum,
				  std::string last)
    {
      const char    *errStr,
	    	    *msgStr;
      stringstream  msg; 

      errStr = WlzStringFromErrorNum(errNum, &msgStr);
      msg <<first << " " << msgStr << " (" << errStr << "). " << last;
      return(msg.str());
    }

    /*!
    * \ingroup  WlzIIPServer
    * \brief    Sets pointer to task parameters
    * \param   	viewP 			Pointer to view parameters
    */
    void 			setView(
    				  const ViewParameters  *viewP)
    {
      viewParams = viewP;
    };

    /*!
    * \ingroup  WlzIIPServer
    * \brief    Forces channel no update to alpha value.
    * \param  	alpha 		New alpha value
    */
    virtual void 		recomputeChannel(
    				  bool alpha)
    {
      if((channels == 3) || (channels == 4))
      {
	channels = alpha? (unsigned int)4: (unsigned int)3;
      }
      else if((channels == 1 || channels == 2))
      {
	channels = alpha? (unsigned int)2: (unsigned int)1;
      }
    };

    // Internal functions
    protected:
    WlzErrorNum 		convertObjToRGB(
    				  WlzUByte * cbuf,
				  WlzObject* obj,
                                  WlzIVertex2  pos,
				  WlzIVertex2  size);
    WlzErrorNum 		convertDomainObjToRGB(
    				  WlzUByte *cbuf,
				  WlzObject* obj,
                                  WlzIVertex2  pos,
				  WlzIVertex2  size,
				  CompoundSelector *sel);
    WlzErrorNum 		convertValueObjToRGB(
    				  WlzUByte *cbuf,
				  WlzObject *obj,
				  WlzIVertex2  pos,
				  WlzIVertex2  size,
				  CompoundSelector *sel);
    WlzErrorNum 		renderObj(
    				  WlzUByte* tile_buf,
				  WlzObject *wlzObject,
                                  WlzObject *tileObject,
				  WlzIVertex2  pos,
			          WlzIVertex2  size,
				  CompoundSelector *sel);
    WlzObject			*getSubProjFromObject(
    				  WlzObject *wlzObject,
				  WlzObject *tileObject,
				  CompoundSelector *sel,
				  WlzErrorNum *dstErr);
    WlzObject 			*getMapLUTObj(
    				  WlzErrorNum *dstErr);
    WlzObject 			*mapValueObj(
    				  WlzObject *iObj,
				  WlzObject *lutObj,
    			   	  WlzErrorNum *dstErr);
    WlzDVertex3 		getCurrentPointInPlane();
    const std::string 		generateHash(const ViewParameters *view);
    const std::string 		selString(const ViewParameters* view );

    /*!
     * \ingroup WlzIIPServer
     * \brief   Return the number of channels for the output
     */
    unsigned int 		getNumChannels()
    {
      // Forces RGB/RGBA output if a selector is specified
      return((viewParams->selector)? ((viewParams->alpha)? 4: 3): channels);
    };

    /*!
     * \return	Returns the current compound array object or NULL if the
     * 		current object is not a compound array.
     * \ingroup	WlzIIPServer
     * \brief	Acesses the current compound array object.
     */
    WlzCompoundArray 		*getArray()
    {
      WlzCompoundArray *array = NULL;

      if(wlzObject)
      {
	if(wlzObject->type == WLZ_COMPOUND_ARR_2)
	{
	  array = (WlzCompoundArray *)wlzObject;
	}
      }
      return(array);
    }

    /*!
     * \return	Returns the current object.
     * \ingroup	WlzIIPServer
     * \brief	Acesses the current object or if it is a compound array
     * 		the first object of the compound array.
     */
    WlzObject			*getObj()
    {
      WlzObject *obj;

      if(wlzObject)
      {
	if(wlzObject->type == WLZ_COMPOUND_ARR_2)
	{
	  WlzCompoundArray *array = (WlzCompoundArray *)wlzObject;
	  if(array->n > 0)
	  {
	    obj = array->o[0];
	  }
	}
	else
	{
	  obj = wlzObject;
	}
      }
      return(obj);
    }
};

#endif
