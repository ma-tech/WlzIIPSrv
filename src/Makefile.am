##
# \file         Makefile.am
# \author       Zsolt Husz, Bill Hill
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

noinst_PROGRAMS 	= \
			WlzExpTest \
			WlzIIPStringParserTest \
			wlziipsrv.fcgi


MYLEX			= @MYLEX@
MYYACC			= @MYYACC@

BUILT_SOURCES 		= \
			WlzExpLexer.h \
			WlzExpLexer.c \
			WlzExpParser.tab.c

INCLUDES 		= \
			@LIBWLZ_INCLUDES@ \
			@LIBLOG4CPP_INCLUDES@  \
			@LIBFCGI_INCLUDES@  \
			@LIBNIFTI_INCLUDES@ \
			@JPEG_INCLUDES@ \
			@TIFF_INCLUDES@ \
			@PNG_INCLUDES@
LIBS 			= \
			@LIBWLZ_LIBS@ \
			@LIBS@ \
			@LIBLOG4CPP_LIBS@ \
			@LIBFCGI_LIBS@ \
			@DL_LIBS@ \
			@LIBNIFTI_LIBS@ \
			@JPEG_LIBS@ \
			@TIFF_LIBS@ \
			@PNG_LIBS@ \
			@MYLEX_LIBS@ \
			-lz -lm -lpthread

AM_LDFLAGS =		\
			@LIBWLZ_LDFLAGS@ \
			@LIBFCGI_LDFLAGS@

if ENABLE_MODULES
  DSO_SOURCES 		= \
			DSOImage.h \
			DSOImage.cc
else
  DSO_SOURCES 		= 
endif

wlziipsrv_fcgi_SOURCES 	= \
			CVT.cc \
			Cache.h \
			ColourTransforms.cc \
			ColourTransforms.h \
			Environment.h \
			FIF.cc \
			ICC.cc \
			IIPImage.cc \
			IIPImage.h \
			IIPResponse.cc \
			IIPResponse.h \
			ImageMap.cc \
			JPEGCompressor.cc \
			JPEGCompressor.h \
			JTL.cc \
			Log.h \
			MAP.cc \
			Main.cc \
			OBJ.cc \
			PNGCompressor.cc \
			PNGCompressor.h \
			PTL.cc \
			RawTile.h \
			SEL.cc \
			TIL.cc \
			TPTImage.cc \
			TPTImage.h \
			Task.cc \
			Task.h \
			TileManager.cc \
			TileManager.h \
			Timer.h \
			Tokenizer.h \
			View.cc \
			View.h \
			ViewParameters.cc \
			ViewParameters.h \
			WLZ.cc \
			WlzExpLexer.lex \
			WlzExpParser.yacc \
			WlzExpression.c \
			WlzIIPStringParser.c \
			WlzImage.cc \
			WlzObjectCache.cc \
			WlzRemoteImage.cc \
			Writer.h \
			$(BUILT_SOURCES) \
			$(DSO_SOURCES)

WlzExpTest_SOURCES	= \
			WlzExpTestMain.c \
			WlzExpression.c \
			WlzExpLexer.lex \
			WlzExpParser.yacc \
			$(BUILT_SOURCES)

WlzExpLexer.c WlzExpLexer.h:	WlzExpLexer.lex
			$(MYLEX) --outfile=WlzExpLexer.c \
		        --header-file=WlzExpLexer.h WlzExpLexer.lex

WlzIIPStringParserTest_SOURCES	= \
			WlzIIPStringParserTestMain.c \
			WlzIIPStringParser.c
			$(BUILT_SOURCES)

# Autoconf, automake, yacc and bison don't work well together. Here
# instead of outputting WlzExpParser.tab.c we get WlzExpParser.tab.cacc!
WlzExpParser.h WlzExpParser.tab.c:	WlzExpParser.yacc
			$(MYYACC) --defines=WlzExpParser.h WlzExpParser.yacc
			mv WlzExpParser.tab.cacc WlzExpParser.tab.c

WlzExpLexer.o:     	WlzExpLexer.c WlzExpParser.tab.c
			$(COMPILE) -c WlzExpLexer.c
