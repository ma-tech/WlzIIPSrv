#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _TileManager_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         TileManager.cc
* \author       Ruven Pillay, Zsolt Husz, Bill Hill
* \date         June 2008
* \version      $Id$
* \par
* Copyright (C) 2005-2006 Ruven Pillay.
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
* \brief	IIP Server: Tile Cache Handler.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "TileManager.h"

using namespace std;

RawTile TileManager::getNewTile(int resolution, int tile,
                                int xangle, int yangle, CompressionType c){
  LOG_INFO("TileManager :: Cache Miss for resolution: " << resolution <<
            ", tile: " << tile);
  LOG_INFO("TileManager :: Cache Size: " <<
	    tileCache->getNumElements() << " tiles, " <<
	    tileCache->getMemorySize() << "MB");

  RawTile ttt;
  int len = 0;

  // Get our raw tile
  ttt = image->getTile( xangle, yangle, resolution, tile);

  if( c == UNCOMPRESSED ){
    // Add to our tile cache
    LOG_COND_INFO(insert_timer.start());
    tileCache->insert( ttt );
    LOG_INFO("TileManager :: Tile cache insertion time: " <<
	      insert_timer.getTime() << "us");
    return ttt;
  }


  /* We need to crop our edge tiles if they are not the full tile size
   */
  if(((ttt.width != image->getTileWidth()) ||
     (ttt.height != image->getTileHeight())) && (c == JPEG || c == PNG)){
    this->crop( &ttt );
  }


  switch( c ){

  case JPEG:

    // Do our JPEG compression iff we have an 8 bit per channel image
    if( ttt.bpc == 8 ){
      LOG_COND_INFO(compression_timer.start());
      len = jpeg->Compress(ttt);
      LOG_INFO("TileManager :: JPEG Compression Time: " <<
	        compression_timer.getTime() << "us");
      ttt.compressionType = JPEG;
    }
    break;

  case PNG:

    // Do our PNG compression iff we have an 8 bit per channel image
    if( ttt.bpc == 8 ){
      LOG_COND_INFO(compression_timer.start());
      len = png->Compress( ttt );
      LOG_INFO("TileManager :: PNG Compression Time: " <<
	        compression_timer.getTime() << "us");
      ttt.compressionType = PNG;
    }
    break;


  case DEFLATE:

    // No deflate for the time being ;-)
    LOG_WARN("TileManager :: DEFLATE Compression requested: Not implimented");

    break;
  default:
    break;
  }
  // Add to our tile cache
  LOG_COND_INFO(insert_timer.start());
  tileCache->insert( ttt );
  LOG_INFO("TileManager :: Tile cache insertion time: " <<
            insert_timer.getTime() << "us");
  return ttt;
}



void TileManager::crop( RawTile *ttt ){

  //Wlz the data has already the correct length, crop is not needed
  if (ttt->dataLength == ttt->height * ttt->width * ttt->channels * ttt->bpc/8)
      return;

  int tw = image->getTileWidth();
  int th = image->getTileHeight();

  LOG_INFO("TileManager :: Edge tile: Base size: " << tw << "x" << th <<
            ": This tile: " << ttt->width << "x" << ttt->height);

  // Create a new buffer, fill it with the old data, then copy
  // back the cropped part into the RawTile buffer
  int len = tw * th * ttt->channels * ttt->bpc/8;
  unsigned char* buffer = (unsigned char*) malloc( len );
  unsigned char* src_ptr = (unsigned char*) memcpy( buffer, ttt->data, len );
  unsigned char* dst_ptr = (unsigned char*) ttt->data;

  // Copy one scanline at a time
  for( unsigned int i=0; i<ttt->height; i++ ){
    len =  ttt->width * ttt->channels * ttt->bpc/8;
    memcpy( dst_ptr, src_ptr, len );
    dst_ptr += len;
    src_ptr += tw * ttt->channels * ttt->bpc/8;
  }

  free( buffer );

  // Reset the data length
  len = ttt->width * ttt->height * ttt->channels * ttt->bpc/8;
  ttt->dataLength = len;

}




