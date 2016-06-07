#ifndef _WLZVIEW_H
#define _WLZVIEW_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _ViewParameters_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         ViewParameters.h
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
* \brief	Storage for session dependent, Woolz specific parameters.
* \ingroup	WlzIIPServer
*/

#include <string.h>

#include <Wlz.h>
#include <WlzExpression.h>

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
* \enum		_RenderModeType
* \ingroup	WlzIIPServer
* \brief	Rendering mode, section of one of WlzProjectIntMode.
*		Typedef: RenderModeType
*/
typedef enum _RenderModeType
{
  RENDERMODE_SECT	= 0,
  RENDERMODE_PROJ_N	= 1,
  RENDERMODE_PROJ_D	= 2,
  RENDERMODE_PROJ_V	= 3
} RenderModeType;

/*!
* \struct	_ImageMapChan
* \ingroup	WlzIIPServer
* \brief	Image value mapping parameters for a single channel.
*/
typedef struct _ImageMapChan
{
  WlzGreyTransformType	type;		/*!< Mapping function. */
  int			il;		/*!< Input lower value. */
  int			iu;		/*!< Input upper value. */
  int			ol;		/*!< Output lower value. */
  int			ou;		/*!< Output upper value. */
  double		p0;		/*!< Gamma (\f$\gamma\f$) or
                                             sigmoid (\f$\mu\f$). */
  double		p1;		/*!< Sigmoid (\f$\sigma\f$). */
} ImageMapChan;

/*!
* \struct	ImageMap
* \ingroup	WlzIIPServer
* \brief	Image value mapping.
*/
typedef struct ImageMap
{
  public:
    ImageMap() {nChan = 0; memset(chan, 0, 4 * sizeof(ImageMapChan));}
    ~ImageMap() {}
    int			parse(const char *str);
    std::string		toString(void) const;
    static std::string  mapTypeToString(WlzGreyTransformType type);
    int			getNChan() const {return(nChan);}
    const ImageMapChan	*getChan(int i)  const  {return(&(chan[i]));}
    WlzObject		*createLUT(WlzErrorNum *dstErr) const;
  private:
    int			nChan;		/*!< Number of channels the map is
					     defined for . */
    ImageMapChan	chan[4];	/*!< Maps for the image channels. */
} ImageMap;

/*!
    Storage class for compound object selection
*/
class CompoundSelector
{
  public:
    /// Constructor
    CompoundSelector(): next(NULL), complexSelection(0), expression(NULL),
                        r(0), g(0), b(0), a(0) {}

    /// Destructor
    ~CompoundSelector()
    {
      if(next)
      {
	delete next;
      }
      next = NULL;
      WlzExpFree(expression);
      expression = NULL;
    }

    /// Data
    CompoundSelector *next;            /*!< The next element */
    int complexSelection;              /*!< Control allowing complex and
    					    costly expressions. */
    WlzExp *expression;		       /*!< Expression for selected object. */
    unsigned char r;                   /*!< red value */
    unsigned char g;                   /*!< green value */
    unsigned char b;                   /*!< blue value */
    unsigned char a;                   /*!< alpha value */
};

/*! 
    Storage class for session dependent, Woolz specific view parameters
*/
class ViewParameters
{
  public:

    double            dist;		/*!< Distance to sectioning plane. */
    double            yaw;              /*!< Yaw angle of sectioning plane
    					     stored here using degrees. */
    double            pitch;            /*!< Pitch angle of sectioning plane 
    					     stored here using degrees. */
    double            roll;             /*!< Roll angle of sectioning plane 
    					     stored here using degrees. */
    double            scale;            /*!< Scale of the sectioning plane. */
    WlzThreeDViewMode mode;             /*!< 3D View Mode of the sectioning
    					     plane. */
    WlzDVertex3       fixed;            /*!< Fixed point .*/
    WlzDVertex3       fixed2;           /*!< Second fixed point. */
    WlzDVertex3       up;               /*!< upvector. */

    double	      depth;		/*!< Rendering depth. */
    RenderModeType    rmd;		/*!< Rendering mode. */

    //selection have no access methods
    int x;                              /*!< Current point x coordinate. */
    int y;                              /*!< Current point y coordinate. */
    int tile;                           /*!< Current point tile. */

    bool alpha;                         /*!< Ask for transparent tile
                                             (for PNG)*/

    WlzDVertex3       queryPoint;       /*!< 3D point for GreyValue enquery. */
    QueryPointType    queryPointType;   /*!< Type of point to be used for
					      GreyValue enquery. */

    ImageMap	      map;              /*!< Image value map. */
    CompoundSelector  *selector;        /*!< List of compound object
                                             selection. */
    CompoundSelector  *lastsel;         /*!< Last element of the selector
    					     list. */

    /// Constructor
    ViewParameters();

    /// Copy constructor
    ViewParameters(const ViewParameters& viewParameters);

    /// Assignement operator
    const ViewParameters& operator= (const ViewParameters& viewParameters);

    /// Test equality of section defining parameters: dist, yaw, pitch, roll,
    /// scale, mode, fixed and up. Query parameters are ignored.
    bool operator==(const ViewParameters& vp) const;

    /// Test inequality of section defining parameters: dist, yaw, pitch, roll,
    /// scale, mode, fixed and up. Query parameters are ignored.
    bool operator!=(const ViewParameters& vp) const { return !(*this==vp); }

    /// Destructor
    ~ViewParameters()
    {
      if(selector)
      {
	delete selector;
      }
    }

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
    /** \param f second fixed point for the sectioning plane rotation in
     ** WLZ_FIXED_LINE_MODE mode*/
    void setFixedPoint2( WlzDVertex3 f ){ fixed2 = f; };

    /** \param u up vector for the sectioning plane rotation */
    void setUpVector( WlzDVertex3 u){ up = u; };

    /// Set the sectioning scale
    /** \param s scale factor*/
    void setScale( double s ){ scale = s; };

    /// Set the alpha channel request
    /** \param a if true alpha channel is generated*/
    void setAlpha( bool  a ){ alpha = a; };

    /// Set the sectioning mode
    WlzErrorNum setMode( string m );

    /// Set the sectioning mode
    /** \param m section mode as WlzThreeDViewMode structure */
    void setMode(WlzThreeDViewMode m){ mode = m; };

    /// Set the render depth
    WlzErrorNum setRenderDepth(string ds);

    /// Set the render depth
    /** \param d render depth */
    void setRenderDepth(double d){depth = d;};

    /// Set the rendering mode
    WlzErrorNum setRenderMode(string m);

    /// Set the rendering mode
    /** \param m section mode as RenderModeType. */
    void setMode(RenderModeType m){rmd = m;};

    /// Set a 2D query point relative to a tile
    void setPoint( int xx, int yy, int tt);

    /// Set a 3D query point
    /** \param p Query point
    */
    void set3DPoint( WlzDVertex3 p)
    { 
      queryPoint = p; 
      queryPointType = QUERYPOINTTYPE_3D;
    };

    /// Set view from fitting a plane.
    void setFromPlaneFit(int nPos, WlzDVertex3 *pos);

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

    /// Return the query point type
    bool getAlpha(){ return alpha; };
};

#endif
