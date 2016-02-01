#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _FIF_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         FIF.cc
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
* Copyright (C) 2005-2006 Ruven Pillay.
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
* \brief
*/

#include "Log.h"
#include "Task.h"
#include "Environment.h"

#include "TPTImage.h"
#include "Cache.h"

using namespace std;

void FIF::run( Session* session, std::string argument ){

  LOG_INFO("FIF handler reached");
  LOG_COND_INFO(command_timer.start());
  // The argument is a path, which may contain spaces or other characters
  // that will be MIME encoded into a suitable format for the URL.
  // So, first decode this path (following code modified from CommonC++ library)

  const char* source = argument.c_str();

  char destination[ 256 ];  // Hopefully we won't get any paths longer than 256 chars!
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


  IIPImage test;

  // Get our image pattern variable
  string filename_pattern = Environment::getFileNamePattern();

  // Get our filesystem prefix
  string filesystem_prefix = Environment::getFileSystemPrefix();

  // Put the image opening into a try block so that we can set
  // a meaningful error
  try{

    // TODO: Try to use a reference to this list, so that we can
    //  keep track of the current sequence between runs

    if(session->imageCache->empty()){
      test = IIPImage( argument );
      test.setFileNamePattern( filename_pattern );
      test.setFileSystemPrefix( filesystem_prefix );
      test.Initialise();
      (*session->imageCache)[argument] = test;
      LOG_INFO("Image cache initialisation");
    }
    else{
      // Cache hit
      if(session->imageCache->find(argument) != session->imageCache->end()){
	test = (*session->imageCache)[ argument ];
	LOG_INFO("Image cache hit. Number of elements: " <<
	          session->imageCache->size());
      }
      else{
	// Cache miss
	test = IIPImage( argument );
	test.setFileNamePattern( filename_pattern );
	test.setFileSystemPrefix( filesystem_prefix );
	test.Initialise();
	LOG_INFO("Image cache miss");
	if(session->imageCache->size() >= 100)
	{
	  session->imageCache->erase(session->imageCache->end());
	}
	(*session->imageCache)[argument] = test;
      }
    }

    /***************************************************************
	      Test for different image types - only TIFF is native for now
    ***************************************************************/

    string imtype = test.getImageType();

    if( imtype == "tif" || imtype == "ptif" ){
      LOG_INFO("FIF :: TIFF image requested");
      *session->image = new TPTImage(test);
    }
    else throw string( "Unsupported image type: " + imtype );

    /*
    else{

#ifdef ENABLE_DL

      // Check our map list for the requested type
      if( moduleList.empty() ){
	throw string( "Unsupported image type: " + imtype );
      }
      else{

	map<string, string> :: iterator mod_it  = moduleList.find( imtype );

	if( mod_it == moduleList.end() ){
	  throw string( "Unsupported image type: " + imtype );
	}
	else{
	  // Construct our dynamic loading image decoder 
	  session->image = new DSOImage( test );
	  (*session->image)->Load( (*mod_it).second );

	  LOG_INFO(imtype << " image requested ... using handler " <<
	            (*session->image)->getDescription());
	}
      }
#else
      throw string( "Unsupported image type: " + imtype );
#endif
    }
    */

    LOG_INFO("FIF :: created image");

    (*session->image)->openImage();
    LOG_INFO("image width is " << (*session->image)->getImageWidth() <<
	      " and height is " << (*session->image)->getImageHeight());
  }
  catch( const string& error ){
    // Unavailable file error code is 1 3
    session->response->setError( "1 3", "FIF" );
    throw error;
  }


  // Reset our angle values
  session->view->xangle = 0;
  session->view->yangle = 90;

  LOG_INFO("FIF :: Total command time " << command_timer.getTime() << "us");
}
