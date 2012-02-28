#ifndef _WLZOBJECTCACHE_H
#define _WLZOBJECTCACHE_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzObjectCache_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzObjectCache.h
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
* \brief	The caching mechanism for Woolz Objects.
* \ingroup	WlzIIPServer
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <string>
#include <Wlz.h>
#include "RawTile.h"
#include "Environment.h"

/*!
* \struct	_WlzObjCacheEntry
* \ingroup	WlzIIPServer
* \brief	Cache entry data structure for the WlzIIP Woolz object cache.
*/
typedef struct _WlzObjCacheEntry
{
  char			*str;		/*!< Object identification string, eg
  					     file from which it was read. */
  WlzObject		*obj;		/*!< The Woolz object. */
} WlzObjCacheEntry;

/*!
* \brief        Cache for Woolz objects within a Woolz IIP server.
* \ingroup      WlzIIPServer
*/
class WlzObjectCache
{
  private:
    int			enabled;		/*!< Used to enable and disable
    						     the cache. */
    AlcLRUCache		*objCache;		/*!< Woolz object cache. */
    inline size_t 	MBytesToBytes(size_t m)
    			{
			  const int	c = 1024 * 1024;

			  return(c * m);
			};
    inline size_t 	BytesToMBytes(size_t b)
    			{
			  const int	c = 1024 * 1024;

			  return((b + c - 1) / c);
			};
    size_t		ComputeObjectSize(WlzObject *obj);
    static unsigned int WlzObjCacheKeyFn(AlcLRUCache *cache, const void *e);
    static int		WlzObjCacheCmpFn(const void *e0, const void *e1);
    static void		WlzObjCacheUnlinkFn(AlcLRUCache *cache, const void *e);

  public:
    WlzObjectCache();
    ~WlzObjectCache();
    void 		insert(WlzThreeDViewStruct *vs, const std::string  str)
                	throw(std::string);
    void 		insert(WlzObject *obj, const std::string  str)
                	throw (std::string);
    WlzObject 		*get(std::string str);
    WlzThreeDViewStruct *getVS(std::string str);
    unsigned int 	getNumElements();
    float 		getMemorySize();
    void 		setMaxSize(size_t max);

};


#endif
