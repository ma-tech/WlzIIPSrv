#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _Task_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         Task.cc
* \author       Ruven Pillay, Zsolt Husz, Bill Hill
* \date         February 2012
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C) 2006 Ruven Pillay.
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
* \brief	IIP Command Handler Member Functions
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"
#include "Tokenizer.h"
#include <iostream>
#include <algorithm>

using namespace std;

Task* Task::factory(std::string type)
{
  // Convert the command string to lower case to handle incorrect
  // viewer implementations

  transform(type.begin(), type.end(), type.begin(), ::tolower);

  if( type == "obj" ) return new OBJ;
  else if( type == "fif" ) return new FIF;
  else if( type == "qlt" ) return new QLT;
  else if( type == "sds" ) return new SDS;
  else if( type == "cnt" ) return new CNT;
  else if( type == "wid" ) return new WID;
  else if( type == "hei" ) return new HEI;
  else if( type == "rgn" ) return new RGN;
  else if( type == "til" ) return new TIL;
  else if( type == "jtl" ) return new JTL;
  else if( type == "jtls" ) return new JTLS;
  else if( type == "icc" ) return new ICC;
  else if( type == "cvt" ) return new CVT;
  else if( type == "shd" ) return new SHD;

  //Extensions for Woolz
  else if( type == "wlz" ) return new WLZ; // Woolz command
  else if( type == "dst" ) return new DST; // Sectioning distance
  else if( type == "yaw" ) return new YAW; // Sectioning yaw angle
  else if( type == "pit" ) return new PIT; // Sectioning pitch angle
  else if( type == "rol" ) return new ROL; // Sectioning roll angle
  else if( type == "mod" ) return new MOD; // Sectioning mode
  else if( type == "fxp" ) return new FXP; // Sectioning fixed point
  else if( type == "fxt" ) return new FXT; // Sectioning second fixed point
  else if( type == "upv" ) return new UPV; // Sectioning up vector
  else if( type == "dpt" ) return new DPT; // Rendering depth
  else if( type == "rmd" ) return new RMD; // Rendering mode
  else if( type == "prl" ) return new PRL; // Sets a 2D point
  else if( type == "pab" ) return new PAB; // Sets a 3D point
  else if( type == "scl" ) return new SCL; // Sets scale
  else if( type == "ptl" ) return new PTL; // PNG tile request, equivalent to JTL
  else if( type == "sel" ) return new SEL; // Selection command for compound objects
  else if( type == "map" ) return new MAP; // Map image values
  else return NULL;
}


void Task::checkImage(){
  if( !*(session->image) ){
    session->response->setError( "1 3", argument );
    throw string( "image not set" );
  }
}


void Task::checkIfWoolz(){
  string imtype = (*session->image)->getImageType();
  if( imtype != "wlz" && imtype != "gz"){
    session->response->setError( "1 3", argument );
    throw string( "image is not a Woolz object" );
  }
}

void Task::openIfWoolz(){
  string imtype = (*session->image)->getImageType();
  if( imtype == "wlz" || imtype == "gz"){
    (*session->image)->openImage();
  }
}

void QLT::run( Session* session, std::string argument ){
  if( argument.length() ){

    int factor = atoi( argument.c_str() );

    // Check the value is realistic
    if((factor < 0) || (factor > 100))
    {
      LOG_WARN("QLT :: JPEG Quality factor of " << argument <<
	       " out of bounds. Must be 0-100");
    }
    session->jpeg->setQuality(factor);
  }
}


void SDS::run( Session* session, std::string argument ){

  LOG_INFO("SDS handler reached");
  // Parse the argument list
  int delimitter = argument.find( "," );
  string tmp = argument.substr( 0, delimitter );
  session->view->xangle = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );

  delimitter = argument.find( "," );
  tmp = argument.substr( 0, delimitter );
  session->view->yangle = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );
  LOG_INFO("SDS :: set to " <<
            session->view->xangle << ", " << session->view->yangle);
}


void CNT::run( Session* session, std::string argument ){
  float contrast = atof( argument.c_str() );
  LOG_INFO("CNT handler reached, requested contrast adjustment is " <<
            contrast);
  session->view->setContrast( contrast );
}


void WID::run( Session* session, std::string argument ){
  int requested_width = atoi( argument.c_str() );
  LOG_INFO("WID handler reached, requested width is " << requested_width);
  session->view->setRequestWidth( requested_width );
}


void HEI::run( Session* session, std::string argument ){
  int requested_height = atoi( argument.c_str() );
  LOG_INFO("HEI handler reached, requested height is " << requested_height);
  session->view->setRequestHeight( requested_height );
}


