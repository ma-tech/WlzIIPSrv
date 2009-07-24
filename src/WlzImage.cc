#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _WlzImage_cc[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         WlzImage.cc
* \author       Zsolt Husz
* \date         June 2008
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C) 2008 Medical research Council, UK.
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
* \ingroup	WlzIIPServer
* \todo         -
* \bug          None known.
*/


/*
TODO:

2. Default commands / metadata
3. 
    - range ??
    - other , default ocmmands

    - return features of the image: check default parameters  - title, etc
    - read metadata for wlz

    - error handling:
      - range chechking - what other parameters?

    - reset changed features?
*/

#include "WlzImage.h"
#include <WlzProto.h>
#include <WlzExtFF.h>
#include "Environment.h"

//#define __EXTENDED_DEBUG
#ifdef __EXTENDED_DEBUG
#include "Task.h"
extern      Session session;
#endif

using namespace std;

/*!Interpolation type */
const WlzInterpolationType WlzImage::interp = WLZ_INTERPOLATION_NEAREST;

/*!View structure cache. Static for all queries. 
*/
WlzViewStructCache        WlzImage::cacheViewStruct;


/*!Object cache. Static for all queries. 
*/
WlzObjectCache            WlzImage::cacheWlzObject(&cacheViewStruct);


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
* \brief        Update the view structure used to generate sections either from the 
*       view structure cache or by recomputing it.
*
* \return       void
* \par      Source:
*                WlzImage.cc
*/
void WlzImage::prepareViewStruct() throw(string)
{
  if (!isViewChanged())
  {
    return;
  }


  if (!viewParams)
  {
    throw string( "WlzImage :: viewParams is NULL.");
  }

  prepareObject();  //make sure object is loaded

  WlzErrorNum errNum = WLZ_ERR_NONE;

  //generate cache hash
  string hash = generateHash(viewParams);

  #ifdef __EXTENDED_DEBUG
    *(session.logfile) << "WlzImage :: prepareViewStruct: hash:" << hash << endl;
  #endif

  if( wlzViewStr != NULL )
      WlzFree3DViewStruct( wlzViewStr );

  wlzViewStr  = WlzAssign3DViewStruct( cacheViewStruct.get(hash), NULL );

  if (wlzViewStr == NULL)  // cache miss?
  {

    if((wlzViewStr = WlzAssign3DViewStruct( WlzMake3DViewStruct(WLZ_3D_VIEW_STRUCT, &errNum), NULL )) != NULL){
      wlzViewStr->theta           = viewParams->yaw   * WLZ_M_PI / 180.0;
      wlzViewStr->phi             = viewParams->pitch * WLZ_M_PI / 180.0;
      wlzViewStr->zeta            = viewParams->roll  * WLZ_M_PI / 180.0;
      wlzViewStr->dist            = viewParams->dist;
      wlzViewStr->fixed           = viewParams->fixed;
      wlzViewStr->fixed_2         = viewParams->fixed2;
      wlzViewStr->up              = viewParams->up;
      wlzViewStr->view_mode       = viewParams->mode;
      wlzViewStr->scale           = viewParams->scale ;
      wlzViewStr->voxelRescaleFlg = 0x02;  //use scale
      //wlzViewStr->voxelRescaleFlg = 0x01 | 0x02;  // set if voxel size correciton is needed
    } else {
      throw string( "WlzImage :: prepareViewStruct creation failed.");
    }

    WlzInit3DViewStruct(wlzViewStr, wlzObject);
    ////////////////////////////////////////////////////////

    cacheViewStruct.insert(wlzViewStr , hash);

  }
  return ;
}

