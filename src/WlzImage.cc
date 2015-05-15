#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzImage_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzImage.cc
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

#include "Log.h"
#include "WlzImage.h"
#include <WlzProto.h>
#include <WlzExtFF.h>
#include "Environment.h"

//#define __PERFORMANCE_DEBUG
#ifdef __PERFORMANCE_DEBUG
#include <sys/time.h>
#endif

//#define __ALLOW_REMOTE_FILE
#ifdef __ALLOW_REMOTE_FILE
#include "WlzRemoteImage.h"
#endif

using namespace std;

/*!Interpolation type */
const WlzInterpolationType WlzImage::interp = WLZ_INTERPOLATION_NEAREST;

/*!
 * Woolz object cache. Static for all queries. 
 */
WlzObjectCache            WlzImage::wlzObjectCache;


/*!
 * \ingroup      WlzIIPServer
 * \brief        Default constructor
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
WlzImage::WlzImage(): IIPImage() { 
  wlzObject         = NULL;
  tile_buf          = NULL;
  tile_width        = 0;
  tile_height       = 0;
  numResolutions    = 0;
  viewParams        = NULL;
  wlzViewStr        = NULL;
  curViewParams     = NULL;
  number_of_tiles   = 0;
  lastTileWidth     = 0;
  lastTileHeight    = 0;
  ntlx              = 0;
  ntly              = 0; 
  
  tile_height       = Environment::getWlzTileHeight();
  tile_width        = Environment::getWlzTileWidth();
  
};

/*!
 * \ingroup      WlzIIPServer
 * \brief        Constructor with a path
 * \param        path image path
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
WlzImage::WlzImage( const std::string& path): IIPImage( path ) {
  wlzObject         = NULL;
  tile_buf          = NULL;
  tile_width        = 0;
  tile_height       = 0;
  numResolutions    = 0;
  viewParams        = NULL;
  wlzViewStr        = NULL;
  curViewParams     = NULL;
  number_of_tiles   = 0;
  lastTileWidth     = 0;
  lastTileHeight    = 0;
  ntlx              = 0;
  ntly              = 0;
  
  tile_height       = Environment::getWlzTileHeight();
  tile_width        = Environment::getWlzTileWidth();
};

/*!
 * \ingroup      WlzIIPServer
 * \brief        Copy constructor
 * \param        image WlzImage object
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
WlzImage:: WlzImage( const WlzImage& image ): IIPImage( image ) {
  
  wlzObject = WlzAssignObject(image.wlzObject , NULL);
  
  tile_buf          = NULL;
  tile_width        = image.tile_width; 
  tile_height       = image.tile_height;
  numResolutions    = image.numResolutions;
  viewParams        = image.viewParams;
  wlzViewStr        = image.wlzViewStr;
  number_of_tiles   = image.number_of_tiles;
  lastTileWidth     = image.lastTileWidth; 
  lastTileHeight    = image.lastTileHeight;
  ntlx              = image.ntlx;
  ntly              = image.ntly; 
  tile_height       = image.tile_height;
  tile_width        = image.tile_width;
  
  if (image.curViewParams != NULL){
    curViewParams   = new ViewParameters;
    *curViewParams  = *image.curViewParams;
  }
  else {
    curViewParams   = NULL;
  }
};

/*!
 * \ingroup      WlzIIPServer
 * \brief        Open a new image and loads it metadata.
 *               Not directly used, but kept for compatibility with IIPImage.
 *
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::openImage()
{
  // if already opened do not reopen
  if( wlzObject || tile_buf ){
    return;
  }
  loadImageInfo( 0, 0 );
  isSet = true;
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Update the view structure used to generate sections either
 * 		 from the view structure cache or by recomputing it.
 *
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::prepareViewStruct()
throw(string)
{
  WlzErrorNum errNum = WLZ_ERR_NONE;
  
  if (!isViewChanged())
  {
    return;
  }
  if (!viewParams)
  {
    errNum = WLZ_ERR_PARAM_NULL;
    throw(
    makeWlzErrorMessage("WlzImage::prepareViewStruct() viewParams NULL.",
                        errNum));
  }
  prepareObject();  //make sure object is loaded
  //generate cache hash
  string hash = generateHash(viewParams);
  LOG_DEBUG("WlzImage::prepareViewStruct() hash:" << hash);
  if(wlzViewStr != NULL)
  {
    WlzFree3DViewStruct(wlzViewStr);
  }
  wlzViewStr = WlzAssign3DViewStruct(wlzObjectCache.getVS(hash), NULL);
  if (wlzViewStr == NULL)  // cache miss?
  {
    
    if((wlzViewStr = WlzAssign3DViewStruct(
                     WlzMake3DViewStruct(WLZ_3D_VIEW_STRUCT, &errNum),
		                          NULL )) != NULL)
    {
      wlzViewStr->theta           = viewParams->yaw   * WLZ_M_PI / 180.0;
      wlzViewStr->phi             = viewParams->pitch * WLZ_M_PI / 180.0;
      wlzViewStr->zeta            = viewParams->roll  * WLZ_M_PI / 180.0;
      wlzViewStr->dist            = viewParams->dist;
      wlzViewStr->fixed           = viewParams->fixed;
      wlzViewStr->fixed_2         = viewParams->fixed2;
      wlzViewStr->up              = viewParams->up;
      wlzViewStr->view_mode       = viewParams->mode;
      wlzViewStr->scale           = viewParams->scale;
      wlzViewStr->voxelRescaleFlg = 0x01 | 0x02;  // Set if voxel size
                                                  // correciton is needed.
    }
    else
    {
      throw(
      makeWlzErrorMessage("WlzImage::prepareViewStruct() creation failed.",
                          errNum));
    }
    if(wlzObject->type == WLZ_COMPOUND_ARR_2)
    {
      WlzCompoundArray *array = (WlzCompoundArray *)wlzObject;
      WlzObject *obj = array->o[0];
      if(array && array->n>0 && obj)
      {
	if(obj->domain.p)
	{
	  wlzViewStr->voxelSize[0]    = obj->domain.p->voxel_size[0];
	  wlzViewStr->voxelSize[1]    = obj->domain.p->voxel_size[1];
	  wlzViewStr->voxelSize[2]    = obj->domain.p->voxel_size[2];
	}
	errNum = WlzInit3DViewStruct(wlzViewStr, array->o[0]);
      }
      else
      {
	throw(
	makeWlzErrorMessage(
	  "WlzImage::prepareViewStruct() can't find object to prepare "
	  "ViewStruct.", errNum));
      }
    }
    else
    {
      if(wlzObject && wlzObject->domain.p)
      {
	wlzViewStr->voxelSize[0]    = wlzObject->domain.p->voxel_size[0];
	wlzViewStr->voxelSize[1]    = wlzObject->domain.p->voxel_size[1];
	wlzViewStr->voxelSize[2]    = wlzObject->domain.p->voxel_size[2];
      }
      errNum = WlzInit3DViewStruct(wlzViewStr, wlzObject);
    }
    if(errNum != WLZ_ERR_NONE)
    {
      throw(
      makeWlzErrorMessage(
        "WlzImage::prepareViewStruct() failed.", errNum));
    }
    wlzObjectCache.insert(wlzViewStr , hash);
  }
  return;
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Prepare the 3D Woolz object either by looking it up from the
 * 		 object cache or by reading it from disk
 *
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::prepareObject()
throw(string)
{
  WlzGreyType   gType;
  WlzErrorNum   errNum = WLZ_ERR_NONE;
  
  if(!wlzObject)
  {
    string filename;
    int usepipe = 0;
    LOG_DEBUG("WlzImage::prepareObject() reloading");
    //check cache first
    filename = getFileName( );
    wlzObject  = WlzAssignObject(wlzObjectCache.get(filename), NULL);
#ifdef __PERFORMANCE_DEBUG
    struct timeval tVal;
    struct timeval tVal2;
    int time = 0;
    gettimeofday(&tVal, NULL);
#endif
    LOG_DEBUG("WlzImage::prepareObject() cache " <<
              (wlzObject != NULL)? "hit": "miss");
    if (wlzObject == NULL)  // cache miss?
    {
      // if not in cache then load
      FILE *fp = NULL;
      if (filename.substr(filename.length()-3, 3) == ".gz") {
	string command = "gunzip -c ";
	command += filename;
	fp = popen( command.c_str(), "r");
	usepipe = 1;
      } else
	fp = fopen(filename.c_str(), "r");
      
      if (fp)
	wlzObject = WlzEffReadObj( fp , NULL, WLZEFF_FORMAT_WLZ,
	                           0, 0, 0, &errNum );
      
      if (fp) {
	if (usepipe)
	  pclose(fp);
	else
	  fclose(fp);
      }
      
#ifdef __ALLOW_REMOTE_FILE
      /////???????????? Yiya added for remote wlz file
	// no local wlzObject
	if (NULL == wlzObject) {
	  char* remoteFile = new char[strlen(filename.c_str()) + 1];
	  strcpy(remoteFile, filename.c_str());

	  wlzObject = WlzRemoteImage::wlzRemoteReadObj((const char*)remoteFile, (const char*)NULL, -1);

	  if (NULL == wlzObject) 
	  {
	    errNum = WLZ_ERR_OBJECT_NULL;
	    throw(
	    makeWlzErrorMessage(
	      "WlzImage::prepareObject() reading remote file failed.",
	      errNum));
	  }
	  delete[] remoteFile;
	  remoteFile = NULL;
	}
	/////???????????? Yiya added for remote wlz file
#endif
	  
	  if(NULL == wlzObject) 
	  {
	    errNum = WLZ_ERR_OBJECT_NULL;
	    throw(
	    makeWlzErrorMessage(
	      "WlzImage::prepareObject() no such file.",
	       errNum));
	  }
	  //test object type
	  switch (wlzObject->type) {
	  case WLZ_3D_DOMAINOBJ:
	    break;
	  case WLZ_COMPOUND_ARR_2:
	    break;
	  default:
	    errNum = WLZ_ERR_OBJECT_TYPE;
	    throw(
	    makeWlzErrorMessage(
	      "WlzImage::prepareObject()  incorrect object type",
	      errNum,
	      "Currently supported are: "
	      "WLZ_3D_DOMAINOBJ, and WLZ_COMPOUND_ARR_2 with "
	      "WLZ_3D_DOMAINOBJ."));
	  }
	  wlzObject = WlzAssignObject( wlzObject, &errNum );
	  if(wlzObject == NULL || errNum != WLZ_ERR_NONE)
	  {
	    throw("WlzImage::prepareObject() failed to read object "
	          "from file " + filename + ".");
	  }
	  wlzObjectCache.insert(wlzObject , filename);
    }
#ifdef __PERFORMANCE_DEBUG
    gettimeofday(&tVal2, NULL);
    time = tVal2.tv_sec - tVal.tv_sec;
    if (0 < time)
    {
      printf("%s reading takes %d seconds\n", filename.c_str(), time);
    }
#endif
    
    isSet = false;   //yet object view specific data has to be set
  }
  else
  {
    LOG_DEBUG("WlzImage :: prepareObject: not reloaded, already initialised");
  }
  
  WlzCompoundArray *array = wlzObject->type==WLZ_COMPOUND_ARR_2 ?
                            (WlzCompoundArray *)wlzObject : NULL;
  WlzObject *initObj = array?  ( array->n>0 ? array->o[0] : NULL) : wlzObject;
  
  if (!initObj)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
    throw(
    makeWlzErrorMessage("WlzImage::prepareObject() unsuported object.",
                        errNum));
  }
  
  //set global parameters
  if (initObj->values.core == NULL) // no values
  {
    gType = WLZ_GREY_UBYTE; // consider it UBYTE
  }
  else
  {
    gType = WlzGreyTypeFromObj(initObj, &errNum);
  }
  if(errNum != WLZ_ERR_NONE)
  {
    throw(
    makeWlzErrorMessage(
      "WlzImage::prepareObject() error when determining grey type.",
      errNum));
  }
  switch( gType ){
  case WLZ_GREY_LONG:
  case WLZ_GREY_INT:
  case WLZ_GREY_SHORT:
  case WLZ_GREY_FLOAT:
  case WLZ_GREY_DOUBLE:
  case WLZ_GREY_UBYTE:
    channels = viewParams->alpha ? (unsigned int) 2 :(unsigned int) 1;
    break;
  case WLZ_GREY_RGBA:
    channels = viewParams->alpha ? (unsigned int) 4 :(unsigned int) 3;
    break;
  default:
    errNum = WLZ_ERR_GREY_TYPE;
    throw(
    makeWlzErrorMessage(
      "WlzImage::prepareObject()  unsuported grey type.",
      errNum,
      "Currently supported are: "
      "WLZ_GREY_LONG, WLZ_GREY_INT, WLZ_GREY_SHORT, "
      "WLZ_GREY_UBYTE, WLZ_GREY_FLOAT, WLZ_GREY_DOUBLE and "
      "WLZ_GREY_RGBA."));
  }
  
  bpp = (unsigned int) 8;  //bits per channel
  
  // Assign the colourspace variable
  if( channels == 8 ) colourspace = CIELAB;
  else if( channels == 1 ) colourspace = GREYSCALE;
  else colourspace = sRGB;
  
  background[0]=0;    // black background & transparent alpha
  background[1]=0;
  background[2]=0;
  background[3]=0;
  
  
  //set background
  if (initObj && initObj->values.c != NULL)
  {
    WlzPixelV pixel=WlzGetBackground(initObj, NULL);
    background[1]=0;  // alpha 0, transparent
    switch( gType ){
    case WLZ_GREY_LONG:
      background[0] = pixel.v.lnv%255;
      break;
    case WLZ_GREY_INT:
      background[0] = pixel.v.inv%255;
      break;
    case WLZ_GREY_SHORT:
      background[0] = pixel.v.shv%255;
      break;
    case WLZ_GREY_FLOAT:
      background[0] = ((int)round(pixel.v.flv))%255;
      break;
    case WLZ_GREY_DOUBLE:
      background[0] = ((int)round(pixel.v.dbv))%255;
      break;
    case WLZ_GREY_UBYTE:
      background[0] = pixel.v.ubv;
      break;
    case WLZ_GREY_RGBA:
      background[0] = WLZ_RGBA_RED_GET(pixel.v.rgbv);
      background[1] = WLZ_RGBA_GREEN_GET(pixel.v.rgbv);
      background[2] = WLZ_RGBA_BLUE_GET(pixel.v.rgbv);
      background[3] = WLZ_RGBA_ALPHA_GET(pixel.v.rgbv);
      break;
    default:
      throw(
      makeWlzErrorMessage(
        "WlzImage::prepareObject() when setting background.",
	errNum,
	"Currently supported are: "
	"WLZ_GREY_LONG, WLZ_GREY_INT, WLZ_GREY_SHORT, "
	"WLZ_GREY_UBYTE, WLZ_GREY_FLOAT, WLZ_GREY_DOUBLE and "
	"WLZ_GREY_RGBA."));
    }
  }
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Checks if current view structure of WlzImage is up to date
 * 		 with the parameters passed to the server
 *
 * \return       true if up to date, false if reload required
 * \par      Source:
 *                WlzImage.cc
 */