void RGN::run( Session* session, std::string argument ){
  Tokenizer izer( argument, "," );
  int i = 0;
  LOG_INFO("RGN handler reached");
  float region[4];
  while( izer.hasMoreTokens() ){
    try{
      region[i++] = atof( izer.nextToken().c_str() );
    }
    catch( const string& error ){
      LOG_ERROR("RGN " << error);
    }
  }
  // Only load this information if our argument was correctly parsed to
  // give 4 values
  if( i == 4 ){
    session->view->setViewLeft( region[0] );
    session->view->setViewTop( region[1] );
    session->view->setViewWidth( region[2] );
    session->view->setViewHeight( region[3] );
  }
  LOG_INFO("RGN :: requested region is " <<
            region[0] << ", " << region[1] << ", " << region[2] << ", " <<
	    region[3]);
}


void JTLS::run( Session* session, std::string argument ){
  /* The argument is comma separated into 4:
     1) xangle
     2) resolution
     3) tile number
     4) yangle
     This is a legacy command. Clients should use SDS to specity the x,y angle
     and JTL for the resolution and tile number.
  */

  Tokenizer izer( argument, "," );
  int i = 0;

  LOG_INFO("JTLS handler reached");
  int values[4];
  while( izer.hasMoreTokens() ){
    try{
      values[i++] = atoi( izer.nextToken().c_str() );
    }
    catch(const string& error){
      LOG_ERROR("JTLS Error " << error);
    }
  }
  if( i == 4 ){
    session->view->xangle = values[0];
    session->view->yangle = values[3];
    char tmp[128];
    snprintf( tmp, 128, "%d,%d", values[1], values[2] );
    JTL jtl;
    jtl.run( session, tmp );
  }
}


void SHD::run( Session* session, std::string argument ){

  /* The argument is comma separated into the 3D angles of incidence of the
     light source in degrees for the angle in the horizontal plane from 12 o'clock
     and downwards in the vertical plane, where 0 represents a source pointing
     horizontally
  */

  Tokenizer izer( argument, "," );
  int i = 0;
  LOG_INFO("SHD handler reached");
  int values[2];
  while( izer.hasMoreTokens() ){
    try{
      values[i++] = atoi( izer.nextToken().c_str() );
    }
    catch(const string& error){
      LOG_ERROR("SHD Error " << error);
    }
  }
  if( i == 2 ){
    session->view->shaded = true;
    session->view->shade[0] = values[0];
    session->view->shade[1] = values[1];
  }
  LOG_INFO("SHD :: requested shade incidence angle is " <<
            values[0] << "," << values[1]);
}

// Woolz extension commands
void DST::run( Session* session, std::string argument ){
  if( argument.length() ){
    double distance = atof( argument.c_str() );
    session->viewParams->setDistance( distance );
    LOG_INFO("DST :: requested Woolz sectioning distance is " <<
	      session->viewParams->dist);
  }
}

void YAW::run( Session* session, std::string argument ){
  if(argument.length())
  {
    double value = atof( argument.c_str());
    // Check the value is realistic
    if((value < 0) || (value > 360)){
      LOG_WARN("YAW :: Yaw of" << argument << 
	       " out of bounds. Must be 0-360 degrees.");
      value = (value < 0)? 0: 360;
    }
    session->viewParams->setYaw(value);
    LOG_INFO("YAW :: Woolz sectioning yaw angle set to " <<
              session->viewParams->yaw);
  }
}

void PIT::run( Session* session, std::string argument ){
  if( argument.length() ){
    double value = atof( argument.c_str() );
    // Check the value is realistic
    if((value < 0) || (value > 180)){
      LOG_WARN("PIT :: Pitch of" << argument <<
	       " out of bounds. Must be 0-180 degrees.");
      value = (value < 0)? 0: 180;
    }
    session->viewParams->setPitch(value);
    LOG_INFO("PIT :: Woolz sectioning pitch angle set to " <<
	      session->viewParams->pitch);
  }
}

void ROL::run( Session* session, std::string argument ){
  if( argument.length() ){
    double value = atof( argument.c_str() );
    // Check the value is realistic
    if((value < 0) || (value > 360)){
      LOG_WARN("ROL :: Roll of" << argument <<
	       " out of bounds. Must be 0-360 degrees.");
      value = (value < 0)? 0: 360;
    }
    session->viewParams->setRoll(value);
    LOG_INFO("ROL :: Woolz sectioning roll angle set to " <<
	      session->viewParams->roll);
  }
}

void MOD::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    // Set if the value is valid 
    if(session->viewParams->setMode( argument) != WLZ_ERR_NONE)
    {
      LOG_WARN("MOD :: Unknown mode " << argument <<
	  ". The mode must be one of STATUE, UP_IS_UP, FIXED_LINE ZERO_ZETA" <<
	  " or ZETA.");
    }
    else
    {
      LOG_INFO("MOD :: Woolz sectioning mode set to " <<
                session->viewParams->mode);
    }
  }
}

