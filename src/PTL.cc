#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _WLZ_cc[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         PTL.cc
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
* \brief	Provides the ptl command of the WlzIIPServer.
* \ingroup	WlzIIPServer
*/

#include "Task.h"

using namespace std;


void PTL::run( Session* session, std::string argument ){

  /* The argument should consist of 2 comma separated values:
     1) resolution
     2) tile number
  */

  if( session->loglevel >= 3 ) (*session->logfile) << "PTL handler reached" << endl;
  session = session;

  int resolution, tile;


  // Time this command
  if( session->loglevel >= 2 ) command_timer.start();


  // Parse the argument list
  int delimitter = argument.find( "," );
  resolution = atoi( argument.substr( 0, delimitter ).c_str() );

  delimitter = argument.find( "," );
  tile = atoi( argument.substr( delimitter + 1, argument.length() ).c_str() );

  session->viewParams->setAlpha(true);

  TileManager tilemanager( session->tileCache, *session->image, session->jpeg, session->png, session->logfile, session->loglevel );
  RawTile rawtile = tilemanager.getTile( resolution, tile, session->view->xangle,
					 session->view->yangle, PNG );

  int len = rawtile.dataLength;

  if( session->loglevel >= 2 ){
    *(session->logfile) << "PTL :: Tile size: " << rawtile.width << " x " << rawtile.height << endl
			<< "PTL :: Channels per sample: " << rawtile.channels << endl
			<< "PTL :: Bits per channel: " << rawtile.bpc << endl
			<< "PTL :: Compressed tile size is " << len << endl;
  }


#ifndef DEBUG
  char buf[1024];
  snprintf( buf, 1024, "Pragma: no-cache\r\n"
	    "Content-length: %d\r\n"
	    "Content-type: image/png\r\n"
	    "Content-disposition: inline;filename=\"ptl.png\""
	    "\r\n\r\n", len );

  session->out->printf( (const char*) buf );
#endif


  if( session->out->putStr( (const char*) rawtile.data, len ) != len ){
    if( session->loglevel >= 1 ){
      *(session->logfile) << "PNG :: Error writing png tile" << endl;
    }
  }

  session->out->printf( "\r\n" );

  if( session->out->flush() == -1 ) {
    if( session->loglevel >= 1 ){
      *(session->logfile) << "PNG :: Error flushing png tile" << endl;
    }
  }


  // Inform our response object that we have sent something to the client
  session->response->setImageSent();

  // Total JTLS response time
  if( session->loglevel >= 2 ){
    *(session->logfile) << "PNG :: Total command time " << command_timer.getTime() << " microseconds" << endl;
  }

}
