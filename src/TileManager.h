#ifndef _TILEMANAGER_H
#define _TILEMANAGER_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _TileManager_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         TileManager.h
* \author       Patrick Audley, Ruven Pillay, Zsolt Husz, Bill Hill
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
* Copyright (C) 2004 by Patrick Audley
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
* \brief	IIP Server: Tile Cache Handler.
* 		Based on an LRU cache by Patrick Audley
* 		<http://blackcat.ca/lifeline/query.php/tag=LRU_CACHE>.
* \ingroup	WlzIIPServer
*/

#include <fstream>

#include "RawTile.h"
#include "IIPImage.h"
#include "JPEGCompressor.h"
#include "PNGCompressor.h"
#include "Cache.h"
#include "Timer.h"



/// Class to manage access to the tile cache and tile cropping

class TileManager{


 private:

  Cache* tileCache;
  JPEGCompressor* jpeg;
  PNGCompressor* png;
  IIPImage* image;
  Timer compression_timer, tile_timer, insert_timer;

  /// Get a new tile from the image file
  /**
   *  If the JPEG tile already exists in the cache, use that, otherwise check for
   *  an uncompressed tile. If that does not exist either, extract a tile from the
   *  image. If this is an edge tile, crop it.
   *  @param resolution resolution number
   *  @param tile tile number
   *  @param xangle horizontal sequence number
   *  @param yangle vertical sequence number
   *  @param c CompressionType
   *  @return RawTile
   */
  RawTile getNewTile( int resolution, int tile, int xangle, int yangle, CompressionType c );


  /// Crop a tile to remove padding
  /** @param t pointer to tile to crop
   */
  void crop( RawTile* t );


 public:


  /// Constructor
  /**
   * @param tc pointer to tile cache object
   * @param im pointer to IIPImage object
   * @param j  pointer to JPEGCompressor object
   * @param p  pointer to PNGCompressor object
   */
  TileManager( Cache* tc, IIPImage* im, JPEGCompressor* j, PNGCompressor* p){
    tileCache = tc; 
    image = im;
    jpeg = j;
    png = p;
  };



  /// Get a tile from the cache
  /**
   *  If the JPEG tile already exists in the cache, use that, otherwise check for
   *  an uncompressed tile. If that does not exist either, extract a tile from the
   *  image. If this is an edge tile, crop it.
   *  @param resolution resolution number
   *  @param tile tile number
   *  @param xangle horizontal sequence number
   *  @param yangle vertical sequence number
   *  @param c CompressionType
   *  @return RawTile
   */
  RawTile getTile( int resolution, int tile, int xangle, int yangle, CompressionType c );


};


#endif
