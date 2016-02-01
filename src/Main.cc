#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _Main_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         Main.cc
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
* Copyright (C) 2000-2006 Ruven Pillay
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
* \brief	IIP FCGI server module - Main loop.
* \ingroup	WlzIIPServer
* \bug: 	CVT command may crash for small strides compressed by
* 		JPEGCompressor::CompressStrip.  See JPEGCompressor.cc for
* 		details.
*/

#define _MAIN_CC

#include <ctime>
#include <csignal>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <map>
#include <sys/time.h>


#include <fcgiapp.h>

#include "Log.h"
#include "TPTImage.h"
#include "JPEGCompressor.h"
#include "PNGCompressor.h"
#include "Tokenizer.h"
#include "IIPResponse.h"
#include "View.h"
#include "ViewParameters.h"
#include "Timer.h"
#include "TileManager.h"
#include "Task.h"
#include "Environment.h"
#include "Writer.h"
#include "WlzImage.h"


#ifdef ENABLE_DL
#include "DSOImage.h"
#endif

//#define DEBUG 1

using namespace std;

unsigned long accessCount;

/* Handle a signal - print out some stats and exit
 */
/*!
* \ingroup	WlzIIPServer
* \brief	The WlzIIP server signal handler.
* \param	signal			Signal caught.
*/
void		IIPSignalHandler(int signal)
{
  LOG_FATAL("WlzIIP caught signal " << signal << ". " <<
            "Terminating after " << accessCount << " accesses");
  exit(1);
}

