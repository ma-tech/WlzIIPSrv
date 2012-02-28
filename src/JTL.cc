#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _JTL_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         JTL.cc
* \author       Ruven Pillay, Zsolt Husz, Bill Hill
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
* \brief	IIP JTLS Command Handler Class Member Function.
* \ingroup    	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"

using namespace std;


void JTL::run( Session* session, std::string argument ){
  /* The argument should consist of 2 comma separated values:
     1) resolution
     2) tile number
  */
  LOG_INFO("JTL handler reached");
  session = session;
  int resolution, tile;
  LOG_COND_INFO(command_timer.start());
  // Parse the argument list
  int delimitter = argument.find( "," );
  resolution = atoi( argument.substr( 0, delimitter ).c_str() );

  delimitter = argument.find( "," );
  tile = atoi( argument.substr( delimitter + 1, argument.length() ).c_str() );


  TileManager tilemanager( session->tileCache, *session->image, session->jpeg, session->png);
  RawTile rawtile = tilemanager.getTile( resolution, tile, session->view->xangle,
					 session->view->yangle, JPEG );

  int len = rawtile.dataLength;

  LOG_INFO("JTL :: Tile size: " <<
            rawtile.width << " x " << rawtile.height << endl <<
	    "JTL :: Channels per sample: " << rawtile.channels << endl <<
            "JTL :: Bits per channel: " << rawtile.bpc << endl << 
            "JTL :: Compressed tile size is " << len);

#ifndef DEBUG
  char buf[1024];
  snprintf(buf, 1024, "Pragma: no-cache\r\n"
	   "Content-length: %d\r\n"
	   "Content-type: image/jpeg\r\n"
	   "Content-disposition: inline;filename=\"jtl.jpg\""
	   "\r\n\r\n", len);

  session->out->printf((const char*) buf);
#endif

  if(session->out->putStr((const char* )rawtile.data, len) != len){
    LOG_ERROR("JTL :: Error writing jpeg tile");
  }

//  session->out->printf( "\r\n" ); //causes incorrect packet length, that results in crash, Z Husz 16/04/2010

  if( session->out->flush() == -1 ) {
    LOG_ERROR("JTL :: Error flushing jpeg tile");
  }


  // Inform our response object that we have sent something to the client
  session->response->setImageSent();

  // Total JTLS response time
  LOG_INFO("JTL :: Total command time " << command_timer.getTime() << "us");
}