/*!
* \ingroup      WlzIIPServer
* \brief        Prepare the 3D Woolz object either by looking it up from the object cache or
*               by reading it from disk
*
* \return       void
* \par      Source:
*                WlzImage.cc
*/
void WlzImage::prepareObject() throw(string)
{
  WlzGreyType   gType;
  WlzErrorNum   errNum = WLZ_ERR_NONE;

  if( !wlzObject ) {

    string filename;

    #ifdef __EXTENDED_DEBUG
      *(session.logfile) << "WlzImage :: prepareObject: reloading" << endl;
    #endif

    //check cache first
    filename = getFileName( );

    wlzObject  = WlzAssignObject( cacheWlzObject.get(filename), NULL );

    if (wlzObject == NULL)  // cache miss?
    {
      // if not in cache then load

      wlzObject = WlzEffReadObj( NULL, filename.c_str(), WLZEFF_FORMAT_WLZ, 0, &errNum );

      //test object type
      switch (wlzObject->type) {
          case WLZ_3D_DOMAINOBJ:
              break;
          default:
              throw string( "WlzImage :: prepareObject:  Incorrect object type. Currently supported: WLZ_3D_DOMAINOBJ.");
      }


      wlzObject = WlzAssignObject( wlzObject, &errNum );

      if (wlzObject == NULL || errNum != WLZ_ERR_NONE)
      {
        throw string( "WlzImage :: prepareObject:  Woolz object can not be read: " + filename );
      }
      cacheWlzObject.insert( wlzObject , filename );
    }

     isSet = false;   //yet object view specific data has to be set
  }
  else
  {
    #ifdef __EXTENDED_DEBUG
      *(session.logfile) << "WlzImage :: prepareObject: not reloaded: already initialised" << endl;
    #endif
  }

  //set global parameters
  gType = WlzGreyTypeFromObj(wlzObject, &errNum);
  if (errNum != WLZ_ERR_NONE) {
          throw string( "WlzImage :: prepareObject:  Error while WrlGreyTypeFromObj.");
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
            throw string( "WlzImage :: prepareObject:  Unsuported GreyType. Currently supported: WLZ_GREY_LONG, WLZ_GREY_INT, WLZ_GREY_SHORT, WLZ_GREY_UBYTE, WLZ_GREY_FLOAT, WLZ_GREY_DOUBLE and WLZ_GREY_RGBA.");
  }

  bpp = (unsigned int) 8;  //bits per channel

  // Assign the colourspace variable
  if( channels == 8 ) colourspace = CIELAB;
  else if( channels == 1 ) colourspace = GREYSCALE;
  else colourspace = sRGB;

}

/*!
* \ingroup      WlzIIPServer
* \brief        Checks if current view structure of WlzImage is up to date with the parameters
                passed to the server
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
* \brief        Load object specific info: image and tile size, number of tiles, metadata. 
* 
*
* \return       void
* \par      Source:
*                WlzImage.cc
* \par           Pramaters are not used, required by the base class definition.
*/
void WlzImage::loadImageInfo( int , int ) throw(string)
{

  WlzErrorNum errNum = WLZ_ERR_NONE;

  if (isViewChanged()) {
    prepareObject();

#ifdef COMPUTELOCAL_VIEWSTR
    // This code is currently not used,
    // since local view structure would add additional computations
    // for point queries.

    /* check the pointers and types */
    if( wlzViewStr == NULL || wlzObject == NULL ){
      errNum =  WLZ_ERR_OBJECT_NULL;
    }

    /* release any allocated structures and memory */
    if( errNum == WLZ_ERR_NONE ){
      /* check free stack */
      if( wlzViewStr->freeptr ){
        AlcFreeStackFree(wlzViewStr->freeptr);
        wlzViewStr->freeptr = NULL;
      }
      /* check the reference object */
      if( wlzViewStr->ref_obj ){
        errNum = WlzFreeObj(wlzViewStr->ref_obj);
        wlzViewStr->ref_obj = NULL;
      }
      /* reset initialisation */
      wlzViewStr->initialised = WLZ_3DVIEWSTRUCT_INIT_NONE;
    }

    /* calculate the affine transform */
    if( errNum == WLZ_ERR_NONE ){
      errNum = WlzInit3DViewStructAffineTransform(wlzViewStr);
    }

    /* calculate target bounding box */
    if( errNum == WLZ_ERR_NONE ){
      errNum = Wlz3DViewStructTransformBB(wlzObject, wlzViewStr);
    }

    if( errNum == WLZ_ERR_NONE ){
      throw string( "WlzImage :: loadImageInfo: wlzViewStr creation failed" );
    }
#else
     prepareViewStruct();
#endif

    image_width  = (unsigned int)(wlzViewStr->maxvals.vtX-wlzViewStr->minvals.vtX) + 1;
    image_height = (unsigned int)(wlzViewStr->maxvals.vtY-wlzViewStr->minvals.vtY) + 1;

    #ifdef __EXTENDED_DEBUG
      *(session.logfile) << "WlzImage :: loadImageInfo: image size : "<< image_width << " x " << image_height << endl;
    #endif

    // Get the width and height for last row and column tiles
    lastTileWidth = image_width % tile_width  ;
    lastTileHeight = image_height % tile_height;

    // Calculate the number of tiles in each direction
    ntlx = (image_width / tile_width) + (lastTileWidth== 0 ? 0 : 1);
    ntly = (image_height / tile_height) + (lastTileHeight == 0 ? 0 : 1);

    number_of_tiles = ntlx * ntly;

    #ifdef __EXTENDED_DEBUG
      *(session.logfile) << "WlzImage :: loadImageInfo: number of tiles: "<< number_of_tiles << endl;
    #endif


    numResolutions = 1; // resoltions. currently only one resolution is used.

    // Update current sections view status
    if (curViewParams == NULL)
    { 
      curViewParams = new ViewParameters;
    }
    *curViewParams = *viewParams;

    // No metadata is stored in the Woolz object, therefer set them to unknosn
    metadata["author"] = "author unknown";
    metadata["copyright"] = "copyright unknown";
    metadata["create-dtm"] = "create-dtm unknown";
    metadata["subject"] = "subject unknown";
    metadata["app-name"] = "app-name unknown";

  }
  #ifdef __EXTENDED_DEBUG
    *(session.logfile) << "WlzImage :: loadImageInfo: done" << endl;
  #endif
}

