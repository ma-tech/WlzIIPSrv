#ifndef _IIPRESPONSE_H
#define _IIPRESPONSE_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _IIPImage_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         IPResponse.h
* \author       Ruven Pillay, Zsolt Husz, Bill Hill
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
* Copyright (C), [2003-2004] Ruven Pillay.
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
* \brief	IIP Response Handler Class.
* \ingroup    	WlzIIPServer
*/

#include <string>

/// Class to handle non-image IIP responses including errors

class IIPResponse{


 private:

  std::string mimeType;            // Mime type header
  std::string eof;                 // End of response delimitter eg "\r\n"
  std::string protocol;            // IIP protocol version
  std::string responseBody;        // The main response
  std::string error;               // Error message
  bool sent;                       // Indicate whether a response has been sent


 public:

  /// Constructor
  IIPResponse();


  /// Set the IIP protocol version
  /** \param p IIP protocol version */
  void setProtocol( const std::string& p ) { protocol = p; };


  /// Add a response string
  /** \param r response string */
  void addResponse( const std::string& r ); 


  /// Add a response string
  /** \param c response string */
  void addResponse( const char* c );


  /// Add a response string
  /** \param c response string
      \param a integer value
   */
  void addResponse( const char* c, int a );


  /// Add a response string
  /** \param c response string
      \param a long long value
   */
  void addResponse( const char* c, long long a );

  /// Add a response string
  /** \param c response string
      \param a string reply
   */
  void addResponse( std::string c, const char* a );


  /// Add a response string
  /** \param c response string
      \param a integer value
      \param b another integer value
   */
  void addResponse( const char* c, int a, int b );

  void addResponse( const char *c0, int i0, const char *c1,
                    double d0, double d1, double d2, double d3,
		    double d4, double d5);

  /// Set an error
  /** \param code error code
      \param arg the argument supplied by the client
   */
  void setError( const std::string& code, const std::string& arg );


  /// Get a formatted string to send back
  std::string formatResponse();


  /// Indicate whether this object has had any arguments passed to it
  bool isSet(){
    if( error.length() || responseBody.length() || protocol.length() ) return true;
    else return false;
  }


  /// Indicate whether we have an error message
  bool errorIsSet(){
    if( error.length() ) return true;
    else return false;
  }


  /// Set the sent flag indicating that some sort of response has been sent
  void setImageSent() { sent = true; };


  /// Indicate whether a response has been sent
  bool imageSent() { return sent; };


  /// Display our advertising banner ;-)
  /** \param version server version */
  std::string getAdvert( const std::string& version );


  /// Add a response string
  /** \param c response string
      \param a double value
      \param b another double value
   */
  void addResponse( const char* c, double a, double b );

  /// Add a response string
  /** \param d response string
      \param a first double value
      \param b second double value
      \param c third double value
   */
  void addResponse( const char* d, double a, double b, double c );

  /// Add a response string
  /** \param g response string
      \param a first double value
      \param b second double value
      \param c third double value
      \param d forth double value
   */
  void addResponse( const char* g, double a, double b, double c, double d );

  /// Add a response string
  /** \param s response string
      \param n number of values
      \param values array of n values
   */
  void addResponse( const char* s, int n, int * values );

  /// Add a response string
  /** \param g response string
      \param a first integer value
      \param b second integer value
      \param c third integer value
      \param d forth integer value
      \param e fifth integer value
      \param f sixth integer value
   */
  void addResponse( const char* g, int a, int b, int c, int d, int e, int f  );


};


#endif
