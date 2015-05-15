#ifndef _WLZVIEW_H
#define _WLZVIEW_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _ViewParameters_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         ViewParameters.h
* \author       Zsolt Husz
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
    CompoundSelector(): next(NULL), expression(NULL), r(0), g(0), b(0), a(0) {}

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

    double            dist;		/*!< distance to sectioning plane */
    double            yaw;              /*!< yaw angle of sectioning plane */
    double            pitch;            /*!< pitch angle of sectioning plane */
    double            roll;             /*!< roll angle of sectioning plane */
    double            scale;            /*!< scale of the sectioning plane */
    WlzThreeDViewMode mode;             /*!< 3D View Mode of sectioning plane */
    WlzDVertex3       fixed;            /*!< fixed point */
    WlzDVertex3       fixed2;           /*!< second fixed point */
    WlzDVertex3       up;               /*!< upvector */

    double	      depth;		/*!< rendering depth */
    RenderModeType    rmd;		/*!< rendering mode */

    //selection have no access methods
    int x;                              /*!< current point x coordinate */
    int y;                              /*!< current point y coordinate */
    int tile;                           /*!< current point tile */

    bool alpha;                         /*!< ask for transparent tile
                                             (for PNG)*/

    WlzDVertex3       queryPoint;       /*!< 3D point for GreyValue enquery */
    QueryPointType    queryPointType;   /*!< Type of point to be used for
					      GreyValue enquery */

    ImageMap	      map;              /*!< Image value map. */
    CompoundSelector  *selector;        /*!< list of compound object
                                             selection */
    CompoundSelector  *lastsel;         /*!< last element of the selector
    					     list */

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
      depth           = 0.0;
      rmd	      = RENDERMODE_SECT;
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
    };

    /// Copy constructor
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
      rmd	      = viewParameters.rmd;
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
    };

    /// assignement operator
    const ViewParameters& operator= (const ViewParameters& viewParameters)
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
      return *this;
    };

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
    /** \param m section mode as case-insensitive string. Accepted values
     ** are: STATUE, UP_IS_UP, FIXED_LINE, ZERO_ZETA and ZETA */
    WlzErrorNum setMode( string m )
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
    };

    /// Set the sectioning mode
    /** \param m section mode as WlzThreeDViewMode structure */
    void setMode(WlzThreeDViewMode m){ mode = m; };

    /// Set the render depth
    /** \param ds render depth string */
    WlzErrorNum setRenderDepth(string ds)
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
    };

    /// Set the render depth
    /** \param d render depth */
    void setRenderDepth(double d){depth = d;};

    /// Set the rendering mode
    /** \param m section mode as case-insensitive string. Accepted values are:
      SECT, PRJN, PRJD and PRJV. */
    WlzErrorNum setRenderMode(string m)
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

    /// Set the rendering mode
    /** \param m section mode as RenderModeType. */
    void setMode(RenderModeType m){rmd = m;};

    /// Set a 2D query point relative to a tile
    /** \param xx current point x coordinate
	\param yy current point y coordinate
	\param tt current point tile number
    */
    void setPoint( int xx, int yy, int tt)
    {
      x = xx; y = yy; tile = tt;
      if(queryPointType == QUERYPOINTTYPE_NONE) // if query point was set, mark it
      {
	queryPointType = QUERYPOINTTYPE_2D;   // 2D point can be used
      }
    }

    /// Set a 3D query point
    /** \param p Query point
    */
    void set3DPoint( WlzDVertex3 p)
    { 
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

    /// Return the query point type
    bool getAlpha(){ return alpha; };


    /// Test equality of section defining parameters: dist, yaw, pitch, roll,
    /// scale, mode, fixed and up. Query parameters are ignored.
    bool operator==(const ViewParameters& vp) const
    {
       return dist  == vp.dist  && 
	      yaw   == vp.yaw   && 
	      pitch == vp.pitch && 
	      roll  == vp.roll  && 
	      scale == vp.scale && 
	      mode  == vp.mode  && 
	      alpha == vp.alpha  && 
	      WlzGeomVtxEqual3D (fixed, vp.fixed, 0)  && 
	      WlzGeomVtxEqual3D (fixed2, vp.fixed2, 0)  && 
	      WlzGeomVtxEqual3D (up, vp.up ,0) ;
    }

    /// Test inequality of section defining parameters: dist, yaw, pitch, roll,
    /// scale, mode, fixed and up. Query parameters are ignored.
    bool operator!=(const ViewParameters& vp) const { return !(*this==vp); }
};

#endif