RawTile TileManager::getTile( int resolution, int tile, int xangle, int yangle, CompressionType c ){

  RawTile* rawtile = NULL;
  string tileCompression;
  string compName;


  // Time the tile retrieval
  LOG_COND_INFO(tile_timer.start());

  /* Try to get this tile from our cache first as a JPEG, then uncompressed
     Otherwise decode one from the source image and add it to the cache
   */
  switch( c )
    {

    case JPEG:
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					  xangle, yangle, JPEG, jpeg->getQuality() )) ) break;
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, DEFLATE, 0 )) ) break;
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, UNCOMPRESSED, 0 )) ) break;
      break;

    case PNG:
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					  xangle, yangle, PNG, 100 )) ) break;
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, DEFLATE, 0 )) ) break;
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, UNCOMPRESSED, 0 )) ) break;
      break;

    case DEFLATE:

      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, DEFLATE, 0 )) ) break;
      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, UNCOMPRESSED, 0 )) ) break;
      break;


    case UNCOMPRESSED:

      if( (rawtile = tileCache->getTile( image->getHash(), resolution, tile,
					 xangle, yangle, UNCOMPRESSED, 0 )) ) break;
      break;


    default: 
      break;

    }


  // If we haven't been able to get a tile, get a raw one
  if( !rawtile ){
    RawTile newtile = this->getNewTile( resolution, tile, xangle, yangle, c );
    LOG_INFO("TileManager :: Total Tile Access Time: " <<
	      tile_timer.getTime() << "us");
    return newtile;
  }


  // Define our compression names
  switch( rawtile->compressionType ){
    case JPEG: compName = "JPEG"; break;
    case PNG: compName = "PNG"; break;
    case DEFLATE: compName = "DEFLATE"; break;
    case UNCOMPRESSED: compName = "UNCOMPRESSED"; break;
    default: break;
  }

  LOG_INFO("TileManager :: Cache Hit for resolution: " << resolution <<
            ", tile: " << tile << ", compression: " << compName);
  LOG_INFO("TileManager :: Cache Size: " <<
	    tileCache->getNumElements() << " tiles, " <<
	    tileCache->getMemorySize() << "MB");


  // Check whether the compression used for out tile matches our requested compression type.
  // If not, we must convert

  if( c == JPEG && rawtile->compressionType == UNCOMPRESSED ){

    // Rawtile is a pointer to the cache data, so we need to create a copy of it in case we compress it
    RawTile ttt( *rawtile );

    // Do our JPEG compression iff we have an 8 bit per channel image
    if( rawtile->bpc == 8 ){

      // Crop if this is an edge tile
      if( (ttt.width != image->getTileWidth()) || (ttt.height != image->getTileHeight()) ){
	this->crop( &ttt );
      }
      LOG_COND_INFO(compression_timer.start());
      unsigned int oldlen = rawtile->dataLength;
      unsigned int newlen = jpeg->Compress( ttt );
      LOG_INFO(
    "TileManager :: JPEG requested, but UNCOMPRESSED compression in cache.");
      LOG_INFO("TileManager :: JPEG Compression Time: " <<
                compression_timer.getTime() << "us");
      LOG_INFO("TileManager :: Compression Ratio: " <<
                newlen << "/" << oldlen << " = " <<
		((float)newlen/(float)oldlen));

      // Add our compressed tile to the cache
      LOG_COND_INFO(insert_timer.start());
      tileCache->insert( ttt );
      LOG_INFO("TileManager :: Tile cache insertion time: " <<
	        insert_timer.getTime() << "us");
      LOG_INFO("TileManager :: Total Tile Access Time: " <<
	        tile_timer.getTime() << "us");
      return RawTile( ttt );
    }
  }

  if( c == PNG && rawtile->compressionType == UNCOMPRESSED ){

    // Rawtile is a pointer to the cache data, so we need to create a copy of it in case we compress it
    RawTile ttt( *rawtile );

    // Do our PNG compression iff we have an 8 bit per channel image
    if( rawtile->bpc == 8 ){

      // Crop if this is an edge tile
      if( (ttt.width != image->getTileWidth()) || (ttt.height != image->getTileHeight()) ){
	this->crop( &ttt );
      }
      LOG_COND_INFO(compression_timer.start());
      unsigned int oldlen = rawtile->dataLength;
      unsigned int newlen = png->Compress( ttt );
      LOG_INFO(
      "TileManager :: PNG requested, but UNCOMPRESSED compression in cache.");
      LOG_INFO("TileManager :: PNG Compression Time: " <<
                compression_timer.getTime() << "us");
      LOG_INFO("TileManager :: Compression Ratio: " <<
                newlen << "/" << oldlen << " = " <<
		((float )newlen/(float )oldlen));

      // Add our compressed tile to the cache
      LOG_COND_INFO(insert_timer.start());
      tileCache->insert( ttt );
      LOG_INFO("TileManager :: Tile cache insertion time: " <<
	        insert_timer.getTime() << "us");
      LOG_INFO("TileManager :: Total Tile Access Time: " <<
	        tile_timer.getTime() << "us");
      return RawTile( ttt );
    }
  }
  LOG_INFO("TileManager :: Total Tile Access Time: " <<
            tile_timer.getTime() << " microseconds");
  return RawTile( *rawtile );
}
