##
# \file         Makefile.am
# \author       Zsolt Husz
# \date         June 2008
# \version      $Id$
# \par
# Address:
#               MRC Human Genetics Unit,
#               Western General Hospital,
#               Edinburgh, EH4 2XU, UK.
# \par
# Copyright (C) 2008 Medical research Council, UK.
#               based on iipsrv 0.9.7, 2000-2006 Ruven Pillay 
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be
# useful but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#
# \brief        Process this file with automake to produce Makefile.in
# \todo         -
# \bug          None known.
##
## Process this file with automake to produce Makefile.in

noinst_PROGRAMS =	wlziipsrv.fcgi

INCLUDES =		@LIBWLZ_INCLUDES@ @LIBFCGI_INCLUDES@  @JPEG_INCLUDES@ @TIFF_INCLUDES@ @PNG_INCLUDES@
LIBS =			@LIBWLZ_LIBS@ @LIBS@ @LIBFCGI_LIBS@ @DL_LIBS@ @JPEG_LIBS@ @TIFF_LIBS@ @PNG_LIBS@ -lz -lm
AM_LDFLAGS =		@LIBWLZ_LDFLAGS@ @LIBFCGI_LDFLAGS@

if ENABLE_MODULES

wlziipsrv_fcgi_SOURCES = \
			Main.cc \
			IIPImage.h \
			IIPImage.cc \
			TPTImage.h \
			TPTImage.cc \
			JPEGCompressor.h \
			JPEGCompressor.cc \
			PNGCompressor.h \
			PNGCompressor.cc \
			RawTile.h \
			Timer.h \
			Cache.h \
			TileManager.h \
			TileManager.cc \
			Tokenizer.h \
			IIPResponse.h \
			IIPResponse.cc \
			View.h \
			View.cc \
			ColourTransforms.h \
			ColourTransforms.cc \
			Environment.h \
			Writer.h \
			Task.h \
			Task.cc \
			OBJ.cc \
			FIF.cc \
			WLZ.cc \
			WlzRemoteImage.cc \
			WlzImage.cc \
			JTL.cc \
			SEL.cc \
			PTL.cc \
			TIL.cc \
			ICC.cc \
			CVT.cc \
			DSOImage.h \
			DSOImage.cc

else

wlziipsrv_fcgi_SOURCES = \
			Main.cc \
			IIPImage.h \
			IIPImage.cc \
			TPTImage.h \
			TPTImage.cc \
			JPEGCompressor.h \
			JPEGCompressor.cc \
			PNGCompressor.h \
			PNGCompressor.cc \
			RawTile.h \
			Timer.h \
			Cache.h \
			TileManager.h \
			TileManager.cc \
			Tokenizer.h \
			IIPResponse.h \
			IIPResponse.cc \
			View.h \
			View.cc \
			ColourTransforms.h \
			ColourTransforms.cc \
			Environment.h \
			Writer.h \
			Task.h \
			Task.cc \
			OBJ.cc \
			FIF.cc \
			WLZ.cc \
			WlzRemoteImage.cc \
			WlzImage.cc \
			PTL.cc \
			SEL.cc \
			JTL.cc \
			TIL.cc \
			ICC.cc \
			CVT.cc

endif
