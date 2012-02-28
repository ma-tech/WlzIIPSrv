#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _CVT_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         CVT.cc
* \author       Ruven Pillay, Bill Hill
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
* Copyright (C) 2006 Ruven Pillay.
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
* \brief	IIP CVT Command Handler Class Member Function.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"
#include "ColourTransforms.h"


using namespace std;



/**
 * 
 * @param session 
 * @param argument 
 */
void CVT::run( Session* session, std::string argument ){

  Timer tile_timer;
  this->session = session;
  LOG_INFO("CVT handler reached");
  checkImage();
  LOG_COND_INFO(command_timer.start());
  // Put the argument into lower case
  transform( argument.begin(), argument.end(), argument.begin(), ::tolower );


  // For the moment, only deal with JPEG and PNG. If we have specified something else, give a warning
  // and send JPEG anyway
  if( argument != "jpeg" && argument != "png"){ // png added by Zsolt Husz, 8/05/2009
    LOG_WARN("CVT :: Unsupported request: '" << argument <<
             "'. Sending JPEG.");
    argument = "jpeg";
  }

  if( argument == "jpeg" || argument == "png") { // png added by Zsolt Husz, 8/05/2009

    enum CompressionType requestType=JPEG; // png added by Zsolt Husz, 8/05/2009

    if (argument == "png") // png added by Zsolt Husz, 8/05/2009
      requestType = PNG;

    unsigned int n;
    int cielab = 0;

    LOG_INFO("CVT :: JPEG/PNG output handler reached");

    // Get a fake tile in case we are dealing with a sequence
    (*session->image)->loadImageInfo( session->view->xangle, session->view->yangle );

    // Calculate the number of tiles at the requested resolution
    unsigned int im_width = (*session->image)->getImageWidth();
    unsigned int im_height = (*session->image)->getImageHeight();
    int num_res = (*session->image)->getNumResolutions();

    session->view->setImageSize( im_width, im_height );
    session->view->setMaxResolutions( num_res );

    session->viewParams->setAlpha(requestType==PNG);
    (*session->image)->recomputeChannel(requestType==PNG); //forces channel number update

    int requested_res = session->view->getResolution();
    im_width = session->view->getImageWidth();
    im_height = session->view->getImageHeight();

    LOG_INFO("CVT :: image set to " << im_width << " x " << im_height <<
	      " using resolution " << requested_res);

    // The tile size of the source tile
    unsigned int src_tile_width = (*session->image)->getTileWidth();
    unsigned int src_tile_height = (*session->image)->getTileHeight();

    // The tile size of the destination tile
    unsigned int dst_tile_width = src_tile_width;
    unsigned int dst_tile_height = src_tile_height;

    // The basic tile size ie. not the current tile
    unsigned int basic_tile_width = src_tile_width;

    unsigned int rem_x = im_width % src_tile_width;
    unsigned int rem_y = im_height % src_tile_height;

    unsigned int channels = (*session->image)->getNumChannels();

    // The number of tiles in each direction
    unsigned int ntlx = (im_width / src_tile_width) + (rem_x == 0 ? 0 : 1);
    unsigned int ntly = (im_height / src_tile_height) + (rem_y == 0 ? 0 : 1);

    int len;

    // If we have a region defined, calculate our viewport
    unsigned int view_left, view_top, view_width, view_height;
    unsigned int startx, endx, starty, endy, xoffset, yoffset;
    unsigned int tile_width_padding;

    if( session->view->viewPortSet() ){

      // Set the absolute viewport size and extract the co-ordinates
      view_left = session->view->getViewLeft();
      view_top = session->view->getViewTop();
      view_width = session->view->getViewWidth();
      view_height = session->view->getViewHeight();

      // Calculate the start tiles
      startx = (unsigned int) ( view_left / src_tile_width );
      starty = (unsigned int) ( view_top / src_tile_height );
      xoffset = view_left % src_tile_width;
      yoffset = view_top % src_tile_height;
      endx = (unsigned int) ( (view_width + view_left) / src_tile_width ) + 1;
      endy = (unsigned int) ( (view_height + view_top) / src_tile_height ) + 1;

      LOG_INFO("CVT :: view port is set: image: " <<
                im_width << "x" << im_height << ". View Port: " <<
		view_left << "," << view_top << "," << view_width << "," <<
		view_height << endl << "CVT :: Pixel Offsets: " << startx <<
		"," << starty << "," << xoffset << "," << yoffset << endl <<
		"CVT :: End Tiles: " << endx << "," << endy);
    } else {
      LOG_INFO("CVT :: No view port set");
      view_left = 0;
      view_top = 0;
      view_width = im_width;
      view_height = im_height;
      startx = starty = xoffset = yoffset = 0;
      endx = ntlx;
      endy = ntly;
    }


    // Allocate memory for a strip (tile height x image width)
    unsigned int o_channels = channels;
    if( session->view->shaded ) o_channels = 1;

    unsigned char* buf = new unsigned char[view_width * src_tile_height * o_channels + 4000]; // If image to small then 4000 bytes 
                                                                              // should be enought to cover the compression overhead

    unsigned char* bufDest = buf;                                  // destination buffer. PNGs need destination different from source
    if(requestType == PNG) { // png added by Zsolt Husz, 8/05/2009
        bufDest = new unsigned char[view_width * src_tile_height * o_channels + 4000]; // If image to small then 4000 bytes
    }                                                                                     // should be enought to cover the compression overhead


    // Create a RawTile for the entire image
    RawTile complete_image( 0, 0, 0, 0, view_width, view_height, o_channels, 8 );

    complete_image.dataLength = view_width * src_tile_height * o_channels + 4000;
    complete_image.data = buf;

    if(requestType == PNG) { // png added by Zsolt Husz, 8/05/2009

    // Initialise our PNH compression object
    len = session->png->InitCompression( complete_image, src_tile_height );
#ifndef DEBUG
      session->out->printf( // 			  "Pragma: no-cache\r\n"
			 "Last-Modified: Mon, 1 Jan 2000 00:00:00 GMT\r\n"
			 "ETag: \"CVT\"\r\n"
 			 "Content-type: image/png\r\n"
			 "Content-disposition: inline;filename=\"cvt.png\""
			 "\r\n\r\n" );
#endif
      // Send the PNG header to the client
      if( session->out->putStr( (const char*) complete_image.data, len ) != len ){
	LOG_ERROR("CVT :: Error writing png header");
      }
    } else { //JPEG
      // Initialise our JPEG compression object

        session->jpeg->InitCompression( complete_image, src_tile_height );
#ifndef DEBUG
      session->out->printf( // 			  "Pragma: no-cache\r\n"
			 "Last-Modified: Mon, 1 Jan 2000 00:00:00 GMT\r\n"
			 "ETag: \"CVT\"\r\n"
 			 "Content-type: image/jpeg\r\n"
			 "Content-disposition: inline;filename=\"cvt.jpg\""
			 "\r\n\r\n" );
#endif
    // Send the JPEG header to the client
      len = session->jpeg->getHeaderSize();
      if( session->out->putStr( (const char*) session->jpeg->getHeader(), len ) != len ){
	LOG_ERROR("CVT :: Error writing jpeg header");
      }
    }

    // Decode the image strip by strip and dynamically compress with JPEG

    for( unsigned int i=starty; i<endy; i++ ){
      unsigned int buffer_index = 0;
      // Keep track of the current pixel boundary horizontally. ie. only up
      //  to the beginning of the current tile boundary.
      int current_width = 0;

      for( unsigned int j=startx; j<endx; j++ ){
        LOG_COND_INFO(tile_timer.start());
	// Get an uncompressed tile from our TileManager
	TileManager tilemanager( session->tileCache, *session->image, session->jpeg, session->png);
	RawTile rawtile = tilemanager.getTile( requested_res, (i*ntlx) + j, session->view->xangle, session->view->yangle, UNCOMPRESSED );

	LOG_INFO("CVT :: Tile access time " << tile_timer.getTime() << "us");

	// Check the colour space - CIELAB images will need to be converted
	if( (*session->image)->getColourSpace() == CIELAB ){
	  cielab = 1;
	  LOG_INFO("CVT :: Converting from CIELAB->sRGB");
	}

#ifdef WLZ_IIP_LOG
	// Only log this out once per image
        if((logCat != NULL) &&
	   (logCat->getPriority() >= log4cpp::Priority::INFO) &&
	   (i==starty) && (j==starty))
	{
	  LOG_INFO("CVT :: Tile data is " << rawtile.channels <<
	            " channels, " << rawtile.bpc << " bits per channel");
	}
#endif
	// Set the tile width and height to be that of the source tile
	// - Use the rawtile data because if we take a tile from cache
	//   the image pointer will not necessarily be pointing to the
	//   the current tile
	//	src_tile_width = (*session->image)->getTileWidth();
	//	src_tile_height = (*session->image)->getTileHeight();
	src_tile_width = rawtile.width;
	src_tile_height = rawtile.height;
	tile_width_padding = rawtile.width_padding;
	dst_tile_width = src_tile_width;
	dst_tile_height = src_tile_height;

	// Variables for the pixel offset within the current tile
	unsigned int xf = 0;
	unsigned int yf = 0;

	// If our viewport has been set, we need to modify our start
	// and end points on the source image
	if( session->view->viewPortSet() ){

	  if( j == startx ){
	    // Calculate the width used in the current tile
	    // If there is only 1 tile, the width is just the view width
	    if( j < endx - 1 ) dst_tile_width = src_tile_width - xoffset;
	    else dst_tile_width = view_width;
	    xf = xoffset;
	  }
	  else if( j == endx-1 ){
	    dst_tile_width = (view_width+view_left) % basic_tile_width;
	  }

	  if( i == starty ){
	    // Calculate the height used in the current row of tiles
	    // If there is only 1 row the height is just the view height
	    if( i < endy - 1 ) dst_tile_height = src_tile_height - yoffset;
	    else dst_tile_height = view_height;
	    yf = yoffset;
  
	  }
	  else if( i == endy-1 ){
	    dst_tile_height = (view_height+view_top) % basic_tile_width;
  	  }
	  LOG_INFO("CVT :: destination tile height: " << dst_tile_height <<
	            ", tile width: " << dst_tile_width);
	}


	// Copy our tile data into the appropriate part of the strip memory
	// one whole tile width at a time
	for( unsigned int k=0; k<dst_tile_height; k++ ){

	  buffer_index = (current_width*channels) + (k*view_width*channels);
	  unsigned int inx = ((k+yf)*(basic_tile_width-tile_width_padding)*channels) + (xf*channels);
	  unsigned char* ptr = (unsigned char*) rawtile.data; 

	  // If we have a CIELAB image, convert each pixel to sRGB first
	  // Otherwise just do a fast memcpy
	  if( cielab ){
	    for( n=0; n<dst_tile_width*channels; n+=channels ){
              iip_LAB2sRGB( &ptr[inx + n], &bufDest[buffer_index + n] );
	    }
	  }
	  else if( session->view->shaded ){
	    int m;
	    for( n=0, m=0; n<dst_tile_width*channels; n+=channels, m++ ){
              shade( &ptr[inx + n], &bufDest[current_width + (k*view_width) + m],
		     session->view->shade[0], session->view->shade[1],
		     session->view->getContrast() );
	    }
	  }
	  // If we have a 16 bit image, multiply by the contrast adjustment if it exists
	  // and clip to 8 bits
	  else if( rawtile.bpc == 16 ){
	    unsigned short* sptr = (unsigned short*) rawtile.data;
	    for( n=0; n<dst_tile_width*channels; n++ ){
	      float v = (float)sptr[inx+n] * session->view->getContrast();
	      if( v > 255.0 ) v = 255.0;
              bufDest[buffer_index + n] = (unsigned char) v;
	    }
	  }
	  else if( (rawtile.bpc == 8) && (session->view->getContrast() != 1.0) ){
	    unsigned char* sptr = (unsigned char*) rawtile.data;
	    for( n=0; n<dst_tile_width*channels; n++ ){
	      float v = (float)sptr[inx+n] * session->view->getContrast();
	      if( v > 255.0 ) v = 255.0;
              bufDest[buffer_index + n] = (unsigned char) v;
	    }
          } else {
                memcpy( &bufDest[buffer_index],	&ptr[inx], dst_tile_width*channels );
          }
	}
	current_width += dst_tile_width;
      }

      // Compress the strip
      if(requestType == PNG) // png added by Zsolt Husz, 8/05/2009
        len = session->png->CompressStrip( bufDest, dst_tile_height );
      else
        len = session->jpeg->CompressStrip( bufDest, dst_tile_height );  // bug fix 15/05/2009

      LOG_INFO("CVT :: Compressed data strip length is " << len);

      // Send this strip out to the client
      if(len != session->out->putStr((const char* )complete_image.data, len)){
	LOG_ERROR("CVT :: Error writing jpeg strip data: " << len);
      }

      if( session->out->flush() == -1 ) {
	LOG_ERROR("CVT :: Error flushing jpeg tile");
      }
    }

    // Finish off the image compression
    if(requestType == PNG)  // png added by Zsolt Husz, 8/05/2009
      len = session->png->Finish();
    else
      len = session->jpeg->Finish();

    if(session->out->putStr((const char* )complete_image.data, len) != len){
      LOG_ERROR("CVT :: Error writing jpeg EOI markers");
    }

    // Finish off the flush the buffer
    //session->out->printf( "\r\n" ); //was possible bug: causes incorrect packet length, that results in crash, Z Husz 16/04/2010

    if( session->out->flush()  == -1 ) {
      LOG_ERROR("CVT :: Error flushing image");
    }

    // Inform our response object that we have sent something to the client
    session->response->setImageSent();


    // Don't forget to delete our strip of memory
    if (bufDest!=buf)
       delete[] bufDest;
    delete[] buf;

  } // End of if( argument == "jpeg" || argument == "png")

  // Total CVT response time
  LOG_INFO("CVT :: Total command time " << command_timer.getTime() << "us");
}
