#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzRemoteImage_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzRemoteImage.cc
* \author       Yiya Yang
* \date         September 2010
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
* \brief	Reads Woolz objects from a remote server socket for the
* 		Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>    /* sockaddr_in structure */
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* This entry allows the program to look up the name
   of the host and any alias names associated with it. */
#include <netdb.h>          /* /etc/hosts table entries */


#include "Log.h"
#include "WlzRemoteImage.h"
#include "WlzType.h"
#include "WlzProto.h"
#include "WlzExtFF.h"

int MAX_STR_LEN = 512;
int BUFFER_SIZE = 1024;
int ERROR = 1;

/////////////////// related to http protocol

/*!
* \return	Zero on failure.
* \ingroup	WlzIIPServer
* \brief	Parses the given url into a hostname, port number and
* 		resource identifier.
* \param	url			Given URL.
* \param	hostname		Return for hostname.
* \param	port			Return for port number.
* \param	identifier		Return for the resource identifier.
*/
int
WlzRemoteImage::parse_URL(const char *url, char *hostname, int *port, char *identifier)
{
  char protocol[MAX_STR_LEN], scratch[MAX_STR_LEN], *ptr=0, *nptr=0;
  
  strcpy(scratch, url);
  ptr = (char *)strchr(scratch, ':');
  if (!ptr)
    {
      LOG_ERROR("WlzRemoteImage::parse_URL --- no protocol specified "<<url);
      return 0;
    }
  strcpy(ptr, "\0");
  strcpy(protocol, scratch);
  
  if (strcmp(protocol, "http"))
    {
      LOG_ERROR("WlzRemoteImage::parse_URL --- not http protocol in "<<url);
      return 0;
    }
  
  strcpy(scratch, url);
  ptr = (char *)strstr(scratch, "//");
  if (!ptr)
    {
      LOG_ERROR("WlzRemoteImage::parse_URL --- no server in "<<url);
      return 0;
    }
  ptr += 2;
  
  strcpy(hostname, ptr);
  nptr = (char *)strchr(ptr, ':');
  if (!nptr)
    {
      *port = 80; /* use the default HTTP port number */
      nptr = (char *)strchr(hostname, '/');
    }
  else
    {	
      sscanf(nptr, ":%d", port);
      nptr = (char *)strchr(hostname, ':');
    }
  
  if (nptr)
    *nptr = '\0';
  
  nptr = (char *)strchr(ptr, '/');
  
  if (!nptr)
    {
      LOG_ERROR("WlzRemoteImage::parse_URL --- wrong url in "<<url);
      return 0;
    }
  
  strcpy(identifier, nptr);
  
  return 1;
}

