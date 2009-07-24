#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _PNGComressor_h[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         PNGComressor.h
* \author       Zsolt Husz
* \date         May 2009
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
* \brief	PNG class wrapper to png library
* \ingroup	WlzIIPServer
*/


#ifndef _PNGCOMPRESSOR_H
#define _PNGCOMPRESSOR_H

#include <cstdio>
#include <string>
#include "RawTile.h"

extern "C"{
  /* Undefine this to prevent compiler warning
   */
#undef HAVE_STDLIB_H
#include <png.h>
}

/*!
 * \brief	Expanded data destination object for buffered output used by PNG library
 * \ingroup	WlzIIPServer
 */

typedef struct {
  size_t size;                       /**< size of data */
  size_t mx;                         /**< allocated size of data buffer*/
  unsigned char* data;               /**< data buffer */
  png_structp png_ptr;               /**< png data pointer */
  png_infop info_ptr;                /**< png info pointer */
} png_destination_mgr;

typedef png_destination_mgr * png_destination_ptr;

/*!
 * \brief	Wrapper class to PNG library
 * \ingroup	WlzIIPServer
 */
class PNGCompressor{

 private:

  /// the width, height and number of channels per sample for the image
  unsigned int width;        /**< the width of the image */
  unsigned int height;       /**< the height of the image */
  png_uint_32 channels;      /**< the channels per sample for the image */

  png_destination_mgr dest;  /**< destination data structure */

public:
  /*!
  * \ingroup      WlzIIPServer
  * \brief        Constructor
  * \par      Source:
  *                PNGCompressor.cc
  */
  PNGCompressor( ) { dest.data=NULL; dest.mx=0; dest.size=0; };

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Initialise strip based compression
  *
  * If we are doing a strip based encoding, we need to first initialise
  *    with InitCompression, then compress a single strip at a time using
  *    CompressStrip and finally clean up using Finish
  * \param rawtile tile containing the image to be compressed
  * \param strip_height pixel height of the strip we want to compress
  * \return       header size
  * \par      Source:
  *                PNGCompressor.cc
  */
  int InitCompression( RawTile& rawtile, unsigned int strip_height ) throw (std::string);

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Compress a strip of image data
  * \param s source image data
  * \param tile_height pixel height of the tile we are compressing
  * \return       compressed strip size
  * \par      Source:
  *                PNGCompressor.cc
  */
  unsigned int CompressStrip( unsigned char* s, unsigned int tile_height ) throw (std::string);

 /*!
  * \ingroup      WlzIIPServer
  * \brief        Finish the strip based compression and free memory
  * \return       Tail size
  * \par      Source:
  *                PNGCompressor.cc
  */
  unsigned int Finish() throw (std::string);


 /*!
  * \ingroup      WlzIIPServer
  * \brief        Compress an entire buffer of image data at once in one command
  * \param t      tile of image data
  * \return       Compressed data size
  * \par      Source:
  *                PNGCompressor.cc
  */
  int Compress( RawTile& t ) throw (std::string);
};

#endif
