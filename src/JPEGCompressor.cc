#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _JPEGCompressor_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         JPEGCompressor.cc
* \author       Ruven Pillay, Zsolt Husz
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
* Copyright (C) 2000-2005 Ruven Pillay.
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
* \brief	JPEG class wrapper to ijg jpeg library.
* \ingroup    	WlzIIPServer
*/

#include "JPEGCompressor.h"

//for debug only
//#include "Task.h"
//extern      Session session;
using namespace std;


/* My version of the JPEG error_exit function. We want to pass control back
   to the program, so simply throw an exception
*/

METHODDEF(void) iip_error_exit( j_common_ptr cinfo )
{
  char buffer[ JMSG_LENGTH_MAX ];

  /* Create the message
   */
  (*cinfo->err->format_message) ( cinfo, buffer );

  /* Let the memory manager delete any temp files before we die
   */
  jpeg_destroy( cinfo );

  /* throw an exception rather than print out a message and exit
   */
  throw string( buffer );
}



extern "C" {

  void setup_error_functions( jpeg_compress_struct *a ){
    a->err->error_exit = iip_error_exit; 
  }
}




/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

METHODDEF(void)
iip_init_destination (j_compress_ptr cinfo)
{
  size_t mx;
  iip_dest_ptr dest = (iip_dest_ptr) cinfo->dest;

  /* If we have set the strip height, we must be doing a buffer to buffer
     compression, so only allocate enough for this strip. Otherwise allocate
     memory for the whole image
  */
  if( dest->strip_height > 0 ){
    mx = cinfo->image_width * dest->strip_height * cinfo->input_components;
  }
  else{
    mx = cinfo->image_width * cinfo->image_height * cinfo->input_components;
  }

  /* Add a kB because when we have very small tiles, the JPEG data
     including header can end up being larger than the original raw
     data size!
  */
  mx += 1024;

  /* Allocate the output buffer --- it will be released when done with image
   */
  dest->buffer = (JOCTET *)
    (*cinfo->mem->alloc_small) ( (j_common_ptr) cinfo, JPOOL_IMAGE,
				 mx * sizeof(JOCTET) );
  
  dest->size = mx;

  // Set compressor pointers for library
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = mx;
}




METHODDEF(boolean)
iip_empty_output_buffer( j_compress_ptr cinfo )
{
  iip_dest_ptr dest = (iip_dest_ptr) cinfo->dest;
  size_t datacount = dest->size;

  // Copy the JPEG data to our output tile buffer
  if( datacount > 0 ){
    memcpy( dest->source, dest->buffer, datacount );
  }

  // Reset the pointer to the beginning of the buffer
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = datacount;

  return TRUE;
}




/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

void iip_term_destination( j_compress_ptr cinfo )
{
  iip_dest_ptr dest = (iip_dest_ptr) cinfo->dest;
  size_t datacount = dest->size - dest->pub.free_in_buffer;

  // Copy the JPEG data to our output tile buffer
  if( datacount > 0 ){

    //reallocate if needed
    if (datacount > dest->sourcesize )
    {
      free(dest->source);
      dest->source = (unsigned char*)malloc(datacount);
      dest->sourcesize=datacount;
    }
    memcpy( dest->source, dest->buffer, datacount );
  }

  dest->size = datacount;
}




