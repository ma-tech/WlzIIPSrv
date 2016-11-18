#!/bin/sh
# This script will configure and build the Woolz IIP Server. Uncomment
# the appropriate configure command lines for the build you want. The
# easiest way to use this script is probably to copy it to mybuild.sh and
# the edit that script.

set -x
# In most cases a simple autoreconf should be sufficient
autoreconf
# If you hit problems with missing files or libtool use the following
# autoreconf
# autoreconf -i --force

#export MA=$HOME
export MA=/opt/MouseAtlas/
#export MA=$HOME/MouseAtlas/Build/
#export MA=$HOME/MouseAtlas/Build/debug

# Configure for an optimised build on systems with compilers and x86_64
# processors developed after 2013
./configure  \
    --enable-optimise \
    --enable-static-fcgi \
    --with-fcgi-incl=/opt/fcgi/include --with-fcgi-lib=$MA/lib/ \
    --with-log4cpp-incl=$MA/include --with-log4cpp-lib=$MA/lib \
    --with-wlz-incl=$MA/include --with-wlz-lib=$MA/lib \
    --with-nifti-incl=$MA/include --with-nifti-lib=$MA/lib \
    --with-tiff-includes=$MA/include --with-tiff-libraries=$MA/lib \
    --with-jpeg-includes=$MA/include --with-jpeg-libraries=$MA/lib \
    --with-png-includes=$MA/include --with-png-libraries=$MA/lib \
    --enable-openmp --enable-avx2 --enable-lto

# Configure for an optimised build on older systems
# ./configure  \
#     --enable-optimise \
#     --enable-static-fcgi \
#     --with-fcgi-incl=/opt/fcgi/include --with-fcgi-lib=$MA/lib/ \
#     --with-log4cpp-incl=$MA/include --with-log4cpp-lib=$MA/lib \
#     --with-wlz-incl=$MA/include --with-wlz-lib=$MA/lib \
#     --with-nifti-incl=$MA/include --with-nifti-lib=$MA/lib \
#     --with-tiff-includes=$MA/include --with-tiff-libraries=$MA/lib \
#     --with-jpeg-includes=$MA/include --with-jpeg-libraries=$MA/lib \
#     --with-png-includes=$MA/include --with-png-libraries=$MA/lib \
#     --enable-openmp

# Configure for a debug build
# ./configure  \
#     --enable-debug \
#     --enable-static-fcgi \
#     --with-fcgi-incl=/opt/fcgi/include --with-fcgi-lib=$MA/lib/ \
#     --with-log4cpp-incl=$MA/include --with-log4cpp-lib=$MA/lib \
#     --with-wlz-incl=$MA/include --with-wlz-lib=$MA/lib \
#     --with-nifti-incl=$MA/include --with-nifti-lib=$MA/lib \
#     --with-tiff-includes=$MA/include --with-tiff-libraries=$MA/lib \
#     --with-jpeg-includes=$MA/include --with-jpeg-libraries=$MA/lib \
#     --with-png-includes=$MA/include --with-png-libraries=$MA/lib

