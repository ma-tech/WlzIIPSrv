#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _PTL_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         PTL.cc
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
* \brief	Provides the ptl command of the WlzIIPServer.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"

using namespace std;

void PTL::run( Session* session, std::string argument)
{
  /* The argument should consist of 2 comma separated values:
     1) resolution
     2) tile number
  */
  LOG_INFO("PTL handler reached");
  session = session;
  int resolution, tile;
  // Time this command
  LOG_COND_INFO(command_timer.start());
  // Parse the argument list
  int delimitter = argument.find( "," );
  resolution = atoi( argument.substr( 0, delimitter ).c_str() );
  delimitter = argument.find( "," );
  tile = atoi( argument.substr( delimitter + 1, argument.length() ).c_str() );
  session->viewParams->setAlpha(true);
  TileManager tilemanager(session->tileCache, *session->image, session->jpeg,
                          session->png);
  RawTile rawtile = tilemanager.getTile(resolution, tile,
                                        session->view->xangle,
					session->view->yangle, PNG );
  int len = rawtile.dataLength;
  LOG_INFO("PTL :: Tile size: " << rawtile.width << " x " << rawtile.height);
  LOG_INFO("PTL :: Channels per sample: " << rawtile.channels);
  LOG_INFO("PTL :: Bits per channel: " << rawtile.bpc);
  LOG_INFO("PTL :: Compressed tile size is " << len);

#ifndef INFO
  char buf[1024];
  snprintf( buf, 1024, "Pragma: no-cache\r\n"
	    "Content-length: %d\r\n"
	    "Content-type: image/png\r\n"
	    "Content-disposition: inline;filename=\"ptl.png\""
	    "\r\n\r\n", len );
  session->out->printf( (const char*) buf );
#endif
  if(session->out->putStr((const char* )rawtile.data, len) != len){
    LOG_ERROR("PNG :: Error writing png tile");
  }
  //  session->out->printf( "\r\n" );
  //  causes incorrect packet length, that results in crash, Z Husz 16/04/2010
  if( session->out->flush() == -1 ) {
    LOG_ERROR("PNG :: Error flushing png tile");
  }
  // Inform our response object that we have sent something to the client
  session->response->setImageSent();
  // Total JTLS response time
  LOG_INFO("PNG :: Total command time " << command_timer.getTime() << "us");
}
