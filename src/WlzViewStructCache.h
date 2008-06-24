#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _WlzVolume_c[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         WlzViewStructCache.cc
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
* \brief	The caching mechanism for Woolz 3D view structures.
* \ingroup	WlzIIPServer
* \todo         Prioritise removal with an access count. Curently FIFO is in place.
* \bug          None known.
*/


#ifndef _WLZVIEWSTRUCTCACHE_H
#define _WLZVIEWSTRUCTCACHE_H

#include "WlzViewStructCache.h"
#include "Environment.h"

#ifndef USE_HASHMAP
#include <map>
#endif

#include <iostream>
#include <list>
#include <string>
#include "RawTile.h"

/*! 
* \brief	Handles view structure, WlzThreeDViewStruct, caching.
* \ingroup	WlzIIPServer
* \todo         -
* \bug          None known.
*/
class WlzViewStructCache {

 private:

  unsigned long maxSize;        /*!< Max number of cached structures */

  unsigned long currentSize;    /*!< Current number of cached structures */

#ifdef POOL_ALLOCATOR
  typedef std::list < std::pair<const std::string,WlzThreeDViewStruct*>,
    __gnu_cxx::__pool_alloc< std::pair<const std::string,WlzThreeDViewStruct*> > > WlzThreeDViewStructList;
#else
  typedef std::list < std::pair<const std::string,WlzThreeDViewStruct*> > WlzThreeDViewStructList;
#endif
                                                                    /*!< Main cache storage typedef */

  typedef std::list < std::pair<const std::string,WlzThreeDViewStruct*> >::iterator WlzThreeDViewStructList_Iter; 
                                                                    /*!< Main cache list iterator typedef */

#ifdef USE_HASHMAP
#ifdef POOL_ALLOCATOR
  typedef __gnu_cxx::hash_map < const std::string, WlzThreeDViewStructList_Iter,
    __gnu_cxx::hash< const std::string >,
    std::equal_to< const std::string >,
    __gnu_cxx::__pool_alloc< std::pair<const std::string, WlzThreeDViewStructList_Iter> >
    > WlzThreeDViewStructMap;
#else
  typedef __gnu_cxx::hash_map < const std::string,WlzThreeDViewStructList_Iter > WlzThreeDViewStructMap;
#endif
#else
  typedef std::map < const std::string,WlzThreeDViewStructList_Iter > WlzThreeDViewStructMap;
#endif
                                                                 /*!< Index typedef */

  WlzThreeDViewStructList objList; /*!< Main cache storage object */

  WlzThreeDViewStructMap objMap;  /*!< Main Cache storage index object */

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Internal touch function
  *               Touches a key in the Cache and makes it the most recently used
  *  \param key to be touched
  *  \return a Map_Iter pointing to the key that was touched.
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  WlzThreeDViewStructMap::iterator _touch( const std::string &key ) {
    WlzThreeDViewStructMap::iterator miter = objMap.find( key );
    if( miter == objMap.end() ) return miter;
    // Move the found node to the head of the list.
    objList.splice( objList.begin(), objList, miter->second );
    return miter;
  }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Interal remove function
  * \param miter Map_Iter that points to the key to remove
  * \warning miter is now longer usable after being passed to this function.
  * \return  void
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  void _remove( const WlzThreeDViewStructMap::iterator &miter ) {

    WlzFree3DViewStruct( miter->second->second);
    currentSize -= 1;
    objList.erase( miter->second );
    objMap.erase( miter );
  }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Interal remove function
  * \param key to remove 
  * \return  void
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  void _remove( const std::string &key ) {
    WlzThreeDViewStructMap::iterator miter = objMap.find( key );
    this->_remove( miter );
  }



 public:

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Constructor
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  WlzViewStructCache( ) {
    maxSize = Environment::getMaxViewStructCacheSize();
    currentSize = 0;
  };

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Destructor
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  ~WlzViewStructCache() {
    objList.clear();
    objMap.clear();
  }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Sets the maximum cache size
  * \param        max Maximum number of WlzThreeDViewStruct stored
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  void setMaxSize( unsigned long max ) {
    WlzThreeDViewStructList_Iter liter;
    while( currentSize > max ) {
      // Remove the last element.
      liter = objList.end();
      --liter;
      this->_remove( liter->first );
    }
    maxSize = max;
  };

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Insert a tile
  * \param        viewStr WlzThreeDViewStruct to be inserted 
  * \param        key     key of viewStr 
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  void insert(WlzThreeDViewStruct* viewStr, const std::string  key ) {

    //Make a copy of the view structure
    WlzThreeDViewStruct* r = WlzAssign3DViewStruct( viewStr, NULL );

    if( maxSize == 0 ) return;

    // Touch the key, if it exists
    WlzThreeDViewStructMap::iterator miter = this->_touch( key );

    // If this index already exists, do nothing
    if( miter != objMap.end() ) return;

    // Store the key if it doesn't already exist in our cache
    // Ok, do the actual insert at the head of the list
    objList.push_front( std::make_pair(key,r) );

    // And store this in our map
    WlzThreeDViewStructList_Iter liter = objList.begin();
    objMap[ key ] = liter;

    // Update our total current size variable
    currentSize ++;

    // Check to see if we need to remove an element due to exceeding max_size
    while( currentSize > maxSize ) {
      // Remove the last element.
      liter = objList.end();
      --liter;
      this->_remove( liter->first );
    }

  }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Remove all view structures of an object
  * \param        obj Woolz object of which view structures are removed 
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  void remove( WlzObject *obj) {
      WlzThreeDViewStructList_Iter liter, liter_remove ; 
      for (liter = objList.begin(); liter !=objList.end(); )
      {
        if (liter->second->ref_obj==obj)
        {
          liter_remove=liter;
          liter++;
          this->_remove( liter_remove->first );
        } else 
        {
          liter++;
        }
      }
    }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Return the number of structures cached
  * \return       The number of structures cached
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  unsigned int getNumElements() { return objList.size(); }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Get a WlzThreeDViewStruct from the cache 
  * \param        key     the key of the required structure
  * \return       Pointer to the requested WlzThreeDViewStruct or NULL if not found in the cache
  * \par      Source:
  *                WlzViewStructCache.cc
  */
  WlzThreeDViewStruct* get( std::string key) {

    if( maxSize == 0 ) return NULL;

    WlzThreeDViewStructMap::iterator miter = objMap.find( key );
    if( miter == objMap.end() ) return NULL;
    this->_touch( key );

    return miter->second->second;
  }


  /*void WlzViewStructCache::clearall() {
    WlzThreeDViewStructList_Iter liter ;
    while( currentSize > 0 ) {
      *(session.logfile) << "WLZViewStructureCache :: cleanup"<< endl;
      // Remove all
      liter = objList.end();
      --liter;
      this->_remove( liter->first );
    }
   }*/

};
#endif