void JPEGCompressor::InitCompression( RawTile& rawtile, unsigned int strip_height ) throw (string)
{

  // Do some initialisation
  data = (unsigned char*) rawtile.data;
  dest = &dest_mgr;


  // Set up the correct width and height for this particular tile
  width = rawtile.width;
  height = rawtile.height;
  channels = rawtile.channels;

  bool localData = false;

  if( (channels==2) || (channels==4) ){   //added by Zsolt Husz 14/05/2009 to remove alpha channel
      channels--;
      localData = true;
  }

  // Make sure we only try to compress images with 1 or 3 channels
  if( ! ( (channels==1) || (channels==3) )  ){
    throw string( "JPEGCompressor: JPEG can only handle images of either 1 or 3 channels" );
  }


  // We set up the normal JPEG error routines, then override error_exit.
  cinfo.err = jpeg_std_error( &jerr );

  // Overide the error_exit function with our own.
  //   cinfo.err.error_exit = iip_error_exit;

  // Hmmm, we have to do this assignment in C due to the strong type checking of C++
  //  or something like that. So, we use an extern "C" function declared at the top
  //  of this file and pass our arguments through this. I'm sure there's a better
  //  way of doing this, but this seems to work :/

  setup_error_functions( &cinfo );

  jpeg_create_compress( &cinfo );


  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */

  if( !cinfo.dest ){

    // first time for this JPEG object?
    cinfo.dest = ( struct jpeg_destination_mgr* )
      ( *cinfo.mem->alloc_small )
      ( (j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof( iip_destination_mgr ) );
  }


  dest = (iip_dest_ptr) cinfo.dest;
  dest->pub.init_destination = iip_init_destination;
  dest->pub.empty_output_buffer = iip_empty_output_buffer;
  dest->pub.term_destination = iip_term_destination;
  dest->strip_height = strip_height;
  dest->source = data;
  dest->sourcesize = rawtile.dataLength;


  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = channels;
  cinfo.in_color_space = ( channels == 3 ? JCS_RGB : JCS_GRAYSCALE );
  jpeg_set_defaults( &cinfo );

  // Set compression point quality (highest, but possibly slower depending
  //  on hardware) - must do this after we've set the defaults!
  cinfo.dct_method = JDCT_FASTEST;

  jpeg_set_quality( &cinfo, Q, TRUE );

  jpeg_write_tables(&cinfo);

  jpeg_start_compress( &cinfo, TRUE );


  // Copy the JPEG header data to our output tile buffer
  size_t datacount = dest->size - dest->pub.free_in_buffer;
  header_size = datacount;

  /* Assumes dest->source size is enough for the destination. If not then the server will crash. This might happen
  if the image is too small. Calling code has to account for this or buf must be reallocated here
  */
  if( datacount > 0 ){
    memcpy( header, dest->buffer, datacount );
  }


  // Reset the pointers
  size_t mx = cinfo.image_width * strip_height * cinfo.input_components;
  dest->size = mx;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = mx;

  if (localData)
     channels++;
}




/*
  We use a separate tile_height from the predefined strip_height because
  the tile height for the final row can be different
 */
unsigned int JPEGCompressor::CompressStrip( unsigned char* buf, unsigned int tile_height ) throw (string)
{
  bool localData = false;
  int channels_real=channels;

  if( (channels==2) || (channels==4) ){   //added by Zsolt Husz 14/05/2009 to remove alpha channel
      unsigned int i,size=width*tile_height;
      channels--;
      localData = true;

      unsigned char white[3]={255,255,255};
      for (i=0;i<size;i++) {
        if (((unsigned char*)buf)[i*(channels+1)+channels]==0)
           memcpy(((unsigned char*)buf)+i*channels, white, channels);
        else
           memcpy(buf+i*channels, buf+i*(channels+1), channels);
      }
  }

  int row_stride = width * channels;
  JSAMPROW row[1];
  dest->source = buf;
  dest->sourcesize = 0; 

  int i;
  for (i=tile_height;i>0;i--)
  {
    row[0] = &buf[ cinfo.next_scanline * row_stride ];
    jpeg_write_scanlines( &cinfo, row, 1 );
  }
  // Copy the JPEG data to our output tile buffer
  size_t datacount = dest->size - dest->pub.free_in_buffer;

  /* Assumes dest->source size is enough for the destination. If not then the server will crash. This might happen
  if the image is too small. Calling code has to account for this or buf must be reallocated here
  */
  if( datacount > 0 ){
    memcpy( dest->source, dest->buffer, datacount );
  }

  // Set compressor pointers for library
  size_t mx = (cinfo.image_width * dest->strip_height * cinfo.input_components) + 1024;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = mx;
  cinfo.next_scanline = 0;
  dest->size = mx;

  if (localData) { //added by Zsolt Husz 14/05/2009 to remove alpha channel
    channels=channels_real;
  }
  return datacount; 
}

unsigned int JPEGCompressor::Finish() throw (string)
{
  // Add our end of image (EOI) markers
  // This consists of a NULL byte, followed by the 0xFF, M_EOI
  *(dest->pub.next_output_byte)++ = (JOCTET) 0x00;
  *(dest->pub.next_output_byte)++ = (JOCTET) 0xFF;
  *(dest->pub.next_output_byte)++ = (JOCTET) 0xd9;
  dest->pub.free_in_buffer -= 3;

  // Copy the JPEG data to our output tile buffer

  /* Assumes dest->source size is enough for the destination. If not then the server will crash. This might happen
  if the image is too small. Calling code has to account for this or buf must be reallocated here
  */
  size_t datacount = dest->size - dest->pub.free_in_buffer;
  if( datacount > 0 ){
    memcpy( dest->source, dest->buffer, datacount );
  }

  // Tidy up and de-allocate memory
  // There seems to be a problem with jpeg_finish_compress :-(
  // We've manually added the EOI markers, so we don't have to bother calling it
  //   jpeg_finish_compress( &cinfo );
  jpeg_destroy_compress( &cinfo );
  

  return datacount;
}


int JPEGCompressor::Compress( RawTile& rawtile ) throw (string)
{

  // Do some initialisation
  
  data = (unsigned char*) rawtile.data;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  iip_destination_mgr dest_mgr;
  iip_dest_ptr dest = &dest_mgr;
  int dataLength = rawtile.dataLength;

  // Set up the correct width and height for this particular tile
  width = rawtile.width;
  height = rawtile.height;
  channels = rawtile.channels;


  bool localData = false;

  if( (channels==2) || (channels==4)){   //added by Zsolt Husz 14/05/2009 to remove alpha channel
      int i,size=width*height;
      channels--;
      localData = true;
      unsigned char white[3]={255,255,255};
      for (i=0;i<size;i++) {
        if (((unsigned char*)rawtile.data)[i*(channels+1)+channels]==0)
           memcpy(((unsigned char*)data)+i*channels, white, channels);
        else
           memcpy(((unsigned char*)data)+i*channels, ((unsigned char*)rawtile.data)+i*(channels+1), channels);
      }
     dataLength = size*channels;
  }


  // Make sure we only try to compress images with 1 or 3 channels
  if( ! ( (channels==1) || (channels==3) )  ){
    throw string( "JPEGCompressor: JPEG can only handle images of either 1 or 3 channels" );
  }


  // We set up the normal JPEG error routines, then override error_exit.
  cinfo.err = jpeg_std_error( &jerr );

  // Overide the error_exit function with our own.
  //   cinfo.err.error_exit = iip_error_exit;

  // Hmmm, we have to do this assignment in C due to the strong type checking of C++
  //  or something like that. So, we use an extern "C" function declared at the top
  //  of this file and pass our arguments through this. I'm sure there's a better
  //  way of doing this, but this seems to work :/

  setup_error_functions( &cinfo );

  jpeg_create_compress( &cinfo );


  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */

  if( !cinfo.dest ){

    // first time for this JPEG object?
    cinfo.dest = ( struct jpeg_destination_mgr* )
      ( *cinfo.mem->alloc_small )
      ( (j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof( iip_destination_mgr ) );
  }


  dest = (iip_dest_ptr) cinfo.dest;
  dest->pub.init_destination = iip_init_destination;
  dest->pub.empty_output_buffer = iip_empty_output_buffer;
  dest->pub.term_destination = iip_term_destination;
  dest->strip_height = 0;

  dest->source = data;
  dest->sourcesize = dataLength;

  // Set floating point quality (highest, but possibly slower depending
  //  on hardware)
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = channels;
  cinfo.in_color_space = ( channels == 3 ? JCS_RGB : JCS_GRAYSCALE );

  jpeg_set_defaults( &cinfo );

  // Set compression quality (highest, but possibly slower depending
  //  on hardware) - must do this after we've set the defaults!
  cinfo.dct_method = JDCT_FASTEST;

  jpeg_set_quality( &cinfo, Q, TRUE );

  jpeg_start_compress( &cinfo, TRUE );


  // Send the tile data
  unsigned int y;
  int row_stride = width * channels;


  // Try to pass the whole image array at once if it is less than 256x256 pixels:
  // Should be faster than scanlines.

  if( (row_stride * height) <= (256*256*channels) ){

    JSAMPROW *array = new JSAMPROW[height+1];
    for( y=0; y < height; y++ ){
      array[y] = &data[ y * row_stride ];
    }
    jpeg_write_scanlines( &cinfo, array, height );
    delete[] array;

  }
  else{
    JSAMPROW row[1];
    while( cinfo.next_scanline < cinfo.image_height ) {
      row[0] = &data[ cinfo.next_scanline * row_stride ];
      jpeg_write_scanlines( &cinfo, row, 1 );
    }
  }

  // Tidy up, get the compressed data size and de-allocate memory
  jpeg_finish_compress( &cinfo );
  y = dest->size;

  // check if more space was needed and data tile was reallocated
  if (rawtile.data != dest->source)
  {
    // no free is needed, because the was already freed in
    // iip_term_destination, just before reallocation
    rawtile.data = dest->source;
  }
  rawtile.dataLength = y;

  jpeg_destroy_compress( &cinfo );

  // Set the tile compression type
  rawtile.compressionType = JPEG;
  rawtile.quality = Q;

  // Return the size of the data we have compressed
  return y;

}
