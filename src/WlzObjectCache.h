#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _WlzObjectCache_h[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         WlzObjectCache.cc
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
* \brief	The caching mechanism for Woolz Objects.
* \ingroup	WlzIIPServer
* \todo         Prioritise removal with an access count. Curently FIFO is in place.
* \bug          None known.
*/


#ifndef _WLZOBJECTCACHE_H
#define _WLZOBJECTCACHE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Cache.h"

#ifndef USE_HASHMAP
#include <map>
#endif

#include <iostream>
#include <list>
#include <string>
#include "RawTile.h"

#include "WlzObjectCache.h"
#include "Environment.h"

/*!
* \brief	Container for Woolz objects. Manages by the allocation and
*               deallocation of the Woolz object.
* \ingroup	WlzIIPServer
* \todo         -
* \bug          None known.
*/
class WlzObjectContainer {
 protected:
  WlzObject *obj;      /*!< Pointer to the stored object */
  off_t  size;         /*!< Size of the object */
 public:

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Constructor
  * \param    o   Woolz object stored
  * \param    fn  Filename for computing size of the object
  * \return       void
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObjectContainer(WlzObject *o, string fn) {
    struct stat buffer;

    int         status;
    status = stat(fn.c_str(), &buffer);

    if (status || buffer.st_size < 0) {
      throw string( "Size of " + fn + " can not be found.");
    }
    size = buffer.st_size;
    obj       =  WlzAssignObject( o, NULL );
  };

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Copy constructor
  * \param    c   object to be copied
  * \return       void
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObjectContainer(const WlzObjectContainer & c) {
    obj        =  WlzAssignObject( c.obj, NULL);
    size       = c.size;
  };
  WlzObjectContainer(): obj(NULL), size(0) {};

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Destructor
  * \return       void
  * \par      Source:
  *                WlzObjectCache.cc
  */
  ~WlzObjectContainer(){
    if (obj)
      WlzFreeObj( obj );
  };

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Assignment operator
  * \param    c   object to be copied
  * \return       void
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObjectContainer& operator=(const WlzObjectContainer& c){
    if (obj)
    {
      WlzFreeObj( obj );
    }
    obj       = WlzAssignObject( c.obj, NULL);
    size      = c.size;
    return *this;
  }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Returns the size of the object
  * \return       object size in bytes
  * \par      Source:
  *                WlzObjectCache.cc
  */
  off_t getSize() { return size; }

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Return the object
  * \return       pointer to the cached Woolz object
  * \par      Source:
  *                WlzObjectCache.cc
  */
  operator WlzObject*() {return obj;};
};


/*!
* \brief        Cache to Woolz objects
* \ingroup      WlzIIPServer
* \todo         -
* \bug          None known.
*/
class WlzObjectCache {

 private:

  unsigned long maxSize;                  /*!< Max size in bytes of the cache objects. The object 
                                               size is aproximated the total space used mihgt 
                                               be higher then this value */

  unsigned long currentSize;              /*!< Current memory running total */

  WlzViewStructCache *cacheViewStruct;    /*!< View struct cache */

#ifdef POOL_ALLOCATOR
  typedef std::list < std::pair<const std::string, WlzObjectContainer>,
    __gnu_cxx::__pool_alloc< std::pair<const std::string, WlzObjectContainer> > > WlzObjectList;
#else
  typedef std::list < std::pair<const std::string, WlzObjectContainer> > WlzObjectList;
#endif
                                                                     /*!< Main cache storage typedef */

  typedef std::list < std::pair<const std::string, WlzObjectContainer> >::iterator WlzObjectList_Iter;
                                                                     /*!< Main cache list iterator typedef */
#ifdef USE_HASHMAP
#ifdef POOL_ALLOCATOR
  typedef __gnu_cxx::hash_map < const std::string, WlzObjectList_Iter,
    __gnu_cxx::hash< const std::string >,
    std::equal_to< const std::string >,
    __gnu_cxx::__pool_alloc< std::pair<const std::string, WlzObjectList_Iter> >
    > WlzObjectMap;
#else
  typedef __gnu_cxx::hash_map < const std::string, WlzObjectList_Iter > WlzObjectMap;
#endif
#else
  typedef std::map < const std::string, WlzObjectList_Iter > WlzObjectMap;
#endif
                                                                    /*!< Index typedef */
  WlzObjectList objList;                                            /*!< Main cache storage object */

