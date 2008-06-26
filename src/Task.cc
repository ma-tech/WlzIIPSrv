#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _Task_cc[] = "MRC HGU $Id$";
#endif
#endif

/*
    IIP Command Handler Member Functions

    Copyright (C) 2008 Zsolt Husz, Medical research Council, UK.
    Copyright (C) 2006 Ruven Pillay.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "Task.h"
#include "Tokenizer.h"
#include <iostream>
#include <algorithm>


using namespace std;



Task* Task::factory( string type ){

  // Convert the command string to lower case to handle incorrect
  // viewer implementations

  transform( type.begin(), type.end(), type.begin(), ::tolower );

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
  else if( type == "upv" ) return new UPV; // Sectioning up vector
  else if( type == "prl" ) return new PRL; // Sets a 2D point
  else if( type == "pab" ) return new PAB; // Sets a 3D point
  else if( type == "scl" ) return new SCL; // Sets scale
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
  if( imtype != "wlz"){
    session->response->setError( "1 3", argument );
    throw string( "image is not a Woolz object" );
  }
}

void Task::openIfWoolz(){
  string imtype = (*session->image)->getImageType();
  if( imtype == "wlz"){
    (*session->image)->openImage();
  }
}

void QLT::run( Session* session, std::string argument ){

  if( argument.length() ){

    int factor = atoi( argument.c_str() );

    // Check the value is realistic
    if( factor < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }

    session->jpeg->setQuality( factor );
  }

}


void SDS::run( Session* session, std::string argument ){

  if( session->loglevel >= 3 ) *(session->logfile) << "SDS handler reached" << endl;

  // Parse the argument list
  int delimitter = argument.find( "," );
  string tmp = argument.substr( 0, delimitter );
  session->view->xangle = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );

  delimitter = argument.find( "," );
  tmp = argument.substr( 0, delimitter );
  session->view->yangle = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );

  if( session->loglevel >= 2 ) *(session->logfile) << "SDS :: set to " << session->view->xangle << ", "
						   << session->view->yangle << endl;

}


void CNT::run( Session* session, std::string argument ){

  float contrast = atof( argument.c_str() );

  if( session->loglevel >= 2 ) *(session->logfile) << "CNT handler reached" << endl;
  if( session->loglevel >= 3 ) *(session->logfile) << "CNT :: requested contrast adjustment is " << contrast << endl;

  session->view->setContrast( contrast );
}


void WID::run( Session* session, std::string argument ){

  int requested_width = atoi( argument.c_str() );

  if( session->loglevel >= 2 ) *(session->logfile) << "WID handler reached" << endl;
  if( session->loglevel >= 3 ) *(session->logfile) << "WID :: requested width is " << requested_width << endl;

  session->view->setRequestWidth( requested_width );

}


void HEI::run( Session* session, std::string argument ){

  int requested_height = atoi( argument.c_str() );

  if( session->loglevel >= 2 ) *(session->logfile) << "HEI handler reached" << endl;
  if( session->loglevel >= 3 ) *(session->logfile) << "HEI :: requested width is " << requested_height << endl;

  session->view->setRequestHeight( requested_height );

}


void RGN::run( Session* session, std::string argument ){

  Tokenizer izer( argument, "," );
  int i = 0;

  if( session->loglevel >= 2 ) *(session->logfile) << "RGN handler reached" << endl;

  float region[4];
  while( izer.hasMoreTokens() ){
    try{
      region[i++] = atof( izer.nextToken().c_str() );
    }
    catch( const string& error ){
      if( session->loglevel >= 1 ) *(session->logfile) << error << endl;
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

  if( session->loglevel >= 3 ){
    *(session->logfile) << "RGN :: requested region is " << region[0] << ", "
			<< region[1] << ", " << region[2] << ", " << region[3] << endl;
  }

}


void JTLS::run( Session* session, std::string argument ){

  /* The argument is comma separated into 4:
     1) xangle
     2) resolution
     3) tile number
     4) yangle
     This is a legacy command. Clients should use SDS to specity the x,y angle and JTL
     for the resolution and tile number.
  */

  Tokenizer izer( argument, "," );
  int i = 0;

  if( session->loglevel >= 2 ) *(session->logfile) << "JTLS handler reached" << endl;

  int values[4];
  while( izer.hasMoreTokens() ){
    try{
      values[i++] = atoi( izer.nextToken().c_str() );
    }
    catch( const string& error ){
      if( session->loglevel >= 1 ) *(session->logfile) << error << endl;
    }
  }

  if( i == 4 ){
    session->view->xangle = values[0];
    session->view->yangle = values[3];
    char tmp[128];
    snprintf( tmp, 56, "%d,%d", values[1], values[2] );
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

  if( session->loglevel >= 2 ) *(session->logfile) << "SHD handler reached" << endl;

  int values[2];
  while( izer.hasMoreTokens() ){
    try{
      values[i++] = atoi( izer.nextToken().c_str() );
    }
    catch( const string& error ){
      if( session->loglevel >= 1 ) *(session->logfile) << error << endl;
    }
  }

  if( i == 2 ){
    session->view->shaded = true;
    session->view->shade[0] = values[0];
    session->view->shade[1] = values[1];
  }

  if( session->loglevel >= 3 ) *(session->logfile) << "SHD :: requested shade incidence angle is "
						   << values[0] << "," << values[1]  << endl;
}

// Woolz extension commands
void DST::run( Session* session, std::string argument ){

  if( argument.length() ){

    double distance = atof( argument.c_str() );

    // Check the value is realistic
/*    if( distance < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }*/

    session->viewParams->setDistance( distance );
    if( session->loglevel >= 3 ) *(session->logfile) << "DST :: requested Woolz sectioning distance is "
						   << session->viewParams->dist  << endl;
  }
}