void FXP::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    WlzDVertex3 point={0,0,0};
    int read=sscanf(argument.c_str(), "%lf,%lf,%lf",
                    &point.vtX, &point.vtY, &point.vtZ);
    // Set if the value is valid 
    if(read != 3){
      LOG_WARN("FXP :: Incorrect fixed point format " << argument);
    }
    else
    {
      session->viewParams->setFixedPoint(point);
      LOG_INFO("FXP :: Woolz sectioning fixed point set to " << argument <<
		':' <<'(' << session->viewParams->fixed.vtX  << ',' <<
		session->viewParams->fixed.vtY << ',' <<
		session->viewParams->fixed.vtZ << ')');
    }
  }
}

void FXT::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    WlzDVertex3 point={0,0,0};
    int read = sscanf(argument.c_str(),
                      "%lf,%lf,%lf", &point.vtX, &point.vtY, &point.vtZ);
    // Set if the value is valid 
    if(read != 3)
    {
      LOG_WARN("FXT :: Incorrect fixed point format " << argument);
    }
    else
    {
      session->viewParams->setFixedPoint2(point);
      LOG_INFO("FXT :: Woolz second sectioning fixed point set to " <<
                argument << ':' <<'(' << session->viewParams->fixed2.vtX  <<
		',' << session->viewParams->fixed2.vtY << ',' <<
	        session->viewParams->fixed2.vtZ << ')');
    }
  }
}


void UPV::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    WlzDVertex3 point={0,0,-1};
    int read = sscanf(argument.c_str(), "%lf,%lf,%lf",
                      &point.vtX, &point.vtY, &point.vtZ);
    // Set if the value is valid 
    if(read != 3)
    {
      LOG_WARN("UPV :: Incorrect fixed point format " << argument);
    }
    else
    {
      session->viewParams->setUpVector(point);
      LOG_INFO("UPV :: Woolz up vector set to " << argument << ':' <<'(' <<
	        session->viewParams->up.vtX  << ',' <<
	        session->viewParams->up.vtY << ',' <<
	        session->viewParams->up.vtZ << ')');
    }
  }
}

void DPT::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    // Set if the value is valid 
    if(session->viewParams->setRenderDepth(argument) != WLZ_ERR_NONE)
    {
      LOG_WARN("DPT :: Invalid range " << argument <<
	  ". The render mode must a posative floating point number.");
    }
    else
    {
      LOG_INFO("DPT :: Woolz rendering depth set to " <<
                session->viewParams->depth);
    }
  }
}

void RMD::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    // Set if the value is valid 
    if(session->viewParams->setRenderMode(argument) != WLZ_ERR_NONE)
    {
      LOG_WARN("RMD :: Unknown mode " << argument <<
	  ". The render mode must be one of SECT, PROJ_N, PROJ_D or PROJ_V.");
    }
    else
    {
      LOG_INFO("RMD :: Woolz rendering mode set to " <<
                session->viewParams->rmd);
    }
  }
}

void PRL::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    int x,y,t;
    int read = sscanf( argument.c_str(), "%d,%d,%d", &t, &x, &y);
    // Set if the value is valid 
    if(read != 3)
    {
      LOG_WARN("PRL :: Incorrect point of interest format " << argument);
    }
    else
    {
      if(t == -1)
      {
        t=0;  //use display coordinates, same as tile 0 coordinates
      }
      // Check the value is valid
      if(t <= -1)
      {
        LOG_WARN("PRL :: tile must be greater than -1: " << argument);
      }
      else
      {
        session->viewParams->setPoint(x, y, t);
        LOG_INFO("PRL :: requested Woolz query point is " << argument <<
	          " (tile,x,y): " <<'(' << session->viewParams->tile  << ',' <<
	          session->viewParams->x << ',' << session->viewParams->y <<
	          ") / " <<  session->viewParams->queryPointType);
      }
    }
  }
}

void PAB::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    WlzDVertex3 point={0,0,-1};
    int read = sscanf(argument.c_str(), "%lf,%lf,%lf",
                      &point.vtX, &point.vtY, &point.vtZ);
    // Set if the value is valid 
    if(read != 3)
    {
      LOG_WARN("PAB :: Incorrect query point format " << argument);
    }
    else
    {
      session->viewParams->set3DPoint(point);
      LOG_INFO("PAB :: Woolz query point  set to " << argument <<
	  ':' <<'(' << session->viewParams->queryPoint.vtX  << ',' <<
	  session->viewParams->queryPoint.vtY <<
	  ',' << session->viewParams->queryPoint.vtZ << ") / " <<
	  session->viewParams->queryPointType);
    }
  }
}


void SCL::run(Session* session, std::string argument)
{
  if(argument.length())
  {
    double value = atof(argument.c_str());
    // Check the value is realistic
    if(value  <= 0)
    {
      LOG_WARN("SCL :: Scale must be positive: " << argument);
    }
    else
    {
      session->viewParams->setScale(value);
      LOG_INFO("SCL :: Woolz scale set to " << session->viewParams->scale);
    }
  }
}