  WlzObjectMap objMap;                                              /*!< Main Cache storage index object */

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Internal touch function
  *               Touches a key in the Cache and makes it the most recently used
  *  \param key to be touched
  *  \return a Map_Iter pointing to the key that was touched.
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObjectMap::iterator _touch( const std::string &key ) {
    WlzObjectMap::iterator miter = objMap.find( key );
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
  *                WlzObjectCache.cc
  */
  void _remove( const WlzObjectMap::iterator &miter ) {
    // Reduce our current size counter
    currentSize -= miter->second->second.getSize();

    // Remove from view structure cache all view structures associated to this object.
    // This is required, since objects can be freed only the view structure copies of the object are also freed.
    cacheViewStruct->remove(miter->second->second);

    objList.erase( miter->second );
    objMap.erase( miter );
  }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Interal remove function
  * \param key to remove 
  * \return  void
  * \par      Source:
  *                WlzObjectCache.cc
  */
  void _remove( const std::string &key ) {
    WlzObjectMap::iterator miter = objMap.find( key );
    this->_remove( miter );
  }



 public:

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Constructor
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObjectCache( WlzViewStructCache *cacheVS) {

    // get maximum cache size in MB and convert it to bytes
    maxSize = (unsigned long)Environment::getMaxWlzObjCacheSize() *1024000 ; currentSize = 0;
    cacheViewStruct = cacheVS;
  };


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Destructor
  * \par      Source:
  *                WlzObjectCache.cc
  */
  ~WlzObjectCache() {
    objList.clear();
    objMap.clear();
  }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Insert a tile
  * \param        obj     WlzObj to be be inserted
  * \param        key     the key of obj
  * \warning      Objects to big to fit in the cache are not cached
  * \par      Source:
  *                WlzObjectCache.cc
  */
  void insert(WlzObject* obj, const std::string  filename ) throw (std::string) {
      if( maxSize == 0 ) return;

      // Touch the key, if it exists
      WlzObjectMap::iterator miter = this->_touch( filename );

      // If this index already exists, do nothing
      if( miter != objMap.end() ) return;

      // Store the key if it doesn't already exist in our cache
      // Ok, do the actual insert at the head of the list
      WlzObjectContainer r = WlzObjectContainer( obj, filename );

      if ( r.getSize() > maxSize ) {
          return; // WlzObjectCache :: Object too big to be cached. Just return
      }
      objList.push_front( std::make_pair(filename, r) );

      // And store this in our map
      WlzObjectList_Iter liter = objList.begin();
      objMap[ filename ] = liter;

      // Update our total current size variable
      currentSize += r.getSize();

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
  * \brief        Return the number of objects cached
  * \return       The number of objects cached
  * \par      Source:
  *                WlzObjectCache.cc
  */
  unsigned int getNumElements() { return objList.size(); }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Return the number of MB stored
  * \return       The number of objects cached
  * \par      Source:
  *                WlzObjectCache.cc
  */
  float getMemorySize() { return (float) ( currentSize / 1024000.0 ); }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Get a WlzObj from the cache 
  * \param        key     the key of the required object 
  * \return       Pointer to the requested WlzObj or NULL if not found in the cache
  * \par      Source:
  *                WlzObjectCache.cc
  */
  WlzObject* get( std::string key) {

    if( maxSize == 0 ) return NULL;

    WlzObjectMap::iterator miter = objMap.find( key );
    if( miter == objMap.end() ) return NULL;
    this->_touch( key );

    return miter->second->second;
  }


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Sets the maximum cache size. If size is less the currently stored objects, the objects are freed.
  * \param        max Maximum number of WlzObj stored
  * \par      Source:
  *                WlzObjectCache.cc
  */
  void setMaxSize( unsigned long max ) {
    WlzObjectList_Iter liter;
    while( currentSize > max ) {
      // Remove the last element.
      liter = objList.end();
      --liter;
      this->_remove( liter->first );
    }
    maxSize = max;
  };

};



#endif