int main( int argc, char *argv[] )
{

  accessCount = 0;

  // Define ourselves a version
#ifdef VERSION
  string version = string(VERSION);
#else
  string version = string("unknown");
#endif

#ifdef WLZ_IIP_LOG
  // Initialise the logging system before anything else
  int loglevel = Environment::getLogLevel();
  string logfile = Environment::getLogFile();
  log4cpp::Appender *logAppender = new log4cpp::FileAppender("FileAppender",
                                                             logfile);
  log4cpp::Layout *logLayout = new log4cpp::BasicLayout();
  log4cpp::Category &logCategory =
                    log4cpp::Category::getInstance(std::string("WlzIIPSrv"));
  logCat = &logCategory;
  logAppender->setLayout(logLayout);
  logCat->setAppender(logAppender);
  logCat->setPriority(loglevel);
  LOG_NOTICE("Server starting with version " << version);
#endif

  // Set up some FCGI items and make sure we are in FCGI mode
#ifndef DEBUG
  FCGX_Request request;
  int listen_socket = 0;
  int usePort = 0;

  if(argv[1] && (string(argv[1]) == "--standalone"))
  {
    string socket = argv[2];
    if(!socket.length())
    {
      LOG_FATAL("No socket specified");
      exit(1);
    }
    listen_socket = FCGX_OpenSocket(socket.c_str(), 10);
    if(listen_socket < 0)
    {
      LOG_FATAL("Unable to open socket '" << socket << "'");
      exit(1);
    }
  } else
  if(argv[1] && (string(argv[1]) == "--port"))
  {
    string port = argv[2];
    if(!port.length())
    {
      LOG_FATAL("No port is specified");
      exit(1);
    }
    if (port[0]!=':')
    {
      port = ':'+ port; 	// make sure port number starts with a ":"
    }
    listen_socket = FCGX_OpenSocket(port.c_str(), 10000);
    if(listen_socket < 0)
    {
      LOG_FATAL("Unable to open port '" << port << "'");
      exit(1);
    }
    LOG_NOTICE("Server started on port '" << port << "'");
    usePort = 1;
  }
  if(FCGX_InitRequest(&request, listen_socket, 0))
  {
    LOG_FATAL("FCGI initialisation failed.");
    exit(1);
  }
  if(FCGX_IsCGI() && (usePort == 0))
  {
    LOG_FATAL("CGI-only mode detected.");
    exit(1);
  }
  else
  {
#ifdef WLZ_IIP_LOG
    if(usePort)
    {
      LOG_INFO("Running in independent server mode");
    }
    else
    {
      LOG_INFO("Running in FCGI mode");
    }
#endif
  }
#endif
  // Set maximum image cache size
  float max_image_cache_size = Environment::getMaxImageCacheSize();
  imageCacheMapType imageCache;
  // Get our image pattern variable
  string filename_pattern = Environment::getFileNamePattern();
  //  Get the filesystem prefix
  string filesystem_prefix = Environment::getFileSystemPrefix();
  //  Get our default quality variable
  int jpeg_quality = Environment::getJPEGQuality();
  //  Get our max CVT size (not respected by Woolz objects)
  int max_CVT = Environment::getMaxCVT();
  LOG_INFO("Setting filesystem prefix to " <<
           filesystem_prefix);
  LOG_INFO("Setting maximum image cache size to " <<
           max_image_cache_size << "MB");
  LOG_INFO("Setting 3D file sequence name pattern to " <<
	   filename_pattern);
  LOG_INFO("Setting default JPEG quality to " << jpeg_quality);
  LOG_INFO("Setting maximum CVT size to " << max_CVT);
  LOG_INFO("Setting maximum view structure cache size to "  <<
	   Environment::getMaxViewStructCacheSize() <<
	   " structures");
  LOG_INFO("Setting maximum object cache size to " <<
	   Environment::getMaxWlzObjCacheSize() << "MB");
  LOG_INFO("Tile size " << Environment::getWlzTileWidth() << " x " <<
	   Environment::getWlzTileHeight());

  // Check for loadable modules, but only if enabled by configure
#ifdef ENABLE_DL
  map <string, string> moduleList;
  string modulePath;
  envpara = getenv( "DECODER_MODULES" );
  if(envpara)
  {
    modulePath = string( envpara );
    // Try to open the module
    Tokenizer izer(modulePath, ",");
    while(izer.hasMoreTokens())
    {
      try
      {
	string token = izer.nextToken();
	DSOImage module;
	module.Load(token);
	string type = module.getImageType();
	moduleList[type] = token;
	LOG_INFO("Loading external module: " << module.getDescription());
      }
      catch(const string& error)
      {
        LOG_ERROR("Failed to load module, Error " << error);
      }
    }
    // Log what's happened
    LOG_NOTICE(moduleList.size() << " external modules loaded");
  }
#endif

  // Set up a signal handler for USR1, TERM and SIGHUP signals
  // - to simplify things, they can all just shutdown the
  //   server. We can rely on mod_fastcgi to restart us.
  // - SIGUSR1 and SIGHUP don't exist on Windows, though. 
#ifndef WIN32
  signal(SIGUSR1, IIPSignalHandler);
  signal(SIGHUP, IIPSignalHandler);
  signal(SIGKILL, IIPSignalHandler);
  signal(SIGINT, IIPSignalHandler);
#endif
  signal(SIGTERM, IIPSignalHandler);

  LOG_INFO("Initialisation Complete.");

  // Set up some timers and create our tile cache
  Timer request_timer;
  Cache tileCache(max_image_cache_size);
  Task* task = NULL;

  // Main FCGI loop
#ifdef DEBUG
  int status = true;
  while(status)
  {
    status = false;
#else
  while( FCGX_Accept_r( &request ) >= 0 )
  {
    FCGIWriter writer( request.out );
#endif
    LOG_COND_INFO(request_timer.start());
    // Declare our image pointer here outside of the try scope
    //  so that we can close the image on exceptions
    IIPImage *image = NULL;
    JPEGCompressor jpeg( jpeg_quality );
    PNGCompressor png;

    // View object for use with the CVT command etc
    View view;
    if(max_CVT != -1)
    {
      view.setMaxSize(max_CVT);
      LOG_INFO("CVT maximum viewport size set to " << max_CVT);
    }

    // Create an IIPResponse object - we use this for the OBJ requests.
    // As the commands return images etc, they handle their own responses.
    IIPResponse response;
    ViewParameters viewParams;
    try
    {

      // Get the query into a string
#ifdef DEBUG
      string request_string = argv[1];
#else
      string request_string = FCGX_GetParam( "QUERY_STRING", request.envp );
#endif

      // Check that we actually have a request string
      if(request_string.length() == 0)
      {
	throw string( "QUERY_STRING not set" );
      }
      LOG_INFO("Full Request is " << request_string);

      // Set up our session data object
      Session session;
      session.image = &image;
      session.response = &response;
      session.view = &view;
      session.viewParams = &viewParams;
      session.jpeg = &jpeg;
      session.png = &png;
      session.imageCache = &imageCache;
      session.tileCache = &tileCache;
      session.out = &writer;

      // Parse up the command list
      list < pair<string,string> > requests;
      list < pair<string,string> > :: const_iterator commands;
      Tokenizer izer(request_string, "&");
      while(izer.hasMoreTokens())
      {
	pair <string,string> p;
	string token = izer.nextToken();
	int n = token.find_first_of("=");
	p.first = token.substr(0, n);
	p.second = token.substr(n + 1, token.length());
	if(p.first.length() && p.second.length())
	{
	  requests.push_back(p);
        }
      }
      int i = 0;
      for(commands = requests.begin(); commands != requests.end(); commands++)
      {
	string command = (*commands).first;
	string argument = (*commands).second;

#ifdef WLZ_IIP_LOG
	++i;
	LOG_INFO("[" << i << "/" << requests.size() <<
	         "]: Command / Argument is " << command << " : " << argument);
#endif
	task = Task::factory( command );
	if(task)
	{
	  task->run(&session, argument);
	  delete task;
	  task = NULL;
	}
	else
	{
	  LOG_WARN("Unsupported command: " << command);
	  // Unsupported command error code is 2 2
	  response.setError("2 2", command);
	}
      }

      ////////// Send out our Errors if necessary ////////////

      // Make sure something has actually been sent to the client
      // If no response has been sent by now, we must have a malformed
      // command.
      if((!response.imageSent()) && (!response.isSet()))
      {
	// Malformed command syntax error code is 2 1
	response.setError( "2 1", request_string );
      }

      // Once we have finished parsing all our OBJ and COMMAND requests
      // send out our response.
      if(response.isSet())
      {
	LOG_INFO("---" << endl << response.formatResponse() << endl << "---");
	if(writer.putS(response.formatResponse().c_str()) == -1)
	{
	  LOG_ERROR("Error sending IIPResponse");
	}
      }

      //////////////// End of try block ////////////////////
    }
    catch( const string& error )
    {
      LOG_ERROR("Error " << error);
      if(response.errorIsSet())
      {
	LOG_INFO("---" << endl << response.formatResponse() << endl << "---");
	if(writer.putS(response.formatResponse().c_str()) == -1)
	{
	  LOG_ERROR("Error sending IIPResponse");
	}
      }
      else
      {
	// Display our advertising banner ;-)
	writer.putS(response.getAdvert(version).c_str());
      }
    }
    catch( ... ) /* Default catch */
    {
      LOG_ERROR("Error: Default Catch: ");
      // Display our advertising banner ;-)
      writer.putS( response.getAdvert( version ).c_str() );

    }
    // Do some cleaning up etc. here after all the potential exceptions
    // have been handled
    if(task)
    {
      delete task;
      task = NULL;
    }
    if(image)
    {
      delete image;
      image = NULL;
    }
    ++accessCount;

    // How long did this request take?
    LOG_INFO("Total Request Time: " << request_timer.getTime() << "us");
    LOG_INFO("Image closed and deleted" << endl << "Server count is " <<
              accessCount);
    ///////// End of FCGI_ACCEPT while loop or for loop in debug mode //////////
  }
  LOG_NOTICE("Terminating after " << accessCount << " iterations");
#ifdef WLZ_IIP_LOG
  log4cpp::Category::shutdown();
#endif
  return(0);
}