/////////////////// related to http protocol

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Reads a Woolz object from the given URL.
* \param	url			Given URL.
*/
WlzObject*  
WlzRemoteImage::wlzHttpRead(char const* url)
{
  if (NULL == url ||
      8 > strlen(url)) {
    return NULL;
  }
  
  char hostname[MAX_STR_LEN];
  int port;
  char identifier[MAX_STR_LEN];
  
  // the server, construct an HTTP request based on the options specified in the 
  // command line, send the HTTP request to server, receive an HTTP response
  if (0 == WlzRemoteImage::parse_URL(url, hostname, &port, identifier)) {
    LOG_INFO("WlzRemoteImage::wlzHttpRead --- parse error: url="<<url<<" host = "<<hostname<<" port = "<<port<<" id = "<<identifier);
    return NULL;
  }
  
  int cnt = -1;
  int sock = -1;
  
  struct sockaddr_in myname; 
  
  struct hostent *hp;
  
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    LOG_ERROR("WlzRemoteImage::wlzHttpRead --- WlzIIPProxy client setup ");
    return NULL;
  }
  
  // set TCP_NODELAY
  int flag = 1;
  int result = setsockopt(sock, 
                          IPPROTO_TCP,
                          TCP_NODELAY,
                          (char *) &flag,
			  
                          sizeof(int));   
  if (result < 0) {
    LOG_ERROR("WlzRemoteImage::wlzHttpRead --- WlzRemoteSrv TCP_NODELAY failure");
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return NULL;
  }

  myname.sin_port = htons(port);
  myname.sin_family = AF_INET;   
  hp = gethostbyname(hostname);
  if (hp == NULL) {
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return NULL;
  }
  
  memcpy((void*) &myname.sin_addr.s_addr, (void*) hp -> h_addr_list[0], hp -> h_length);
  
  
  if ((connect(sock, (struct sockaddr *) &myname, sizeof(myname))) < 0) {
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return NULL;
  }
  
  char GET[1024];
  sprintf(GET, "GET %s HTTP/1.0\n\n", identifier);
  
  // send our get request for http 
  if (send(sock, GET, strlen(GET), 0) == -1) {
    LOG_ERROR("WlzRemoteImage::wlzHttpRead --- Error sending data.");
    return NULL;
  }

  char buffer[BUFFER_SIZE];
  memset(buffer,0x0,BUFFER_SIZE);    //  init line 
  
  int bufsize = read(sock, buffer, 16);
  int length = 0;
  int i = 0;
  char* content_length = NULL;
  char* content_type = NULL;
  WlzObject* ret = NULL;

  // has result
  i = -1;
  if (0 >= bufsize ||
      NULL == strstr(buffer, "OK")) {
    LOG_WARN("WlzRemoteImage::wlzHttpRead --- http non 200 response");
    return NULL;
  }
    
  // find data at begining of the http response
  
  // escape no Content-...
  memset(buffer,0x0,BUFFER_SIZE);    //  init line 
  while (1) {
    bufsize = recv(sock, buffer, BUFFER_SIZE, MSG_PEEK | MSG_WAITALL);
    if (0 >= bufsize)
      break;
    
    content_length  = strstr(buffer, "Content-Length");
    content_type = strstr(buffer, "Content-Type");
    if (NULL == content_length && NULL == content_type) {
      read(sock, buffer, bufsize);
      memset(buffer,0x0,BUFFER_SIZE);    //  init line 
      continue;
    }
    
    break;
  }
  
  // has Content-...
  if (0 >= bufsize) {
    LOG_WARN("WlzRemoteImage::wlzRemoteReadObj --- http header non Content-...");
    return NULL;
  }
  
  // get to the place after the last Content-...
  do {
    length = -1;
    if (NULL != content_length) {
      length = strlen(content_length) - strlen("Content-Length");
    }
    if (NULL != content_type) {
      if (-1 == length ||
	  length > strlen(content_type)) {
	length = strlen(content_type) - strlen("Content-type");
      }
    }
    length = strlen(buffer) - length;
    if (0 >= length) {
      LOG_WARN("WlzRemoteImage::wlzHttpRead --- http empty body");
      return NULL;
    }

    memset(buffer,0x0,BUFFER_SIZE);    //  init line 
    bufsize = read(sock, buffer, length);
    memset(buffer,0x0,BUFFER_SIZE);    //  init line 
    bufsize = recv(sock, buffer, BUFFER_SIZE, MSG_PEEK | MSG_WAITALL);
    content_length  = strstr(buffer, "Content-Length");
    content_type = strstr(buffer, "Content-Type");
  } while (NULL != content_length || NULL != content_type);

  memset(buffer,0x0,BUFFER_SIZE);    //  init line 

  // normally CRLFCRLF is ending of response header and begining of response data
  // however some machine may use LFLF for this purpose
  
  // find beginning of response data
  int cr = 0;
  int lf = 0;

  // find the beginning of the data
  while (lf < 2) {
      buffer[0] = 0x0;
      bufsize = read(sock, buffer, 1);
      if (0 >= bufsize)
	break;
      i = buffer[0];
      if (10 == i)
	lf++;
      if (13 == i)
	cr++;
  }

  if (0 == cr && 0 == lf) {
    LOG_WARN("WlzRemoteImage::wlzHttpRead --- http no header ending marker CRLFCRLF or LFLF");
    return NULL;
  }
  
  // sock data is the beginning of http data content  
  WlzErrorNum  errNum = WLZ_ERR_NONE;
  
  // at the begining of response data
  FILE *file = fdopen(sock, "r");
  if (NULL == file) {
    LOG_ERROR("WlzRemoteImage::wlzHttpRead --- fdopen failed at socket "<<sock);
  } else {
    ret = WlzReadObj(file, &errNum);
    if (NULL == ret) {
      LOG_WARN("WlzRemoteImage::wlzHttpRead --- read wlz via http-socket return null");
    } else {
      if (WLZ_ERR_NONE != errNum) {
	FILE *errorWlz = fopen("/tmp/error_from_IIPServer.wlz", "w");
	if (NULL != errorWlz) {
	  WlzWriteObj(errorWlz, ret);
	  fflush(errorWlz);
	  fclose(errorWlz);
	  errorWlz = NULL;
	}
	LOG_WARN("WlzRemoteImage::wlzHttpRead --- read wlz via http-socket return error " << WlzStringFromErrorNum(errNum, NULL));
	WlzFreeObj(ret);
	ret = NULL;
      }
    }
  }

  close(sock);
  return ret;
}