bool WlzImage::isViewChanged() 
{
  return !viewParams || !curViewParams || (*viewParams!=*curViewParams);
}


/*!
 * \ingroup      WlzIIPServer
 * \brief        Load object specific info: image and tile size, number of
 * 		 tiles, metadata. 
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 * \par           Pramaters are not used, required by the base class definition.
 */
void WlzImage::loadImageInfo( int , int )
throw(string)
{
  
  WlzErrorNum errNum = WLZ_ERR_NONE;
  
  if(isViewChanged())
  {
    prepareObject();
    prepareViewStruct();
    image_width  = (unsigned int)(wlzViewStr->maxvals.vtX-wlzViewStr->minvals.vtX) + 1;
    image_height = (unsigned int)(wlzViewStr->maxvals.vtY-wlzViewStr->minvals.vtY) + 1;
    
    LOG_INFO("WlzImage::loadImageInfo() image size : "<<
	     image_width << " x " << image_height);
    
    // Get the width and height for last row and column tiles
    lastTileWidth = image_width % tile_width;
    lastTileHeight = image_height % tile_height;
    
    // Calculate the number of tiles in each direction
    ntlx = (image_width / tile_width) + (lastTileWidth== 0 ? 0 : 1);
    ntly = (image_height / tile_height) + (lastTileHeight == 0 ? 0 : 1);
    
    number_of_tiles = ntlx * ntly;
    
    LOG_DEBUG("WlzImage::loadImageInfo() number of tiles: " <<
              number_of_tiles);
    
    
    numResolutions = 1; // resoltions. currently only one resolution is used.
    
    // Update current sections view status
    if (curViewParams == NULL)
      { 
	curViewParams = new ViewParameters;
      }
    *curViewParams = *viewParams;
    
    // No metadata is stored in the Woolz object, therefer set them to unknown
    metadata["author"] = "author unknown";
    metadata["copyright"] = "copyright unknown";
    metadata["create-dtm"] = "create-dtm unknown";
    metadata["subject"] = "subject unknown";
    metadata["app-name"] = "app-name unknown";
    
  }
  LOG_INFO("WlzImage::loadImageInfo() done");
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Release the object and all related data.
 *
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::closeImage() {
  // free tile temporary data
  if( tile_buf != NULL ){
    free(tile_buf);
    tile_buf = NULL;
  }
  
  // release view
  if( wlzViewStr != NULL ){
    WlzFree3DViewStruct( wlzViewStr ); 
    wlzViewStr  = NULL;
  }
  
  // release object 
  if( wlzObject != NULL ){
    WlzFreeObj( wlzObject );
    wlzObject = NULL;
  }
  
  // release current object parameters
  if (curViewParams != NULL){
    delete curViewParams;
    curViewParams = NULL;
  }
}

/*!
* \return       Woolz error code.
* \ingroup      WlzIIPServer
* \brief	Renders a Woolz object by either sectioning or projecting
* 		the given 3D object to generate a single tile in tile_buf
* 		for the tileing given by tileObj.
* \param        tileBuf   allocated memmory location for the tile
* \param        gvnObj	   The given 3D woolz object to render.
* \param        tileObj    Given tile object set up for the requested tile.
* \param        pos        Section bounding box origin.
* \param        size       Section bounding box size.
* \param        sel        Selector with the colour to be used for the section.
*/
WlzErrorNum			WlzImage::renderObj(
				  WlzUByte *tileBuf,
				  WlzObject *gvnObj,
                    		  WlzObject *tileObj,
				  WlzIVertex2  pos,
		    		  WlzIVertex2 size,
				  CompoundSelector *sel)
{
  WlzObject 	*renObj = NULL;
  WlzErrorNum 	errNum = WLZ_ERR_NONE;
  const int	dither = 0;

  // Render the object for the given tile domain.
  switch(viewParams->rmd)
  {
    case RENDERMODE_SECT:
      renObj = WlzAssignObject(
	       WlzGetSubSectionFromObject(gvnObj, tileObj, wlzViewStr, interp,
					  NULL, &errNum), NULL);
      break;
    case RENDERMODE_PROJ_N: // FALLTHROUGH
    case RENDERMODE_PROJ_D: // FALLTHROUGH
    case RENDERMODE_PROJ_V:
      renObj = WlzAssignObject(
	       getSubProjFromObject(gvnObj, tileObj, sel, &errNum), NULL);
      break;
    default:
      errNum = WLZ_ERR_PARAM_DATA;
      break;
  }
  if(renObj == NULL || errNum != WLZ_ERR_NONE)
  {
    throw(
    makeWlzErrorMessage(
      "WlzImage::renderObj() sectioning failed.",
      errNum));
  }
  // Set tile buffer values using the rendered object.
  if(renObj->values.core == NULL)
  {
    errNum = convertDomainObjToRGB(tileBuf, renObj,  pos, size, sel);
  }
  else
  {
    int nChan = viewParams->map.getNChan();
    if(nChan > 0)
    {
      WlzObject *lutObj = NULL,
      		*mapObj = NULL;

      lutObj = getMapLUTObj(&errNum);
      if(errNum == WLZ_ERR_NONE)
      {
        WlzGreyType gType,
		    mGType;

	gType = WlzGreyTypeFromObj(renObj, NULL);
	mGType = ((nChan > 1) || (gType == WLZ_GREY_RGBA))?
	         WLZ_GREY_RGBA: WLZ_GREY_UBYTE;
        mapObj = WlzAssignObject(
	         WlzLUTTransformObj(renObj, lutObj, mGType,
	                            0, dither, &errNum), NULL);
      }
      (void )WlzFreeObj(lutObj);
      if(errNum == WLZ_ERR_NONE) {
        errNum = convertValueObjToRGB(tileBuf, mapObj, pos, size, sel);
      }
      (void )WlzFreeObj(mapObj);
    }
    else
    {
      errNum = convertValueObjToRGB(tileBuf, renObj, pos, size, sel);
    }
  }
  if(errNum != WLZ_ERR_NONE)
  {
    throw(
    makeWlzErrorMessage(
      "WlzImage::renderObj() conversion to array.", errNum));
  }
  WlzFreeObj(renObj);
  return(errNum);
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Gets the projection of the given object which falls
* 		within the given tile's domain.
* \param	gvnObj			Given object to be projected.
* \param	tileObj			Object with required til domain.
* \param	sel			The selector (required for cache
* 					string).
* \param	dstErr			Destination error pointer, may be NULL.
*/
WlzObject 			*WlzImage::getSubProjFromObject(
				  WlzObject *gvnObj,
				  WlzObject *tileObj,
				  CompoundSelector *sel,
				  WlzErrorNum *dstErr)
{
  std::string 	pS = "S";
  std::string   prjS;
  WlzObject	*prjObj = NULL,
  		*subObj = NULL;
  WlzProjectIntMode itm = WLZ_PROJECT_INT_MODE_NONE;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  switch(viewParams->rmd)
  {
    case RENDERMODE_PROJ_N:
      pS = "N";
      itm = WLZ_PROJECT_INT_MODE_NONE;
      break;
    case RENDERMODE_PROJ_D:
      pS = "D";
      itm = WLZ_PROJECT_INT_MODE_DOMAIN;
      break;
    case RENDERMODE_PROJ_V:
      pS = "V";
      itm = WLZ_PROJECT_INT_MODE_VALUES;
      break;
    default:
      errNum = WLZ_ERR_PARAM_DATA;
      break;
  }
  prjS = "PRJ=" + pS + "," + getHash() +
         "SEL=" + WlzExpStr(sel->expression, NULL, NULL);;
  prjObj = getObjectFromCache(prjS);
  if(prjObj == NULL)
  {

    WlzObject   *t0 = NULL;

    if(errNum == WLZ_ERR_NONE)
    {
      t0 = WlzAssignObject(
	   WlzProjectObjToPlane(gvnObj, wlzViewStr, itm, 1, NULL,
	                        viewParams->depth, &errNum), NULL);
    }
    if(errNum == WLZ_ERR_NONE)
    {
      if(t0->values.core == NULL)
      {
        prjObj = WlzAssignObject(t0, NULL);
      }
      else
      {
	errNum = WlzGreyNormalise(t0, 0);
	if(errNum == WLZ_ERR_NONE)
	{
	  prjObj = WlzAssignObject(
		   WlzConvertPix(t0, WLZ_GREY_UBYTE, &errNum), NULL);
	}
      }
    }
    (void )WlzFreeObj(t0);
    if(errNum == WLZ_ERR_NONE)
    {
      addObjectToCache(prjObj, prjS);
    }
  }
  if(errNum == WLZ_ERR_NONE)
  {
    WlzObject *tmpObj = WlzIntersect2(tileObj, prjObj, &errNum);
    if(errNum == WLZ_ERR_NONE)
    {
      if(tmpObj->type == WLZ_EMPTY_OBJ)
      {
        subObj = WlzMakeEmpty(&errNum);
      }
      else
      {
	subObj = WlzMakeMain(tmpObj->type, tmpObj->domain, prjObj->values,
			     NULL, NULL, &errNum);
      }
    }
    (void )WlzFreeObj(tmpObj);
  }
  (void )WlzFreeObj(prjObj);
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(subObj);
}

/*!
* \return	Look up table object.
* \ingroup	WlzIIPServer
* \brief	Gets the value map look up table object using the object cache.
* \param	iObj			Given object which must have
* 					image values.
* \param	dstErr			Destination error pointer, may be NULL.
*/
WlzObject
*WlzImage::getMapLUTObj(WlzErrorNum *dstErr)
{
  WlzObject	*lutObj = NULL;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  const ImageMap &map = viewParams->map;
  if(map.getNChan() > 0)
  {
    const string mapS = map.toString();
    lutObj = getObjectFromCache(mapS);     // Increments the objects linkcount.
    if(lutObj == NULL)
    {
      lutObj = WlzAssignObject(map.createLUT(&errNum), NULL);
      if(errNum == WLZ_ERR_NONE)
      {
	addObjectToCache(lutObj, mapS);
      }
    }
  }
  if(dstErr)
  {
    *dstErr = errNum;
  }
  return(lutObj);
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Generate the current tile
 * \param        seq not used
 * \param        ang not used
 * \param        res requested resolution
 * \param        tile requested tile number
 * \return       RawTile raw tile data
 * \par      Source:
 *                WlzImage.cc
 */
RawTile		WlzImage::getTile(int seq, int ang, unsigned int res,
				  unsigned int tile)
throw(string)
{
  int 		tw=0, th=0; //real tile width and height
  WlzErrorNum 	errNum=WLZ_ERR_NONE;
  string 	filename;
  WlzObject     *tmpObj;
  WlzIVertex3   pos;
  WlzObject     *wlzSection = NULL;
  
  //seq = ang =  res  = 0;
  // force unused parameters to zero to, facilitate cache match
  loadImageInfo( 0, 0);
  
  // Check that a valid tile number was given
  if( tile >= number_of_tiles)
  {
    char tile_no[64];
    snprintf( tile_no, 64, "%d", tile );
    string tile_n = string( tile_no );
    throw("WlzImage::getTile() tile " + tile_n + " does not exist");
  }
  tw = tile_width;
  th = tile_height;
  // Alter the tile size if it's in the last column
  if((tile % ntlx == ntlx - 1) && (lastTileWidth != 0))
  {
    tw = lastTileWidth;
  }
  // Alter the tile size if it's in the bottom row
  if((tile / ntlx == ntly - 1) && (lastTileHeight != 0))
  {
    th = lastTileHeight;
  }
  if(errNum == WLZ_ERR_NONE)
  {
    /* Create maximum sized rectangular object */
    WlzDomain     domain;
    WlzValues     values;
    
    pos.vtX = (tile % ntlx) * tile_width +  WLZ_NINT(wlzViewStr->minvals.vtX);
    pos.vtY = (tile / ntlx) * tile_height + WLZ_NINT(wlzViewStr->minvals.vtY);
    if((domain.i = WlzMakeIntervalDomain(WLZ_INTERVALDOMAIN_RECT,
					 WLZ_NINT(pos.vtY),
					 WLZ_NINT(pos.vtY + th - 1),
					 WLZ_NINT(pos.vtX),
					 WLZ_NINT(pos.vtX + tw - 1),
					 &errNum)))
    {
      LOG_DEBUG("WlzImage::getTile() domain size " << pos.vtX <<"," <<
                pos.vtY << "," << tw << "," << th << " --- " <<
		wlzObject->domain.core->type);
      values.core = NULL;
      tmpObj = WlzAssignObject(WlzMakeMain(WLZ_2D_DOMAINOBJ, domain,
					   values, NULL, NULL, &errNum), NULL);
    }
  }
  WlzIBox3 gBufBox;
  gBufBox.xMin=pos.vtX;
  gBufBox.yMin=pos.vtY;
  gBufBox.xMax=pos.vtX + tw -1;
  gBufBox.yMax=pos.vtY + th -1;
  gBufBox.zMin=0;
  gBufBox.zMax=0;
  
  WlzIVertex2 pos2D;
  WlzIVertex2 size;
  pos2D.vtX = pos.vtX;
  pos2D.vtY = pos.vtY;
  size.vtX= tw;
  size.vtY= th;
  
  //recompute out channels
  int outchannels = getNumChannels();
  WlzCompoundArray *array = (wlzObject->type == WLZ_COMPOUND_ARR_2)?
                            (WlzCompoundArray* )wlzObject: NULL;
  if(!tile_buf)
  {
    tile_buf = (WlzUByte *)malloc(tile_width * tile_height * outchannels);
  }
  //init tile buffer
  for (int i = 0; i < size.vtX * size.vtY; i++)
  {
    memcpy(tile_buf + i * outchannels, background, outchannels);
  }
  if(viewParams->selector)
  {
    //if selector existis
    CompoundSelector *iter = viewParams->selector;
    while(iter)
    {
      if(array)
      {
	if(iter->expression)
	{
          WlzObject *obj= NULL;

          obj = WlzImageExpEval(iter->expression); // Assigns obj.
	  if(obj)
	  {
	    renderObj(tile_buf, obj, tmpObj, pos2D, size, iter);
	  }
	  (void )WlzFreeObj(obj);
	}
      }
      else
      {
	// use selector with lowest index
	renderObj(tile_buf, wlzObject, tmpObj, pos2D, size, iter);
	break;
      }
      iter = iter->next;
    }
  }
  else
  {
    //use default selector
    CompoundSelector sel;
    sel.a=255;
    sel.r=255;
    sel.g=255;
    sel.b=255;
    sel.expression = NULL;
    if(array)
    {
      if((array->n > 0) && array->o[0])
      {
	renderObj(tile_buf, array->o[0], tmpObj, pos2D, size, &sel);
      }
    }
    else
    {
      renderObj(tile_buf, wlzObject , tmpObj, pos2D, size, &sel);
    }
  }
  //free tileing object
  (void *)WlzFreeObj(tmpObj);
  
  LOG_DEBUG("WlzImage::getTile() raw creation");
  
  RawTile rawtile(tile, res, seq, ang, tw, th, outchannels, bpp);
  rawtile.data = tile_buf;
  rawtile.dataLength = tw * th * outchannels;
  rawtile.width_padding = tile_width - tw;
  //get hash of the tile
  rawtile.filename = getHash();
  return(rawtile);
}

/*!
* \return       Woolz object or NULL on error.
* \ingroup      WlzIIPServer
* \brief        Evaluates a morphological expression with reference to the
*               given object but first tries to retrieve it from the Woolz
*               object cache.
* \param        exp                     Morphological expression to be
*                                       evaluated using the current object.
*/
WlzObject      *WlzImage::WlzImageExpEval(WlzExp *exp)
{
  char          *eS;
  string   	cS;
  WlzObject	*cObj = NULL;

  eS = WlzExpStr(exp, NULL, NULL);
  if(eS)
  {
    cS = getFileName() + string("&SEL=") + string(eS);
    AlcFree(eS);
    cObj = getObjectFromCache(cS);
  }
  if(cObj == NULL)
  {
    cObj = WlzExpEval(wlzObject, exp, NULL);
    if(cObj)
    {
      WlzAssignObject(cObj, NULL);
      addObjectToCache(cObj, cS);
    }
  }
  return(cObj);
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Returns the file name. Only individual files are supported,
 compared to IIP where also sequences are accepted
 * \return       filename of the Woolz object
 * \par      Source:
 *                WlzImage.cc
 */
string WlzImage::getFileName( )
{
  return getImagePath();
}


/*!
 * \ingroup      WlzIIPServer
 * \brief        Return the section size
 * \param        min reference to store the minimum value
 * \param        max reference to store the maximum value
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::getDepthRange(double& min, double& max){
  prepareViewStruct();
  min=wlzViewStr->minvals.vtZ;
  max=wlzViewStr->maxvals.vtZ;
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return in degrees the rotation angles of the section
 * \param        theta reference to theta rotation angle
 * \param        phi reference to phi rotation angle
 * \param        zeta reference to zeta rotation angle
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
void WlzImage::getAngles(double& theta, double& phi, double& zeta){
  prepareViewStruct(); 
  theta = wlzViewStr->theta / WLZ_M_PI * 180.0;
  phi   = wlzViewStr->phi   / WLZ_M_PI * 180.0;
  zeta  = wlzViewStr->zeta  / WLZ_M_PI * 180.0;
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return section coordinates of the quieried point with tile or
 * 		 display coordinates
 * \return       WlzDVertex3 section coordinates of the queried point
 * \par      Source:
 *                WlzImage.cc
 */
WlzDVertex3 WlzImage::getCurrentPointInPlane(){
  int tw, th; //real tile width and height
  
  prepareViewStruct();
  
  // Check that a valid tile number was given
  WlzDVertex3   result;
  
  loadImageInfo(0,0);
  
  //recompute image sizes
  if( viewParams->tile == -1 )  {
    result.vtX = wlzViewStr->minvals.vtX + viewParams->x;
    result.vtY = wlzViewStr->minvals.vtY + viewParams->y;
  } else {
    if( viewParams->tile >= number_of_tiles || viewParams->tile < 0)  {
      char tile_no[64];
      snprintf( tile_no, 64, "%d", curViewParams->tile );
      string tile_n = string( tile_no );
      throw("WlzImage::getCurrentPointInPlane() "
            "asked for non-existant tile: " + tile_n);
    }
    
    // Alter the tile size if it's in the last column
    if( ( viewParams->tile % ntlx == ntlx - 1 ) && ( lastTileWidth != 0 ) ) {
      tw = lastTileWidth;
    }
    
    // Alter the tile size if it's in the bottom row
    if( ( viewParams->tile / ntlx == ntly - 1 ) && ( lastTileHeight != 0 ) ) {
      th = lastTileHeight;
    }
    result.vtX = (viewParams->tile % ntlx) * tile_width + wlzViewStr->minvals.vtX + viewParams->x;
    result.vtY = (viewParams->tile / ntlx) * tile_height + wlzViewStr->minvals.vtY + viewParams->y;
  }
  result.vtZ = curViewParams->dist;
  return result;
}

/*!
 * \return      The object coordinates of the queried point.
 * \ingroup     WlzIIPServer
 * \brief 	Gets object coordinates of the quieried point with tile or
 * 		display coordinates
 */
WlzDVertex3
WlzImage::getCurrentPointIn3D()
{
  WlzDVertex3   result = getCurrentPointInPlane();
  prepareViewStruct();
  Wlz3DSectionTransformInvVtx(&result , wlzViewStr );
  return(result);
}

/*!
 * \return      Object coordinates of the transformed query point in the
 * 		current section plane.
 * \ingroup     WlzIIPServer
 * \brief       Gets the transformed coordinates of the 3D query point.
 */
WlzDVertex3
WlzImage::getTransformed3DPoint()
{
  WlzDVertex3   pos = {0};
  if(viewParams->queryPointType == QUERYPOINTTYPE_3D)
  {
    pos =  viewParams->queryPoint;
  }
  prepareViewStruct();
  Wlz3DSectionTransformVtx(&pos, wlzViewStr);
  pos.vtZ -= viewParams->dist;
  pos.vtX -= wlzViewStr->minvals.vtX;
  pos.vtY -= wlzViewStr->minvals.vtY;
  return(pos);
}

/*!
 * \return       Pointer to a 1x3 float array of the voxel size.
 * \ingroup      WlzIIPServer
 * \brief        Gets the object's voxel size.
 */
float
*WlzImage::getTrueVoxelSize()
throw(std::string)
{
  float	*voxSz = NULL;
  prepareObject();
  WlzObject *obj = getObj();
  if(obj == NULL)
  {
    throw(makeWlzErrorMessage("WlzImage::get3DBoundingBox()",
                              WLZ_ERR_OBJECT_NULL));
  }
  else
  {
    voxSz = obj->domain.p->voxel_size;
  }
  return(voxSz);
}

/*!
 * \ingroup     WlzIIPServer
 * \brief       Gets the 3D bounding box of the current object.
 * \param 	box		Destination for the bounding box.
 */
void
WlzImage::get3DBoundingBox(WlzIBox3 &box)
throw(std::string)
{
  prepareObject();
  WlzObject *obj = getObj();
  if(obj == NULL)
  {
    throw(makeWlzErrorMessage("WlzImage::get3DBoundingBox()",
                              WLZ_ERR_OBJECT_NULL));
  }
  else
  {
    box.zMin = obj->domain.p->plane1;
    box.zMax = obj->domain.p->lastpl;
    box.yMin = obj->domain.p->line1;
    box.yMax = obj->domain.p->lastln;
    box.xMin = obj->domain.p->kol1;
    box.xMax = obj->domain.p->lastkl;
  }
}

/*!
* \ingroup	WlzIIPServer
* \brief	Gets simple grey value statistics for the current object.
* \param	n		Destination for the object's volume.
* \param	t		Destination for the object's type.
* \param	gl		Destination for minimum grey value.
* \param	gu		Destination for maximum grey value.
* \param	sum		Destination for sum of grey values.
* \param	ss		Destination for sum of squares of grey values.
* \param	mean		Destination for mean grey values.
* \param	sdev		Destination for standard deviation of grey
* 				values.
*/
void
WlzImage::getGreyStats(int &n, WlzGreyType &t, double &gl, double &gu,
                       double &sum, double &ss, double &mean, double &sdev)
throw(std::string)
{
  WlzErrorNum	errNum = WLZ_ERR_NONE;
  prepareObject();
  WlzObject *obj = getObj();
  n = WlzGreyStats(obj, &t, &gl, &gu, &sum, &ss, &mean, &sdev, &errNum);
  if(errNum != WLZ_ERR_NONE)
  {
    throw(makeWlzErrorMessage("WlzImage::getGreyStats() ", errNum));
  }
}

/*!
 * \ingroup     WlzIIPServer
 * \brief       Return transformed 3D bounding box
 * \param 	box		Destination for the bounding box.
 */
void
WlzImage::getTransformed3DBBox(WlzIBox3 &box)
throw(std::string)
{
  WlzErrorNum	errNum = WLZ_ERR_NONE;
  prepareObject();
  prepareViewStruct();
  errNum = Wlz3DViewStructTransformBB(getObj(), wlzViewStr);
  if(errNum == WLZ_ERR_NONE)
  {
    box.zMin = WLZ_NINT(wlzViewStr->minvals.vtZ);
    box.zMax = WLZ_NINT(wlzViewStr->maxvals.vtZ);
    box.yMin = WLZ_NINT(wlzViewStr->minvals.vtY);
    box.yMax = WLZ_NINT(wlzViewStr->maxvals.vtY);
    box.xMin = WLZ_NINT(wlzViewStr->minvals.vtX);
    box.xMax = WLZ_NINT(wlzViewStr->maxvals.vtX);
  }
  else
  {
    throw(makeWlzErrorMessage("WlzImage::getTransformed3DBBox()", errNum));
  }
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return object volume
 * \return       the object volume
 */
WlzLong 	WlzImage::getVolume()
throw(std::string)
{
  WlzLong	vol = 0;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  prepareObject();
  vol = WlzVolume(getObj(), &errNum);
  if(errNum != WLZ_ERR_NONE)
  {
    throw(makeWlzErrorMessage("WlzImage::getVolume()", errNum));
  }
  return(vol);
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return pixel value 
 * \param        points pointer to the array where the points should be stored
 * \return       the number of channels
 * \par      Source:
 *                WlzImage.cc
 */
int WlzImage::getGreyValue(int *points){
  
  WlzErrorNum errNum = WLZ_ERR_NONE;
  WlzGreyValueWSpace* gvWSp;
  WlzDVertex3   pos;
  
  switch (viewParams->queryPointType) {
  case QUERYPOINTTYPE_2D:
    // use 2D (tile based) relative query
    pos = getCurrentPointInPlane();
    Wlz3DSectionTransformInvVtx ( &pos, wlzViewStr );
    break;
  case QUERYPOINTTYPE_3D:
    // use 3D absolute query
    pos =  viewParams->queryPoint;
    break;
  defualt:
    return -1;  //return -1 for invalid point
  }
  
  prepareObject();
  
  WlzCompoundArray *array = wlzObject->type==WLZ_COMPOUND_ARR_2 ? (WlzCompoundArray *)wlzObject : NULL;
  WlzObject *obj = array ?  ( array->n>0 ? array->o[0] : NULL) : wlzObject;
  
  gvWSp =  WlzGreyValueMakeWSp(obj, &errNum);
  if (errNum == WLZ_ERR_NONE) {
    WlzGreyValueGet(gvWSp, pos.vtZ, pos.vtY, pos.vtX);
    
    switch (gvWSp->gType) {
    case WLZ_GREY_INT:
      points[0]=(*(gvWSp->gVal)).inv;
      return 1;
    case WLZ_GREY_UBYTE:
      points[0]=(*(gvWSp->gVal)).ubv;
      return 1;
    case WLZ_GREY_SHORT:
      points[0]=(*(gvWSp->gVal)).shv;
      return 1;
    case WLZ_GREY_RGBA :
      points[0]=WLZ_RGBA_RED_GET((*(gvWSp->gVal)).rgbv);
      points[1]=WLZ_RGBA_GREEN_GET((*(gvWSp->gVal)).rgbv);
      points[2]=WLZ_RGBA_BLUE_GET((*(gvWSp->gVal)).rgbv);
      if (channels==4) { 
	points[3]=WLZ_RGBA_ALPHA_GET((*(gvWSp->gVal)).rgbv);
	return 4;
      }
      return 3;
    default:
      break;
    }
  }
  return 0;
}


/*!
 * \ingroup      WlzIIPServer
 * \brief        Return number of object components
 * \return       the number of compound object or if not compound it returns 1
 * \par      Source:
 *                WlzImage.cc
 */
int WlzImage::getCompoundNo() {
  prepareObject();
  WlzCompoundArray *array = wlzObject->type==WLZ_COMPOUND_ARR_2 ? (WlzCompoundArray *)wlzObject : NULL;
  return array ?  array->n : 1;
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return number of visible objects at the query point and return these object indexes
 * \param        points pointer to the array where the points should be stored
 * \return       the number visible objects, or -1 for error
 * \par      Source:
 *                WlzImage.cc
 */
int WlzImage::getForegroundObjects(int *values){
  
  WlzErrorNum errNum = WLZ_ERR_NONE;
  WlzDVertex3   pos;
  
  switch (viewParams->queryPointType) {
  case QUERYPOINTTYPE_2D:
    // use 2D (tile based) relative query
    pos = getCurrentPointInPlane();
    Wlz3DSectionTransformInvVtx ( &pos, wlzViewStr );
    break;
  case QUERYPOINTTYPE_3D:
    // use 3D absolute query
    pos =  viewParams->queryPoint;
    break;
  defualt:
    return -1;  //return -1 for invalid point
  }
  
  prepareObject();
  int counter = 0;
  WlzCompoundArray *array = wlzObject->type==WLZ_COMPOUND_ARR_2 ? (WlzCompoundArray *)wlzObject : NULL;
  WlzObject *obj;
  if (array) {
    int i;
    for (i=0; i<array->n; i++) {
      errNum = WLZ_ERR_NONE;
      obj = array->o[i];
      if (obj && WlzInsideDomain(obj, pos.vtZ, pos.vtY, pos.vtX, &errNum) && errNum == WLZ_ERR_NONE) {
	values[counter++]= i;
      }
    }
  } else {
    if (WlzInsideDomain(wlzObject, pos.vtZ, pos.vtY, pos.vtX, &errNum) && errNum == WLZ_ERR_NONE) {
      values[counter++]= 0;
    }
  }
  return counter;
}


/*!
 * \ingroup      WlzIIPServer
 * \brief        Converts a Woolz of type WLZ_2D_DOMAINOBJ with WLZ_GREY_RGBA
 * 		 grey values into a 3 channel raw bitmap
 * \param        buffer to store converted image. the buffer has to be correctly preallocated
 * \param        obj object to be converted
 * \param        pos the origin of the tile
 * \return       void
 * \par      Source:
 *                WlzImage.cc
 */
WlzErrorNum
WlzImage::convertDomainObjToRGB(WlzUByte *cbuffer, WlzObject* obj,
                                WlzIVertex2  pos, WlzIVertex2  size,
				CompoundSelector *sel)
{
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(!cbuffer || !obj)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else if(obj->type == WLZ_2D_DOMAINOBJ)
  {
    int           i;
    WlzIntervalWSpace iwsp;

    int col1 = pos.vtX;
    int line1 = pos.vtY;
    int lnOff = 0;
    int iwidth = 0;
    int alpha = sel ? sel->a:255;
    int nCh = getNumChannels();
    float fA = alpha / 255.0;
    float fA1 = 1.0f - fA;
    float r = (sel ? sel->r:255) * fA;
    float g = (sel ? sel->g:255) * fA;
    float b = (sel ? sel->b:255) * fA;

    //scan the object
    if((errNum = WlzInitRasterScan(obj, &iwsp,
	                           WLZ_RASTERDIR_ILIC)) == WLZ_ERR_NONE)
    {
      while((errNum = WlzNextInterval(&iwsp)) == WLZ_ERR_NONE)
      {
	WlzUInt val;
	lnOff = ((size.vtX * (iwsp.linpos-line1)) + (iwsp.lftpos-col1))* nCh;
	iwidth = iwsp.rgtpos - iwsp.lftpos;
	for(i = 0; i <= iwidth; i++)
	{
	  int	lnCOff = lnOff + (i * nCh);

	  cbuffer[lnCOff]     = (WlzUByte )(r + cbuffer[lnCOff] * fA1);
	  cbuffer[lnCOff + 1] = (WlzUByte )(g + cbuffer[lnCOff + 1] * fA1);
	  cbuffer[lnCOff + 2] = (WlzUByte )(b + cbuffer[lnCOff + 2] * fA1);
	  if (nCh==4)
	  {
	    float a2 = cbuffer[lnCOff + 3] / 255.0f;
	    cbuffer[lnCOff + 3] = (WlzUByte )round((fA + a2 - a2 * fA)*255.0);
	  }
	}
      }
    }
    if(errNum == WLZ_ERR_EOO)
    {
      errNum = WLZ_ERR_NONE;
    }
  }
  return(errNum);
}

/*!
 * \return      Woolz error code.
 * \ingroup     WlzIIPServer
 * \brief       Converts a 2D value object to a linearised 2D array
 * \param       cbuf    	Allocated memmory location for the output.
 * \param	obj        	Input object.
 * \param       pos        	Section bounding box origin.
 * \param       size       	Section bounding box size.
 * \param       sel        	Selector with the colour to be used for the
 * 				section.
 * \par      Source:
 *                WlzImage.cc
 */
WlzErrorNum
WlzImage::convertValueObjToRGB(WlzUByte *cbuf,
			       WlzObject* obj,
                               WlzIVertex2  pos, WlzIVertex2  size,
			       CompoundSelector *sel)
{
  WlzIntervalWSpace     iwsp;
  WlzGreyWSpace         gwsp;
  int           i,
  		j;
  WlzGreyType   gType;
  WlzErrorNum   errNum = WLZ_ERR_NONE;
  
  if(!cbuf || !obj)
  {
    return WLZ_ERR_OBJECT_NULL;
  }
  if(obj->values.i == NULL)
  {
    return WLZ_ERR_DOMAIN_NULL;
  }
  
  int col1 = pos.vtX;
  int line1 = pos.vtY;
  int lineoff = 0;
  int iwidth = 0;
  int outchannels = getNumChannels();
  bool copyGreyToRGB = outchannels != channels;
  int alphaoffset = (outchannels == 2 || outchannels==4)?
                    ((copyGreyToRGB || outchannels==4)? 3 :1): 0;
  float gray;
  int   alpha = sel ? sel->a : 255;
  float fA = alpha / 255.0f;
  float fR = sel ? sel->r/255.0f : 1.0f;
  float fG = sel ? sel->g/255.0f : 1.0f;
  float fB = sel ? sel->b/255.0f : 1.0f;
  float fAlphaPrev = 0, fANew=0;
  
  gType = WlzGreyTypeFromObj(obj, &errNum);
  //scan the object
  if(errNum == WLZ_ERR_NONE)
  {
    errNum = WlzInitGreyScan(obj, &iwsp, &gwsp);
  }
  if(errNum == WLZ_ERR_NONE)
  {
    j = 0;
    while((errNum == WLZ_ERR_NONE) &&
          ((errNum = WlzNextGreyInterval(&iwsp)) == WLZ_ERR_NONE))
    {
      lineoff = (size.vtX * (iwsp.linpos - line1) + (iwsp.colpos - col1)) *
                outchannels;
      iwidth = iwsp.rgtpos-iwsp.lftpos;
      switch( gType ){
      case WLZ_GREY_LONG:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray = gwsp.u_grintptr.lnp[i] * fA;
	  cbuf[boff] = (WlzUByte)(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB)
	  {
	    cbuf[boff + 1] = (WlzUByte )(gray * fG + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB + cbuf[boff + 2] * (1 - fA));
	  }
	  if(alphaoffset > 0)
	  {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )(round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0));
	  }
	}
	break;
      case WLZ_GREY_INT:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray = gwsp.u_grintptr.inp[i] * fA;
	  cbuf[boff] = (WlzUByte )(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB) {
	    cbuf[boff + 1] = (WlzUByte )(gray * fG + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB + cbuf[boff + 2] * (1 - fA));
	  }
	  if (alphaoffset>0) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )(round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0));
	  }
	}
	break;
      case WLZ_GREY_SHORT:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray= gwsp.u_grintptr.shp[i] * fA;
	  cbuf[boff] = (WlzUByte )(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB) {
	    cbuf[boff + 1] = (WlzUByte )(gray * fR + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB + cbuf[boff + 2] * (1 - fA));
	  }
	  if (alphaoffset>0) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0);
	  }
	}
	break;
      case WLZ_GREY_FLOAT:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray= gwsp.u_grintptr.flp[i] * fA;
	  cbuf[boff] = (WlzUByte )(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB) {
	    cbuf[boff + 1] = (WlzUByte )(gray * fG + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB+ cbuf[boff + 2] * (1 - fA));
	  }
	  if (alphaoffset>0) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0);
	  }
	}
	break;
      case WLZ_GREY_DOUBLE:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray= gwsp.u_grintptr.dbp[i] * fA;
	  
	  cbuf[boff] = (WlzUByte )(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB) {
	    cbuf[boff + 1] = (WlzUByte )(gray * fG + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB + cbuf[boff + 2] * (1 - fA));
	  }
	  if (alphaoffset>0) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )(round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0));
	  }
	}
	break;
	
      case WLZ_GREY_UBYTE:
	for(i=0; i <= iwidth; i++){
	  int boff = lineoff + (i * outchannels);
	  gray= gwsp.u_grintptr.ubp[i] * fA;
	  cbuf[boff] = (WlzUByte )(gray * fR + cbuf[boff] * (1 - fA));
	  if (copyGreyToRGB) {
	    cbuf[boff + 1] = (WlzUByte )(gray * fG + cbuf[boff + 1] * (1 - fA));
	    cbuf[boff + 2] = (WlzUByte )(gray * fB + cbuf[boff + 2] * (1 - fA));
	  }
	  if (outchannels==2 || outchannels==4) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )round((fA + fAlphaPrev - fAlphaPrev * fA)*255.0);
	  }
	}
	break;
	
      case WLZ_GREY_RGBA:
	for(i=0; i <= iwidth; i++){
          WlzUInt val;
	  int boff = lineoff + (i * outchannels);
	  val = gwsp.u_grintptr.rgbp[i];
	  cbuf[boff]   = (WlzUByte )(WLZ_RGBA_RED_GET(val)   * fA * fR + cbuf[boff] * (1 - fA));
	  cbuf[boff + 1] = (WlzUByte )(WLZ_RGBA_GREEN_GET(val) * fA * fG + cbuf[boff + 1] * (1 - fA));
	  cbuf[boff + 2] = (WlzUByte )(WLZ_RGBA_BLUE_GET(val)  * fA * fB + cbuf[boff + 2] * (1 - fA));
	  if (outchannels==4) {
	    fAlphaPrev=cbuf[boff + alphaoffset]/255.0f;
	    fANew = fA* ((WlzUByte )(WLZ_RGBA_ALPHA_GET(val)))/255.0f;
	    cbuf[boff + alphaoffset] = (WlzUByte )round((fANew + fAlphaPrev - fAlphaPrev * fANew)*255.0);
	  }
	}
	break;
      default:
	errNum = WLZ_ERR_GREY_TYPE;
	break;
      }
    }
  }
  if(errNum == WLZ_ERR_EOO)
  {
    errNum = WLZ_ERR_NONE;
  }
  return(errNum);
}