void YAW::run( Session* session, std::string argument ){

  if( argument.length() ){

    double value = atof( argument.c_str() );

    // Check the value is realistic
/*    if( distance < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }*/

    session->viewParams->setYaw( value );
    if( session->loglevel >= 3 ) *(session->logfile) << "YAW :: requested Woolz sectioning yaw angle is "
						   << session->viewParams->yaw  << endl;
  }
}

void PIT::run( Session* session, std::string argument ){

  if( argument.length() ){

    double value = atof( argument.c_str() );

    // Check the value is realistic
/*    if( distance < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }*/

    session->viewParams->setPitch( value );
    if( session->loglevel >= 3 ) *(session->logfile) << "PIT :: requested Woolz sectioning pitch angle is "
						   << session->viewParams->pitch  << endl;
  }
}

void ROL::run( Session* session, std::string argument ){

  if( argument.length() ){

    double value = atof( argument.c_str() );

    // Check the value is realistic
/*    if( distance < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }*/

    session->viewParams->setRoll( value );
    if( session->loglevel >= 3 ) *(session->logfile) << "ROL :: requested Woolz sectioning roll angle is "
						   << session->viewParams->roll  << endl;
  }
}

void MOD::run( Session* session, std::string argument ){

  if( argument.length() ){

    // Set if the value is valid 
    if( session->viewParams->setMode( argument) != WLZ_ERR_NONE ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "MOD :: Unknown mode " << argument
			    << ". The mode must be STATUE, UP_IS_UP, FIXED_LINE ZERO_ZETA or ZETA." << endl;
      }
    }

    if( session->loglevel >= 3 ) *(session->logfile) << "MOD :: sectioning mode is "
						   << session->viewParams->mode  << endl;
  }
}



void FXP::run( Session* session, std::string argument ){

  if( argument.length() ){

    WlzDVertex3 point={0,0,0};

    int read=sscanf( argument.c_str(), "%lf,%lf,%lf", &point.vtX, &point.vtY, &point.vtZ);

    // Set if the value is valid 
    if(read!=3){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "FXP :: Incorrect fixed point format " << argument
                            << endl;
      }
    }

    session->viewParams->setFixedPoint( point );
    if( session->loglevel >= 3 ) *(session->logfile) << "FXP :: requested Woolz sectioning fixed point is " << argument << ':' <<'(' << session->viewParams->fixed.vtX  << ',' << session->viewParams->fixed.vtY << ',' << session->viewParams->fixed.vtZ << ')'  << endl;
  }
}


void UPV::run( Session* session, std::string argument ){

  if( argument.length() ){

    WlzDVertex3 point={0,0,-1};


    int read=sscanf( argument.c_str(), "%lf,%lf,%lf", &point.vtX, &point.vtY, &point.vtZ);


    // Set if the value is valid 
    if(read!=3){
      if( session->loglevel >= 2 ){
        *(session->logfile) << "UPV :: Incorrect fixed point format " << argument
                            << endl;
      }
    }

    session->viewParams->setUpVector( point );
    if( session->loglevel >= 3 ) 
        *(session->logfile) << "UPV :: requested Woolz up vector is " << argument << ':' <<'(' << session->viewParams->up.vtX  << ',' << session->viewParams->up.vtY << ',' << session->viewParams->up.vtZ << ')'  << endl;
  }
}

void PRL::run( Session* session, std::string argument ){

  if( argument.length() ){

    int x,y,t;

    int read=sscanf( argument.c_str(), "%d,%d,%d", &t, &x, &y);


    // Set if the value is valid 
    if(read!=3){
      if( session->loglevel >= 2 ){
        *(session->logfile) << "PRL :: Incorrect point of interest format " << argument
                            << endl;
      }
    }
    session->viewParams->setPoint( x, y, t );
    if( session->loglevel >= 3 ) 
          *(session->logfile) << "PRL :: requested Woolz query point is " << argument << " (tile,x,y): " <<'(' << session->viewParams->tile  << ',' << session->viewParams->x << ',' << session->viewParams->y << ") / " <<  session->viewParams->queryPointType   << endl;
  }
}

void PAB::run( Session* session, std::string argument ){

  if( argument.length() ){


    WlzDVertex3 point={0,0,-1};


    int read=sscanf( argument.c_str(), "%lf,%lf,%lf", &point.vtX, &point.vtY, &point.vtZ);


    // Set if the value is valid 
    if(read!=3){
      if( session->loglevel >= 2 ){
        *(session->logfile) << "PAB :: Incorrect query point format " << argument
                            << endl;
      }
    }

    session->viewParams->set3DPoint( point );
    if( session->loglevel >= 3 ) 
        *(session->logfile) << "PAB :: requested Woolz query point  is " << argument << ':' <<'(' << session->viewParams->queryPoint.vtX  << ',' << session->viewParams->queryPoint.vtY << ',' << session->viewParams->queryPoint.vtZ << ") / " <<  session->viewParams->queryPointType   << endl;
  }
}


void SCL::run( Session* session, std::string argument ){

  if( argument.length() ){

    double value = atof( argument.c_str() );

    // Check the value is realistic
/*    if( distance < 0 || factor > 100 ){
      if( session->loglevel >= 2 ){
	*(session->logfile) << "QLT :: JPEG Quality factor of " << argument
			    << " out of bounds. Must be 0-100" << endl;
      }
    }*/

    session->viewParams->setScale( value );
    if( session->loglevel >= 3 ) 
      *(session->logfile) << "SCL :: requested Woolz scale is " << session->viewParams->scale  << endl;
  }
}


