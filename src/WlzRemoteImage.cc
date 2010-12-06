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


#include "WlzRemoteImage.h"
#include "WlzType.h"
#include <WlzProto.h>
#include <WlzExtFF.h>

int MAX_STR_LEN = 512;
int BUFFER_SIZE = 1024;
int ERROR = 1;

/////////////////// related to http protocol

// Parses an url into hostname, port number and resource identifier.
int
WlzRemoteImage::parse_URL(const char *url, char *hostname, int *port, char *identifier)
{
  char protocol[MAX_STR_LEN], scratch[MAX_STR_LEN], *ptr=0, *nptr=0;
  
  strcpy(scratch, url);
  ptr = (char *)strchr(scratch, ':');
  if (!ptr)
    {
      perror("Wrong url: no protocol specified\n");
      return 0;
    }
  strcpy(ptr, "\0");
  strcpy(protocol, scratch);
  
  if (strcmp(protocol, "http"))
    {
      sprintf(scratch, "Wrong protocol: %s\n", protocol);
      perror(scratch);
      return 0;
    }
  
  strcpy(scratch, url);
  ptr = (char *)strstr(scratch, "//");
  if (!ptr)
    {
      perror("Wrong url: no server specified\n");
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
      perror("Wrong url: no file specified\n");
      return 0;
    }
  
  strcpy(identifier, nptr);
  
  return 1;
}

/////////////////// related to http protocol

WlzObject*  
WlzRemoteImage::wlzHttpRead(char const* url)
{
  if (NULL == url ||
      8 > strlen(url))
    return NULL;
  
  char hostname[MAX_STR_LEN];
  int port;
  char identifier[MAX_STR_LEN];
  
  // the server, construct an HTTP request based on the options specified in the 
  // command line, send the HTTP request to server, receive an HTTP response
  if (0 == WlzRemoteImage::parse_URL(url, hostname, &port, identifier))
    return NULL;
  
  int cnt = -1;
  int sock = -1;
  
  struct sockaddr_in myname; 
  
  struct hostent *hp;
  
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("WlzIIPProxy client setup \n");
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
    perror("WlzRemoteSrv TCP_NODELAY failure \n");
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
  sprintf(GET, "GET /%s HTTP/1.0\n\n", identifier);
  
  // send our get request for http 
  if (send(sock, GET, strlen(GET), 0) == -1) {
    perror("Error sending data.");
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
  if (0 < bufsize &&
      NULL != strstr(buffer, "OK")) {
    
    // find data begining of the http response
    memset(buffer,0x0,BUFFER_SIZE);    //  init line 
    while (1) {
    bufsize = recv(sock, buffer, BUFFER_SIZE, MSG_PEEK | MSG_WAITALL);
      if (0 >= bufsize)
	break;

      content_length  = strstr(buffer, "Content-Length");
      content_type = strstr(buffer, "Content-Type");

      if (NULL == content_length)
	i = 0;
      else 
	i = strlen(content_length);
      if (NULL == content_type)
	length = 0;
      else 
	length = strlen(content_type);

      if (0 == i)
	i = length;
      else {
	if (i  > length)
	  i =  length;
      }

      length = strlen(buffer) - i;
      read(sock, buffer, length);
      if (0 == i) {
	memset(buffer,0x0,BUFFER_SIZE);    //  init line 
	continue;
      }

      break;
    }

    // escape to last Content-Type/Conenet-Length

    // normally CRLFCRLF is ending of response header and begining of response data
    // however some machine may use LFLF for this purpose
    
    // find beginning of response data
    int cr = -1;
    int lf = -1;
    while (1) {
      buffer[0] = 0x0;
      bufsize = read(sock, buffer, 1);
      if (0 >= bufsize)
	break;
      i = buffer[0];
      if (10 == i) {
	lf = 1;
	read(sock, buffer, 1);
	i = (int)buffer[0];
	if (10 == i || 13 == i)
	  break;
      } 
      if (13 == i) {
	cr = 1;
	read(sock, buffer, 3);
	if ((10 == buffer[0] || 13 == buffer[0]) &&
	    (10 == buffer[1] || 13 == buffer[1]) &&
	    (10 == buffer[2] || 13 == buffer[2]))
	  break;
      }
    }

    if (-1 == cr && -1 == lf)
      return NULL;

    // sock data is the beginning of http data content
    
    WlzErrorNum  errNum = WLZ_ERR_NONE;
    
    // at the begining of response data
    FILE *file = fdopen(sock, "r");
    if (NULL == file)
      printf("fdopen failed %d\n", sock);
    else {
      ret = WlzReadObj(file, &errNum);
      if (NULL == ret)
	perror("Fail to read wlz object\n");
      else {
	if (WLZ_ERR_NONE != errNum) {
	  perror("Error in reading wlz object\n");
	  WlzFreeObj(ret);
	  ret = NULL;
	}
      }
    }
  }
  close(sock);
  return ret;
}

WlzObject*  
WlzRemoteImage::wlzRemoteReadObj(const char* filename, const char* serverInput, int portInput)
{
  if (NULL == filename)
    return NULL;

  // filename is a absolute path such as 
  // /opt/emageDBLocation/webImage/emage/dbImage/segment1/5725/5725_opt.wlz
  char url[MAX_STR_LEN];
  sprintf(url, "http://caperdonich.hgu.mrc.ac.uk%s", filename);

  WlzObject* ret = WlzRemoteImage::wlzHttpRead(url);

  if (NULL == ret)
    return ret;

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
    printf("cannot create/open file %s\n", filename);
    return ret;
  }
  
  // ret cannot be written as memory mapped format
  // it has already written as normal wlz format
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
  
  domObj = WlzMakeMain(ret->type, ret->domain, ret->values, NULL, NULL, &errNum);
  if(errNum == WLZ_ERR_NONE && NULL != domObj)
    outObj = WlzMakeTiledValuesFromObj(domObj, tlSz, copy, gType, bgdV, &errNum);
  
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

  return ret;
}