/*!
* \ingroup      WlzIIPServer
* \brief        Release the object and all related data.
*
* \return       void
* \par      Source:
*                WlzImage.cc
*/
void WlzImage::closeImage()
{

  // free tile temporary data
  if( tile_buf != NULL ){
    //AlcFree(tile_buf);
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
  }
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
RawTile WlzImage::getTile( int seq, int ang, unsigned int res, unsigned int tile) throw (string)
{
  int tw, th; //real tile width and height

  WlzErrorNum errNum=WLZ_ERR_NONE;

  string filename;

  WlzObject     *tmpObj;
  WlzIVertex3   pos;
  WlzObject     *wlzSection = NULL;

  seq = ang = res  = 0; // force unused parameters to zero to, facilitate cache match
  loadImageInfo( 0, 0);

  // Check that a valid tile number was given
  if( tile >= number_of_tiles)  {
    char tile_no[64];
    snprintf( tile_no, 64, "%d", tile );
    string tile_n = string( tile_no );
    throw string( "Asked for non-existant tile: " + tile_n );
  }

  tw= tile_width;
  th= tile_height;

  // Alter the tile size if it's in the last column
  if( ( tile % ntlx == ntlx - 1 ) && ( lastTileWidth != 0 ) ) {
    tw = lastTileWidth;
  }

  // Alter the tile size if it's in the bottom row
  if( ( tile / ntlx == ntly - 1 ) && ( lastTileHeight != 0 ) ) {
    th = lastTileHeight;
  }


  if( errNum == WLZ_ERR_NONE ){
    /* create maximum sized rectangular object */
    WlzDomain     domain;
    WlzValues     values;

    pos.vtX = (tile % ntlx) * tile_width + (unsigned int)(wlzViewStr->minvals.vtX);
    pos.vtY = (tile / ntlx) * tile_height + (unsigned int)(wlzViewStr->minvals.vtY);

    if((domain.i = WlzMakeIntervalDomain(WLZ_INTERVALDOMAIN_RECT,
                                    WLZ_NINT(pos.vtY),
                                    WLZ_NINT(pos.vtY + th - 1),
                                    WLZ_NINT(pos.vtX),
                                    WLZ_NINT(pos.vtX + tw - 1),
                                        &errNum))){

      #ifdef __EXTENDED_DEBUG
        *(session.logfile) << "WlzImage :: domainn size "<< pos.vtX<<","<< pos.vtY<<","<<tw<<","<< th<<" ---" <<wlzObject->domain.core->type<<endl;
      #endif

      values.core = NULL;
      tmpObj = WlzAssignObject(WlzMakeMain(WLZ_2D_DOMAINOBJ, domain,
                          values, NULL, NULL, &errNum), NULL);
    }
  }

  if( errNum == WLZ_ERR_NONE ){
    wlzSection =  WlzAssignObject(
                    WlzGetSubSectionFromObject(wlzObject,
                          tmpObj,
                          wlzViewStr,
                          interp,
                          NULL,
                          &errNum ) ,
                  NULL);
    WlzFreeObj(tmpObj);
  }

  if (wlzSection == NULL || errNum != WLZ_ERR_NONE)
  {
    throw string( "WlzImage : Sectioning failed.");
  }

  WlzIBox3 gBufBox;
  gBufBox.xMin=pos.vtX;
  gBufBox.yMin=pos.vtY;
  gBufBox.xMax=pos.vtX + tw -1;
  gBufBox.yMax=pos.vtY + th -1;
  gBufBox.zMin=0;
  gBufBox.zMax=0;

  if (!tile_buf) {
    // allocate tile buffer for the maximum sized buffer,
    // even if this tile is edge tile (\ie smaller)
    tile_buf = malloc ( tile_width * tile_height * channels); 
  }

  #ifdef __EXTENDED_DEBUG
  //  *(session.logfile) << "WlzImage :: closeImage: malloc " << size.vtX << size.vtY << endl;
  #endif


  // extract a 1 or 3 channel tile

  errNum=convertObjToRGB(tile_buf, wlzSection);

  if (errNum!=WLZ_ERR_NONE){
    char temp[20];
    snprintf(temp, 20, " no %d" ,errNum);
    throw string( "WlzImage :: conversion to array" + (string)temp);
  }

  #ifdef __EXTENDED_DEBUG
    *(session.logfile) << "WlzImage :: getTile : Raw creation "<< endl;
  #endif

  RawTile rawtile( tile, res, seq, ang, tw, th, channels, bpp );
  rawtile.data = tile_buf;
  rawtile.dataLength = tw*th*channels;
  rawtile.width_padding = tile_width-tw;

  // 15/07/08 memmory leak fixed by Zsolt Husz
  WlzFreeObj(wlzSection);

  //get hash of the tile
  rawtile.filename = getHash();

  return( rawtile );

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
* \brief        Return section coordinates of the quieried point with tile or display coordinates
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
      throw string( "Asked for non-existant tile: " + tile_n );
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
* \ingroup      WlzIIPServer
* \brief        Return object coordinates of the quieried point with tile or display coordinates
* \return       WlzDVertex3 object coordinates of the queried point
* \par      Source:
*                WlzImage.cc
*/
WlzDVertex3 WlzImage::getCurrentPointIn3D(){
  WlzDVertex3   result = getCurrentPointInPlane();
  Wlz3DSectionTransformInvVtx ( &result , wlzViewStr );
  return result;
}

/*!
* \ingroup      WlzIIPServer
* \brief        Return object voxel size
* \return       float * pointer to a 1x3 float array of the voxel size
* \par      Source:
*                WlzImage.cc
*/
float* WlzImage::getTrueVoxelSize(){
  prepareObject();
  return wlzObject->domain.p->voxel_size;
}

/*!
* \ingroup      WlzIIPServer
* \brief        Return 3D bounding box
* \return       void
* \par      Source:
*                WlzImage.cc
*/
void WlzImage::get3DBoundingBox(int &plane1, int &lastpl, int &line1, int &lastln, int &kol1, int &lastkl){
  prepareObject();
  plane1 = wlzObject->domain.p->plane1;
  lastpl = wlzObject->domain.p->lastpl;
  line1  = wlzObject->domain.p->line1;
  lastln = wlzObject->domain.p->lastln;
  kol1   = wlzObject->domain.p->kol1;
  lastkl = wlzObject->domain.p->lastkl;
  return ;
}



/*!
* \ingroup      WlzIIPServer
* \brief        Return object volume
* \return       the object volume
* \par      Source:
*                WlzImage.cc
*/
int WlzImage::getVolume(){
  prepareObject();
  return WlzVolume(wlzObject,  NULL);
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

  gvWSp =  WlzGreyValueMakeWSp(wlzObject, &errNum);
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
* \brief        Converts a Woolz of type WLZ_2D_DOMAINOBJ with WLZ_GREY_RGBA grey values 
*               into a 3 channel raw bitmap
* \param        buffer to store converted image. the buffer has to be correctly preallocated
* \param        obj object to be converted
* \return       void
* \par      Source:
*                WlzImage.cc
*/
WlzErrorNum WlzImage::convertObjToRGB(void * buffer, WlzObject* obj) {

  WlzIntervalWSpace     iwsp;
  WlzGreyWSpace         gwsp;
  int           i, j;
  WlzGreyType   gType;
  int           width;
  WlzErrorNum   errNum;
  unsigned char          *cbuffer=(unsigned char*)buffer;

  if( obj->type != WLZ_2D_DOMAINOBJ ){
    return WLZ_ERR_OBJECT_TYPE;
  }

  gType = WlzGreyTypeFromObj(obj, &errNum);
  width = obj->domain.i->lastkl - obj->domain.i->kol1 + 1;

  //scan the object
  if((errNum = WlzInitGreyScan(obj, &iwsp, &gwsp)) == WLZ_ERR_NONE){
      j = 0;
      while((errNum = WlzNextGreyInterval(&iwsp)) == WLZ_ERR_NONE){
        WlzUInt val;
        switch( gType ){

       case WLZ_GREY_LONG:
          for(i=0; i < width; i++){
            cbuffer[j++] = gwsp.u_grintptr.lnp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;
       case WLZ_GREY_INT:
          for(i=0; i < width; i++){
            cbuffer[j++] = gwsp.u_grintptr.inp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;
       case WLZ_GREY_SHORT:
          for(i=0; i < width; i++){
            cbuffer[j++] = gwsp.u_grintptr.shp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;
       case WLZ_GREY_FLOAT:
          for(i=0; i < width; i++){
            cbuffer[j++] = (unsigned char)gwsp.u_grintptr.flp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;
       case WLZ_GREY_DOUBLE:
          for(i=0; i < width; i++){
            cbuffer[j++] = (unsigned char)gwsp.u_grintptr.dbp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;

       case WLZ_GREY_UBYTE:
          for(i=0; i < width; i++){
            cbuffer[j++] = gwsp.u_grintptr.ubp[i];
            if (channels==2) {
              cbuffer[j++] = 255;
            }
          }
          break;

        case WLZ_GREY_RGBA:
          for(i=0; i < width; i++){
            val = gwsp.u_grintptr.rgbp[i];
            cbuffer[j++] = WLZ_RGBA_RED_GET(val);
            cbuffer[j++] = WLZ_RGBA_GREEN_GET(val);
            cbuffer[j++] = WLZ_RGBA_BLUE_GET(val);
            if (channels==4) {
              cbuffer[j++] = WLZ_RGBA_ALPHA_GET(val);
            }
          }
          break;
        default:
          return WLZ_ERR_GREY_TYPE;
        }
      }

      if( errNum == WLZ_ERR_EOO ){
        errNum = WLZ_ERR_NONE;
      }
    }
    return errNum ;
}


/*!
* \ingroup      WlzIIPServer
  \brief        Return the image hash
* \return       the hash string
* \par      Source:
*                WlzImage.cc
*/
const std::string WlzImage::getHash() { 
  return generateHash(viewParams);
};

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

  char temp[256];
  snprintf(temp, 256, "(D=%g,S=%g,Y=%g,P=%g,R=%g,M=%d,C=%d,F=%g,%g,%g,F2=%g,%g,%g)",
      view->dist,
      view->scale,
      view->yaw,
      view->pitch,
      view->roll,
      view->mode,
      channels,
      view->fixed.vtX,
      view->fixed.vtY,
      view->fixed.vtZ,
      view->fixed2.vtX,
      view->fixed2.vtY,
      view->fixed2.vtZ);

  return getImagePath() + temp;
};