/*!
* \return	Woolz object or NULL on error.
* \ingroup	WlzIIPServer
* \brief	Reads a Woolz object from a remote server.
* \param	filename		Remote file path.
* \param	erverInput		Does not appear to be used.
* \param	portInput		Does not appear to be used.
*/
WlzObject*  
WlzRemoteImage::wlzRemoteReadObj(const char* filename, const char* serverInput, int portInput)
{
  if (NULL == filename)
    return NULL;

  // filename is a absolute path such as 
  // /opt/emageDBLocation/webImage/emage/dbImage/segment1/5725/5725_opt.wlz
  char url[MAX_STR_LEN];
  char name[MAX_STR_LEN];
  char* sToken = NULL;

  strcpy(name, filename);
  // escape special character in filename
  strcpy(url, "");
  if (NULL != strchr(name, ':')) {
    sToken = strtok(name, ":");
    strcat(url, sToken);
    sToken = strtok(NULL, ":");
    while (NULL != sToken) {
      strcat(url, "%3A");
      strcat(url, sToken);
      sToken = strtok(NULL, ":");
    }

    strcpy(name, url);
    strcpy(url, "");
  }

  //  sprintf(url, "http://caperdonich.hgu.mrc.ac.uk%s", name);
  sprintf(url, "http://tamdhu.hgu.mrc.ac.uk%s", name);

  WlzObject* ret = WlzRemoteImage::wlzHttpRead(url);

  if (NULL == ret) {
    LOG_WARN("WlzRemoteImage::wlzRemoteReadObj --- wlzHttpRead "<<url<<" return null");
    return ret;
  }

  // save to the local machine:
  // for 3D value object, save as memory-maped format
  // for the rest, save as normal format
  
  // make sure the directory is created
    char worker[MAX_STR_LEN];
    int length = 0;
    int i = 0;
    FILE  *fP = fopen(filename, "w");
    struct stat buf;
    int status = -1;

    while (NULL == fP) {
      strcpy(worker, filename);
      length = strlen(worker);
      while (length > 2) {
	for (i = length -1; i >= 0; i--) 
	  if (worker[i] == '/') {
	    worker[i] = '\0';
	    break;
	  }
	if (i < 0) {
	  length = -1;
	  break;
	}
	if (0 == mkdir(worker, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
	  length = 1;
	  break;
	}
	length = strlen(worker);
      }
      if (-1 == length) {
	fP = NULL;
	break;
      }
      fP = fopen(filename, "w");
    }

  if (NULL == fP) {
    LOG_WARN("WlzRemoteImage::wlzRemoteReadObj --- cannot create/open file to dump locally http wlz " << filename);
    return ret;
  }
  
  // ret cannot be written as memory mapped format
  // because its characters
  // instead written as normal wlz format
  if (NULL == ret->domain.core ||
      WLZ_3D_DOMAINOBJ != ret->type) {
    WlzWriteObj(fP, ret);
    fflush(fP);
    fclose(fP);
    fP = NULL;
    return ret;
  }

  WlzErrorNum  errNum = WLZ_ERR_NONE;
  
  // ret has problem with value type
  // write it as normal wlz format
  WlzGreyType	gType = WlzGreyTypeFromObj(ret, &errNum);
  if (WLZ_GREY_ERROR == gType) {
    WlzWriteObj(fP, ret);
    fflush(fP);
    fclose(fP);
    fP = NULL;
    return ret;
  }
  
  // only 3D value wlz object, save  to the local machine as tiled memory format for fast access 
  WlzObject *domObj = NULL;
  WlzObject* outObj = NULL;
  const size_t	tlSz = 4096;
  WlzPixelV	bgdV = WlzGetBackground(ret, &errNum);
  WlzIVertex3	offset;
  int copy = 1;
  
  if (WLZ_ERR_NONE != errNum) {
    bgdV.v.dbv = 0.0;
    bgdV.type = WLZ_GREY_DOUBLE;
  }
  
  WLZ_VTX_3_SET(offset, 0, 0, 0);
  
  domObj = WlzMakeMain(ret->type, ret->domain, ret->values, NULL, NULL,
  		       &errNum);
  if(errNum == WLZ_ERR_NONE && NULL != domObj)
  {
    outObj = WlzMakeTiledValuesFromObj(domObj, tlSz, copy, gType,
                                       0, NULL, bgdV, &errNum);
  }
  
  if(errNum == WLZ_ERR_NONE && NULL != outObj)
    WlzWriteObj(fP, outObj);
  else
    WlzWriteObj(fP, ret);
  
  fflush(fP);
  fclose(fP);
  fP = NULL;
  
  // free memory
  if (NULL != domObj) {
    WlzFreeObj(domObj);
    domObj = NULL;
  }
  if (NULL != outObj) {
    WlzFreeObj(outObj);
    outObj = NULL;
  }
  
  // re-read wlz obj as memory mapped format
  if (NULL != ret) {
    WlzFreeObj(ret);
    fP = fopen(filename, "r");
    ret = WlzReadObj(fP, NULL);
    fflush(fP);
    fclose(fP);
    fP = NULL;
  }

  if (NULL == ret) {
    LOG_WARN("WlzRemoteImage::wlzHttpReadObj --- error in reading dumped local wlz file "<<filename);
  }

  return ret;
}