/*!
 * \return       Woolz error code.
 * \ingroup      WlzIIPServer
 * \brief        Converts a 2D domain or value object to a linearised 2D array
 * \param        cbuffer    allocated memmory location for the output
 * \param        obj        input object
 * \param        pos        section bounding box origin
 * \param        size       section bounding box size
 * \par      Source:
 *                WlzImage.cc
 */
WlzErrorNum
WlzImage::convertObjToRGB(WlzUByte *cbuffer, WlzObject *obj, WlzIVertex2 pos,
			  WlzIVertex2  size)
{
  WlzErrorNum   errNum;

  if(!cbuffer || !obj)
  {
    errNum = WLZ_ERR_OBJECT_NULL;
  }
  else
  {
    int i = 0;
    int outchannel = getNumChannels();
    
    // Clear buffer
    for(i=0;i<size.vtX*size.vtY;i++)
    {
      (void )memcpy(cbuffer + (i * channels), background, outchannel);
    }
    // Set buffer values
    switch(obj->type)
    {
      case WLZ_EMPTY_OBJ:
	break;
      case WLZ_2D_DOMAINOBJ:
	errNum = (obj->values.core == NULL)?
		 convertDomainObjToRGB(cbuffer,  obj,  pos, size, NULL):
		 convertValueObjToRGB(cbuffer,  obj,  pos, size, NULL);
	break;
      default:
	errNum = WLZ_ERR_OBJECT_TYPE;
	break;
    }
  }
  return(errNum);
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Return the image hash
 * \return       the hash string
 * \par      Source:
 *                WlzImage.cc
 */
const std::string WlzImage::getHash() { 
  return generateHash(viewParams) + selString(viewParams);
};

/*!
 * \ingroup      WlzIIPServer
 * \brief        The hash string component generated by SEL commands
 * \param        view parameters
 * \return       the hash string
 * \par      Source:
 *                WlzImage.cc
 */
const std::string WlzImage::selString(const ViewParameters* view )
{
  char		*eStr;
  WlzErrorNum errNum = WLZ_ERR_NONE;

  if( view == NULL)
  {
    return "";
  }
  std::string selStr = "";
  CompoundSelector *iter = view->selector;
  while(iter)
  {
    char	*tStr;
    char 	buf[25];

    tStr = WlzExpStr(iter->expression, NULL, NULL);
    if(tStr)
    {
      selStr += tStr;
    }
    snprintf(buf, 25, ",%d,%d,%d,%d",iter->r,iter->g,iter->b,iter->a);
    selStr += buf;
    iter=iter->next;
    AlcFree(tStr);
  }
  return("S=" + selStr);
}

/*!
 * \ingroup      WlzIIPServer
 * \brief        Generate the image hash for a set of view parameters
 * \param        view parameters
 * \return       the hash string
 * \par      Source:
 *                WlzImage.cc
 */
const std::string WlzImage::generateHash(const ViewParameters* view ) { 
  if ( view == NULL)
    view = curViewParams;
  prepareObject();  // needs to have set channel number
  
  char temp[512];
  snprintf(temp, 512,
	 "(D=%g,S=%g,Y=%g,P=%g,R=%g,M=%d,P=%gN=%d,C=%d,F=%g,%g,%g,F2=%g,%g,%g)",
	   view->dist,
	   view->scale,
	   view->yaw,
	   view->pitch,
	   view->roll,
	   view->mode,
	   view->depth,
	   view->rmd,
	   getNumChannels(),
	   view->fixed.vtX,
	   view->fixed.vtY,
	   view->fixed.vtZ,
	   view->fixed2.vtX,
	   view->fixed2.vtY,
	   view->fixed2.vtZ);
  return(getImagePath() + temp + view->map.toString());
};
