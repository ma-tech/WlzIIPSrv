#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _ViewParameters_h[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         ViewParameters.h
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
* \brief	Storage for session dependent, Woolz specific parameters.
* \ingroup	WlzIIPServer
* \todo         -
* \bug          None known.
*/


#ifndef _WLZVIEW_H
#define _WLZVIEW_H

#include <Wlz.h>

#include <algorithm>
#include <string>
#include <iostream>

/*!
* \enum		_QueryPointType
* \ingroup	WlzIIPServer
* \brief	Query point type
*		Typedef: QueryPointType.
*/
typedef enum _QueryPointType
  {
    QUERYPOINTTYPE_NONE = 0,		/*!< point was not set yet */
    QUERYPOINTTYPE_2D   = 1,		/*!< 2D point was set */
    QUERYPOINTTYPE_3D   = 2,		/*!< 3D point was set */
  } QueryPointType ;


/*! 
    Storage class for session dependent, Woolz specific view parameters
*/
class ViewParameters{
 public:

  double            dist;                     /*!< distance to the sectioning plane */
  double            yaw;                      /*!< yaw angle of the sectioning plane */
  double            pitch;                    /*!< pitch angle of the sectioning plane */
  double            roll;                     /*!< roll angle of the sectioning plane */
  double            scale;                    /*!< scale of the sectioning plane */
  WlzThreeDViewMode mode;                     /*!< 3D View Mode of the sectioning plane */
  WlzDVertex3       fixed;                    /*!< fixed point */
  WlzDVertex3       fixed2;                   /*!< second fixed point */
  WlzDVertex3       up;                       /*!< upvector */

  //selection have no access methods
  int x;                                       /*!< current point x coordinate */
  int y;                                       /*!< current point y coordinate */
  int tile;                                    /*!< current point tile */

  WlzDVertex3       queryPoint;                /*!< 3D point for GreyValue enquery */
  QueryPointType    queryPointType;            /*!< type of point to be used for GreyValue enquery */

  /// Constructor
  ViewParameters() {

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
    x               = 0;
    y               = 0;
    tile            = 0;
    queryPointType = QUERYPOINTTYPE_NONE;
  };


  /// Set the sectioning plane distance
  /** \param d distance to the sectioning plane*/
  void setDistance( double d ){ dist = d; };

  /// Set the sectioning plane yaw angle
  /** \param ya yaw angle of the sectioning plane*/
  void setYaw( double ya ){ yaw = ya; };

  /// Set the sectioning plane pitch angle
  /** \param p pitch angle of the sectioning plane*/
  void setPitch( double p ){ pitch = p; };

  /// Set the sectioning plane roll angle
  /** \param r roll angle of the sectioning plane*/
  void setRoll( double r ){ roll = r; };

  /// Set the fixed point
  /** \param f fixed point for the sectioning plane rotation */
  void setFixedPoint( WlzDVertex3 f ){ fixed = f; };

  /// Set the second fixed point
  /** \param f second fixed point for the sectioning plane rotation in WLZ_FIXED_LINE_MODE mode*/
  void setFixedPoint2( WlzDVertex3 f ){ fixed2 = f; };

  /** \param u up vector for the sectioning plane rotation */
  void setUpVector( WlzDVertex3 u){ up = u; };

  /// Set the sectioning scale
  /** \param s scale factor*/
  void setScale( double s ){ scale = s; };

  /// Set the sectioning mode
  /** \param m section mode as case-insensitive string. Accepted values are:
    STATUE, UP_IS_UP, FIXED_LINE, ZERO_ZETA and ZETA */
  WlzErrorNum setMode( string m ){ 
    //make it uppercase
    transform( m.begin(), m.end(), m.begin(), ::toupper );

    if (m=="STATUE")
        mode = WLZ_STATUE_MODE;
    else if (m=="UP_IS_UP")
        mode = WLZ_UP_IS_UP_MODE;
    else if (m=="FIXED_LINE")
        mode = WLZ_FIXED_LINE_MODE;
    else if (m=="ZERO_ZETA")
        mode = WLZ_ZERO_ZETA_MODE;
    else if (m=="ZETA")
        mode = WLZ_ZETA_MODE;
    else 
    {
        mode = WLZ_STATUE_MODE;
        return  WLZ_ERR_UNSPECIFIED;  // return an unspecified error, since 
                                      // no special error for mode selection is yet defined
    }
    return  WLZ_ERR_NONE;
  };

  /// Set the sectioning mode
  /** \param m section mode as WlzThreeDViewMode structure */
  void setMode(WlzThreeDViewMode m){ 
      mode = m;
  };

  /// Set a 2D query point relative to a tile
  /** \param xx current point x coordinate
      \param yy current point y coordinate
      \param tt current point tile number
  */
  void setPoint( int xx, int yy, int tt){ x = xx; y = yy; tile = tt;
      if (queryPointType == QUERYPOINTTYPE_NONE)    // if query point was set, mark that
        queryPointType = QUERYPOINTTYPE_2D;         // 2D point can be used
  }

  /// Set a 3D query point
  /** \param p Query point
  */
  void set3DPoint( WlzDVertex3 p) { 
      queryPoint = p; 
      queryPointType = QUERYPOINTTYPE_3D;
  };

  /// Return the sectioning plane distance
  float getDistance(){ return dist; };

  /// Return the sectioning plane yaw angle
  float getYaw(){ return yaw; };

  /// Return the sectioning plane pitch angle
  float getPitch(){ return pitch; };

  /// Return the sectioning plane roll angle
  float getRoll(){ return roll; };

  /// Return the sectioning scale
  float getScale(){ return scale; };

  /// Return the fixed point for the sectioning plane rotation
  WlzDVertex3 getFixedPoint(){ return fixed; };

  /// Return the second fixed point for the sectioning plane rotation
  WlzDVertex3 getFixedPoint2(){ return fixed2; };

  /// Return the up vector for the sectioning plane rotation 
  WlzDVertex3 getUpVector(){ return up; };

  /// Return the query point
  WlzDVertex3 getQueryPoint(){ return queryPoint; };

  /// Return the query point type
  QueryPointType getQueryPointType(){ return queryPointType; };

  /// Test equality of section defining parameters: dist, yaw, pitch, roll, scale, mode, fixed and up. Query parameters are ignored.
  bool operator==(const ViewParameters& vp) const {
     return dist  == vp.dist  && 
            yaw   == vp.yaw   && 
            pitch == vp.pitch && 
            roll  == vp.roll  && 
            scale == vp.scale && 
            mode  == vp.mode  && 
            WlzGeomVtxEqual3D (fixed, vp.fixed, 0)  && 
            WlzGeomVtxEqual3D (fixed2, vp.fixed2, 0)  && 
            WlzGeomVtxEqual3D (up, vp.up ,0) ;
  }

  /// Test inequality of section defining parameters: dist, yaw, pitch, roll, scale, mode, fixed and up. Query parameters are ignored.
  bool operator!=(const ViewParameters& vp) const {
     return !(*this==vp);
  }
};


#endif
