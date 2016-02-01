#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _Environment_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         Environment.h
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
* \brief	Defines the Environment class for reading WlzIIP
* 		environment variables.
*/

// Default values
#define LOGFILE 		"/tmp/iipsrv.log"
#define LOGLEVEL 		"WARN"
#define MAX_IMAGE_CACHE_SIZE 	10.0
#define MAX_VIEW_STRUCT_CACHE_COUNT 1024
#define MAX_VIEW_STRUCT_CACHE_SIZE 1024
#define MAX_WLZOBJ_CACHE_COUNT 	1024
#define MAX_WLZOBJ_CACHE_SIZE 	1024 /* in MB */
#define FILESYSTEM_PREFIX       ""
#define FILENAME_PATTERN 	"_pyr_"
#define JPEG_QUALITY 		75
#define MAX_CVT 		5000

#define WLZ_TILE_HEIGHT		100
#define WLZ_TILE_WIDTH 		100

#include <string>
#include "Log.h"


/// Class to obtain environment variables
class Environment {

 public:

  static std::string getLogFile(){
    char* envpara = getenv( "LOGFILE" );
    if( envpara ) return std::string( envpara );
    else return LOGFILE;
  }

  static int getLogLevel()
  {
#ifdef WLZ_IIP_LOG
    int	i;
    int lev = log4cpp::Priority::WARN;
    const char *levStr = "ERROR";
    const char *levTbl[10] = {"EMERG", "FATAL",  "ALERT", "CRIT",  "ERROR",
                              "WARN",  "NOTICE", "INFO",  "DEBUG", "NOTSET"};
    int priTbl[10] = {log4cpp::Priority::EMERG,
		      log4cpp::Priority::FATAL,
		      log4cpp::Priority::ALERT,
		      log4cpp::Priority::CRIT,
		      log4cpp::Priority::ERROR,
		      log4cpp::Priority::WARN,
		      log4cpp::Priority::NOTICE,
		      log4cpp::Priority::INFO,
		      log4cpp::Priority::DEBUG,
		      log4cpp::Priority::NOTSET};
    char *p = getenv("LOGLEVEL");
    if(p)
    {
      for(i = 0; i < 10; ++i)
      {
        if(strncmp(p, levTbl[i], 4) == 0)
	{
	  levStr = levTbl[i];
	  break;
	}
      }
      for(i = 0; i < 10; ++i)
      {
        if(strncmp(levStr, levTbl[i], 4) == 0)
	{
	  lev = priTbl[i];
	  break;
	}
      }
    }
#else
    int lev = 0;
#endif
    return(lev);
  }

  static float getMaxImageCacheSize(){
    float max_image_cache_size = MAX_IMAGE_CACHE_SIZE;
    char* envpara = getenv( "MAX_IMAGE_CACHE_SIZE" );
    if( envpara ){
      max_image_cache_size = atof( envpara );
    }
    return max_image_cache_size;
  }

  static int getMaxViewStructCacheCount(){
    int max_viewstruct_cache_count = MAX_VIEW_STRUCT_CACHE_COUNT;
    char* envpara = getenv( "MAX_VIEW_STRUCT_CACHE_COUNT" );
    if( envpara ){
      max_viewstruct_cache_count = atoi( envpara );
    }
    return max_viewstruct_cache_count;
  }

  static int getMaxViewStructCacheSize(){
    int max_viewstruct_cache_size = MAX_VIEW_STRUCT_CACHE_SIZE;
    char* envpara = getenv( "MAX_VIEW_STRUCT_CACHE_SIZE" );
    if( envpara ){
      max_viewstruct_cache_size = atoi( envpara );
    }
    return max_viewstruct_cache_size;
  }

  static int getMaxWlzObjCacheCount(){
    unsigned int max_object_cache_count = MAX_WLZOBJ_CACHE_COUNT;
    char* envpara = getenv( "MAX_WLZOBJ_CACHE_COUNT" );
    if( envpara ){
      max_object_cache_count = atoi( envpara );
    }
    return max_object_cache_count;
  }

  static int getMaxWlzObjCacheSize(){
    int max_object_cache_size = MAX_WLZOBJ_CACHE_SIZE;
    char* envpara = getenv( "MAX_WLZOBJ_CACHE_SIZE" );
    if( envpara ){
      max_object_cache_size = atoi( envpara );
    }
    return max_object_cache_size;
  }

  static std::string getFileSystemPrefix(){
    char* envpara = getenv( "FILESYSTEM_PREFIX" );

    std::string filesystem_prefix; 
    if(envpara){ 
      filesystem_prefix = std::string( envpara ); 
    } 
    else{
      filesystem_prefix = FILESYSTEM_PREFIX; 
    }
    return(filesystem_prefix);
  }

  static std::string getFileNamePattern(){
    char* envpara = getenv( "FILENAME_PATTERN" );
    std::string filename_pattern;
    if( envpara ){
      filename_pattern = std::string( envpara );
    }
    else filename_pattern = FILENAME_PATTERN;

    return filename_pattern;
  }

  static int getWlzTileWidth(){
    int tile_width = WLZ_TILE_WIDTH;
    char* envpara = getenv( "WLZ_TILE_WIDTH" );
    if( envpara ){
      tile_width  = atoi( envpara );
    }
    return tile_width ;
  }

  static int getWlzTileHeight(){
    int tile_height = WLZ_TILE_HEIGHT;
    char* envpara = getenv( "WLZ_TILE_HEIGHT" );
    if( envpara ){
      tile_height  = atoi( envpara );
    }
    return tile_height ;
  }

  static int getJPEGQuality(){
    char* envpara = getenv( "JPEG_QUALITY" );
    int jpeg_quality;
    if( envpara ){
      jpeg_quality = atoi( envpara );
      if( jpeg_quality > 100 ) jpeg_quality = 100;
      if( jpeg_quality < 1 ) jpeg_quality = 1;
    }
    else jpeg_quality = JPEG_QUALITY;

    return jpeg_quality;
  }


  static int getMaxCVT(){
    char* envpara = getenv( "MAX_CVT" );
    int max_CVT;
    if( envpara ){
      max_CVT = atoi( envpara );
      if( max_CVT < 64 ) max_CVT = 64;
      if( max_CVT == -1 ) max_CVT = -1;
    }
    else max_CVT = MAX_CVT;

    return max_CVT;
  }

};

#endif
