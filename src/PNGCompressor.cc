#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _PNGCompressor_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         PNGCompressor.cc
* \author       Zsolt Husz, Bill Hill
* \date         May 2009
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
* \brief	PNG class wrapper to png library.
* \ingroup	WlzIIPServer
*/

#include "PNGCompressor.h"

using namespace std;

static void
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
  png_uint_32 check;
  png_destination_ptr dest=(png_destination_ptr)png_get_io_ptr(png_ptr);

  if (dest-> size+length > dest->mx) {
    dest->mx+= (10240>length ? 10240 : length);
    dest->data=(unsigned char*)realloc(dest->data, dest->mx); 
  }
  if (!memcpy(dest->data+dest->size, data, length))
     throw string( "png_write_data: error writing to buffer.");
  dest->size+=length;
}

static void
png_flush(png_structp png_ptr)
{
}

static void
png_cexcept_error(png_structp png_ptr, png_const_charp msg)
{
#ifndef PNG_NO_CONSOLE_IO
#endif
   {
     throw string( msg);
   }
}

extern "C" {
}

int PNGCompressor::InitCompression( RawTile& rawtile, unsigned int strip_height ) throw (string)
{
  // Do some initialisation

  // Set up the correct width and height for this particular tile
  width = rawtile.width;
  height = rawtile.height;

  channels = rawtile.channels;

  dest.mx = rawtile.dataLength ;
  dest.data = (unsigned char*)rawtile.data;
  dest.size = 0;

  // Make sure we only try to compress images with 1 or 3 channels with ir without alpha
  if( ! ( (channels==1) || (channels==2) || (channels==3) || (channels==4))  ){
    throw string( "PNGCompressor: currently only either 1 or 3 channels are supported with or without alpha values." );
  }

  const int           ciBitDepth = 8;

  dest.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
      (png_error_ptr)png_cexcept_error, (png_error_ptr)NULL);

  if (!dest.png_ptr)
  {
    throw string( "PNGCompressor: Error allocacating png_structp." );
  }

  dest.info_ptr = png_create_info_struct(dest.png_ptr);
  if (! dest.info_ptr) {
      png_destroy_write_struct(&dest.png_ptr, (png_infopp) NULL);
      throw string( "PNGCompressor: Error creating png_infop." );
  }

  png_set_write_fn(dest.png_ptr, (png_voidp)&dest, png_write_data, png_flush);

  // we're going to write a very simple 3x8 bit RGB image
  png_set_IHDR(dest.png_ptr,  dest.info_ptr, width, height, ciBitDepth,
            (channels<3) ? ((channels==2) ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY): ((channels==4) ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB), PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);

  // write the file header information
  png_write_info(dest.png_ptr,  dest.info_ptr);

  return dest.size;
}


/*
  We use a separate tile_height from the predefined strip_height because
  the tile height for the final row can be different
 */
unsigned int PNGCompressor::CompressStrip( unsigned char* buf, unsigned int tile_height ) throw (string)
{
  png_uint_32     ulRowBytes = width * channels;
  png_byte        **ppbRowPointers = NULL;

  dest.size = 0;  //rewind output buffer

   if ((ppbRowPointers = (png_bytepp) malloc(tile_height * sizeof(png_bytep))) == NULL)
      throw "PNGCompressor: Out of memory";

  for (int i = 0; i < tile_height; i++)
    ppbRowPointers[i] = (png_byte*)(buf + i *ulRowBytes);

  // write out the entire image data in one call
  png_write_rows (dest.png_ptr, ppbRowPointers, tile_height);

  free (ppbRowPointers);
  ppbRowPointers = NULL;

  return dest.size;
}



unsigned int PNGCompressor::Finish() throw (string) {
  dest.size = 0;

 // write the additional chunks to the PNG file 
  png_write_end(dest.png_ptr,  dest.info_ptr);

  // clean up after the write, and free any memory allocated
  png_destroy_write_struct(&(dest.png_ptr), (png_infopp) NULL);

  return dest.size ;
}

int PNGCompressor::Compress( RawTile& rawtile ) throw (string) {
  png_byte   **ppbRowPointers = NULL;
  const int           ciBitDepth = 8;
  png_uint_32         ulRowBytes;

  // Set up the correct width and height for this particular tile
  width = rawtile.width;
  height = rawtile.height;
  channels = rawtile.channels;

  // Make sure we only try to compress images with 1 or 3 channels
  if( ! ( (channels==1) || (channels==2) || (channels==3) || (channels==4))  ){
    throw string( "PNGCompressor: currently only either 1 or 3 channels are supported with or without alpha values." );
  }

  dest.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
      (png_error_ptr)png_cexcept_error, (png_error_ptr)NULL);

  if (!dest.png_ptr) {
    throw string( "PNGCompressor: Error allocacating png_structp." );
  }

  dest.info_ptr = png_create_info_struct(dest.png_ptr);
  if (! dest.info_ptr) {
      png_destroy_write_struct(&(dest.png_ptr), (png_infopp) NULL);
      throw string( "PNGCompressor: Error creating png_infop." );
  }

  dest.size = 0;
  dest.mx =  rawtile.dataLength;  //initial PNG size
  dest.data = (unsigned char*)malloc(dest.mx);

  png_set_write_fn(dest.png_ptr, (png_voidp)&dest, png_write_data, png_flush);

  // we're going to write a very simple 3x8 bit RGB image
  png_set_IHDR(dest.png_ptr,  dest.info_ptr, width, height, ciBitDepth,
            (channels<3) ? ((channels==2) ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY): ((channels==4) ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB), PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);

  // write the file header information
  png_write_info(dest.png_ptr,  dest.info_ptr);

  // row_bytes is the width x number of channels
  ulRowBytes = width * channels;

  try {
    // we can allocate memory for an array of row-pointers
    if ((ppbRowPointers = (png_bytepp) malloc(height * sizeof(png_bytep))) == NULL)
        throw "PNGCompressor: Out of memory";

    // set the individual row-pointers to point at the correct offsets
    for (int i = 0; i < height; i++)
       ppbRowPointers[i] = (png_byte*)((unsigned char*)rawtile.data + i *ulRowBytes);

    // write out the entire image data in one call
    png_write_rows(dest.png_ptr, ppbRowPointers,height);

    // write the additional chunks to the PNG file (not really needed)
    png_write_end(dest.png_ptr,  dest.info_ptr);

    // and we're done
    free (ppbRowPointers);
    ppbRowPointers = NULL;

    // clean up after the write, and free any memory allocated
    png_destroy_write_struct(&(dest.png_ptr), (png_infopp) NULL);
  } catch (char * msg){
    png_destroy_write_struct(&(dest.png_ptr), (png_infopp) NULL);

    if(ppbRowPointers)
        free (ppbRowPointers);
    return 0;
  }

  //if dest is bigger, then realloate
  if (dest.size > rawtile.dataLength ) {
      free(rawtile.data);
      rawtile.data = (unsigned char*)malloc(dest.size);
  }
  rawtile.dataLength = dest.size;
  memcpy(rawtile.data, dest.data, rawtile.dataLength);
  free(dest.data);
  dest.data = NULL;
  dest.size = 0;
  dest.mx   = 0;

  // Set the tile compression type
  rawtile.compressionType = PNG;
  rawtile.quality = 100;  // PNG is not compressed

  // Return the size of the data we have compressed
  return rawtile.dataLength;
}
