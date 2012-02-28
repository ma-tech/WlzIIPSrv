#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WLZ_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WLZ.cc
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
* \brief	Provides the WLZ command of the WlzIIPServer.
*               Implementation is based on TPTImage class
* \ingroup	WlzIIPServer
*/

#include <limits.h>

#include "Log.h"
#include "Task.h"
#include "Environment.h"

#include "WlzImage.h"
#include "Cache.h"


using namespace std;


/*! 
* \ingroup      WlzIIPServer
* \brief        WLZ command handler.
*
* \return       void
* \param        session     Pointer to the current session settings
* \param        argument    Command string (i.e. filename).
* \par      Source:
*                WLZ.cc
*/
void WLZ::run( Session* session, std::string argument ){

  LOG_INFO("WLZ handler reached");
  LOG_COND_INFO(command_timer.start());
  // The argument is a path, which may contain spaces or other characters
  // that will be MIME encoded into a suitable format for the URL.
  // So, first decode this path (following code modified from CommonC++ library)
  const char* source = argument.c_str();
  char destination[PATH_MAX];
  char *dest = destination;
  char* ret = dest;
  char hex[3];

  while( *source ){
    switch( *source ){

    case '+':
      *(dest++) = ' ';
      break;

    case '%':
      // NOTE: wrong input can finish with "...%" giving
      // buffer overflow, cut string here
      if(source[1]){
	hex[0] = source[1];
	++source;
	if(source[1]){
	  hex[1] = source[1];
	  ++source;
	}
	else
	  hex[1] = 0;
      }
      else hex[0] = hex[1] = 0;

      hex[2] = 0;
      *(dest++) = (char) strtol(hex, NULL, 16);
      break;

    default:
      *(dest++) = *source;

    }
    ++source;
  }

  *dest = 0;
  argument = string( ret );

  WlzImage *test = new WlzImage( argument ); ;

  // Get our image pattern variable
  //string filename_pattern = Environment::getFileNamePattern();


  // Put the image opening into a try block so that we can set
  // a meaningful error
  try{
    //test->setFileNamePattern( filename_pattern );  //no pattern needed for Wlz objects
    // initialise object
    test->Initialise();

    /***************************************************************
      Test for different image types - only TIFF is native for now
    ***************************************************************/

    string imtype = test->getImageType();

    if( imtype == "wlz"){
      LOG_INFO("WLZ :: Woolz image requested");

      //set current view parameters
      test->setView( session->viewParams );

      *session->image = test;
    }
    else if( imtype == "gz"){
      LOG_INFO("WLZ :: Gziped Woolz image requested");

      //set current view parameters
      test->setView( session->viewParams );

      *session->image = test;
    }
    else throw string( "WLZ: Unsupported image type: " + imtype );

    LOG_INFO("WLZ :: created image");
  }
  catch( const string& error ){
    // Unavailable file error code is 1 3
    session->response->setError( "1 3", "WLZ" );
    throw error;
  }


  // Reset our angle values
  session->view->xangle = 0;
  session->view->yangle = 90;

	  
  LOG_INFO("WLZ :: Total command time " << command_timer.getTime() << "us");
}
