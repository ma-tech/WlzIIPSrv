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

int TIMEOUT_SERVER = 10000;

int
WlzRemoteImage::init_header(int type, int requestId, size_t contentLength,
            size_t paddingLength, FCGI_Header * header)
{
  if (contentLength > 65535 || paddingLength > 255)
    return 0;
  header->version = FCGI_VERSION_1;
  header->type = (unsigned char) type;
  header->requestIdB1 = (unsigned char) ((requestId >> 8) & 0xff);
  header->requestIdB0 = (unsigned char) (requestId & 0xff);
  header->contentLengthB1 =
    (unsigned char) ((contentLength >> 8) & 0xff);
  header->contentLengthB0 = (unsigned char) ((contentLength) & 0xff);
  header->paddingLength = (unsigned char) paddingLength;
  header->reserved = 0;
  return 1;
}

void
WlzRemoteImage::init_begin_request_body(int role,
                        FCGI_BeginRequestBody * begin_request_body)
{
  begin_request_body->roleB1 = (unsigned char) (((role >> 8) & 0xff));
  begin_request_body->roleB0 = (unsigned char) (role & 0xff);
  begin_request_body->flags = 0;
  memset(begin_request_body->reserved, 0,
	 sizeof(begin_request_body->reserved));
}

size_t
WlzRemoteImage::init_param_body(char* name,  
		unsigned long namelen, 
		char* value, 
		unsigned long valuelen,
		char* buf)
{
  char *cur_buf = buf;
  size_t buffer_size = 0;
  
  /* Put name length to buffer */
  if (namelen < 0x80) {
    if (!buf)
      buffer_size++;
    else
      *cur_buf++ = (unsigned char) namelen;
  } else {
    if (!buf)
      buffer_size += 4;
    else {
      *cur_buf++ = (unsigned char) ((namelen >> 24) | 0x80);
      *cur_buf++ = (unsigned char) (namelen >> 16);
      *cur_buf++ = (unsigned char) (namelen >> 8);
      *cur_buf++ = (unsigned char) namelen;
    }
  }
  
  /* Put value length to buffer */
  if (valuelen < 0x80) {
    if (!buf)
      buffer_size++;
    else
      *cur_buf++ = (unsigned char) valuelen;
  } else {
    if (!buf)
      buffer_size += 4;
    else {
      *cur_buf++ = (unsigned char) ((valuelen >> 24) | 0x80);
      *cur_buf++ = (unsigned char) (valuelen >> 16);
      *cur_buf++ = (unsigned char) (valuelen >> 8);
      *cur_buf++ = (unsigned char) valuelen;
    }
  }
  
  /* Now the name and body buffer */
  if (!buf) {
    buffer_size += namelen;
    buffer_size += valuelen;
  } else {
    memcpy(cur_buf, name, namelen);
    cur_buf += namelen;
    memcpy(cur_buf, value, valuelen);
    cur_buf += valuelen;
  }
  
  return buffer_size;
}

FCGIMessageStack*  
WlzRemoteImage::buildConnectionInfo(const char* filename)
{
  FCGIMessageStack* stack = NULL;
  FCGIMessageStack* lastOnStack = NULL;
  int ok = 1;
  size_t message_size = sizeof(FCGIMessageStack);
  size_t header_size = sizeof(FCGI_Header);
  size_t request_body_size = sizeof(FCGI_BeginRequestBody);
  size_t param_body_size = 0;
  FCGI_Header *request_header = NULL;
  FCGI_BeginRequestBody *request_body = NULL;
  
  /* to build fcgi message
     {FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
     {FCGI_PARAMS, 1, "QUERY_STRING=HOST=??&PORT=??&WLZ=??"}
     {FCGI_PARAMS, 1, ""}
     {FCGI_STDIN, 1, ""}
  */
  
  /* Alloc memory for  {FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
   */
  request_header =(FCGI_Header*)malloc(header_size);
  if (request_header == NULL) {
    ok = 0;
    printf("Can not allocate memory for request_header!\n");
  }
  
  if (ok) {
    request_body = (FCGI_BeginRequestBody*)malloc(request_body_size);
    if (request_body == NULL) {
      ok = 0;
      printf("Can not allocate memory for request_body!\n");
    }
  }
  
  /* Initialize begin request header */
  if (ok &&
      !init_header(FCGI_BEGIN_REQUEST, 1, request_body_size, 0, request_header)) {
    ok = 0;
    printf("error in init_header!\n");
  }
  
  /* Initialize begin request body */
  if (ok) {
    init_begin_request_body(FCGI_RESPONDER, request_body);
    
    /* request message   {FCGI_BEGIN_REQUEST, 1, {FCGI_RESPONDER, 0}}
     */
    stack = (FCGIMessageStack*)malloc(message_size);
    if (stack == NULL) {
      ok = 0;
      printf("Can not allocate memory for stack!\n");
    }
  }
  
  if (ok) {
    lastOnStack = stack;
    
    lastOnStack ->sizeBuffer = request_body_size  +  header_size;
    lastOnStack ->buffer = (char*) malloc(lastOnStack ->sizeBuffer);
    
    if (lastOnStack ->buffer == NULL) {
      ok = 0;
      printf("Can not allocate memory for buffer!\n");
    }
  }
  
  if (ok) {
    memcpy(lastOnStack ->buffer, request_header, header_size);
    memcpy(lastOnStack ->buffer + header_size, request_body, request_body_size);
  }
  
  /* Alloc memory for  {FCGI_PARAMS, 1, "QUERY_STRING..."}
   */
  char name[127];
  unsigned long name_length = 0;
  char value[127];
  unsigned long value_length = 0;
  
  sprintf(name, "QUERY_STRING");
  sprintf(value, "WLZ=%s", filename);
  name_length = strlen(name);
  value_length=strlen(value);
  
  if (ok) {
    param_body_size = init_param_body(name,  name_length, value, value_length, NULL);
    if (NULL != request_body)
      free(request_body);
    request_body = (FCGI_BeginRequestBody*)malloc(param_body_size);
    if (NULL ==  request_body) {
      ok = 0;
      printf("Can not allocate memory for request body!\n");
    }
  }
  
  /* Initialize param request header */
  if (ok &&
      !init_header(FCGI_PARAMS, 1, param_body_size, 0, request_header)) {
    ok = 0;
    printf("error in init param header!\n");
  }
  
  if (ok) {
    init_param_body((char*)name,  (unsigned long)name_length, (char*)value, (unsigned long)value_length, (char*)request_body);
    
    /* request message     {FCGI_PARAMS, 1, "QUERY_STRING..."}
     */
    lastOnStack->next = (FCGIMessageStack*)malloc(message_size);
    lastOnStack =  lastOnStack->next;
    
    if (NULL != lastOnStack)
      {
	lastOnStack ->sizeBuffer = param_body_size  +  header_size;
	lastOnStack ->buffer = (char*) malloc(lastOnStack ->sizeBuffer);
	
	if (lastOnStack ->buffer == NULL) {
	  ok = 0;
	  printf("Can not allocate memory for buffer!\n");
	}
      }
  }
  
  if (ok) {
    memcpy(lastOnStack ->buffer, request_header, header_size);
    memcpy(lastOnStack ->buffer + header_size, request_body, param_body_size);
    
    /* Alloc memory for  {FCGI_PARAMS, 1, ""}
     */
    
    /* Initialize param request header */
    if (!init_header(FCGI_PARAMS, 1, 0, 0, request_header)) {
      ok = 0;
      printf("error in init param header!\n");
    }
  }
  
  /* request message     {FCGI_PARAMS, 1, ""}
   */
  if (ok)
    {
      lastOnStack->next = (FCGIMessageStack*)malloc(message_size);
      lastOnStack =  lastOnStack->next;
      
      if (NULL == lastOnStack)
	ok = 0;
    }
  
  if (ok) {
    lastOnStack ->sizeBuffer = header_size;
    lastOnStack ->buffer = (char*) malloc(lastOnStack ->sizeBuffer);
    
    if (lastOnStack ->buffer == NULL) {
      ok = 0;
      printf("Can not allocate memory for buffer!\n");
    }
  }
  
  if (ok) {
    memcpy(lastOnStack ->buffer, request_header, header_size);
    
    /* Alloc memory for  {FCGI_STDIN, 1, ""}
     */
    
    /* Initialize param request header */
    if (!init_header(FCGI_STDIN, 1, 0, 0, request_header)) {
      ok = 0;
    }
  }
  
  /* request message     {FCGI_STDIN, 1, ""}
   */
  if (ok)
    {
      lastOnStack->next = (FCGIMessageStack*)malloc(message_size);
      lastOnStack =  lastOnStack->next;
      
      if (NULL == lastOnStack)
	ok = 0;
    }
  if (ok)
    {
      lastOnStack ->sizeBuffer = header_size;
      lastOnStack ->buffer = (char*) malloc(lastOnStack ->sizeBuffer);
      
      if (lastOnStack ->buffer == NULL) {
	ok = 0;
	printf("Can not allocate memory for buffer!\n");
      }
    }
  
  if (ok)
    {
      memcpy(lastOnStack ->buffer, request_header, header_size);
      lastOnStack->next = NULL;
    }
  
  // free memory
  if (NULL != request_header)
    {
      free(request_header);
      request_header = NULL;
    }
  if (NULL != request_body)
    {
      free(request_body);
      request_body = NULL;
    }
  if (!ok)
    {
      while (NULL != stack)
	{
	  lastOnStack = stack->next;
	  if (NULL != stack ->buffer)
	    {
	      free(stack ->buffer);
	      stack ->buffer = NULL;
	    }
	  free(stack);
	  stack = lastOnStack;
	}
      stack = NULL;
    }
  
  return stack;
}

/** send request and return sock descriptor for getting response
 */
int 
WlzRemoteImage::sendConnectionInfo(FCGIMessageStack* message, const char* server, int port)
{
  int cnt = -1;
  int sock = -1;
  
  struct sockaddr_in myname;  /* Internet socket name (addr) */
  
  /* For look up in /etc/hosts file.   */
  struct hostent *hp;
  
  /* As in UNIX domain, create a client socket to request
     service          */
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("WlzIIPProxy client setup \n");
    return -1;
  }
  
  // set TCP_NODELAY
  int flag = 1;
  int result = setsockopt(sock,            /* socket affected */
                          IPPROTO_TCP,     /* set option at TCP level */
                          TCP_NODELAY,     /* name of option */
                          (char *) &flag,  /* the cast is historical 
					      cruft */
                          sizeof(int));    /* length of option value */
  if (result < 0) {
    perror("WlzRemoteSrv TCP_NODELAY failure \n");
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return -1;
  }
  
  myname.sin_port = htons(port);
  myname.sin_family = AF_INET;    /* Internet domain */
  hp = gethostbyname(server);
  if (hp == NULL) {
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return -1;
  }
  
  memcpy((void*) &myname.sin_addr.s_addr, (void*) hp -> h_addr_list[0], hp -> h_length);
  
  
  /* Establish socket connection with (remote) server.  */
  if ((connect(sock, (struct sockaddr *) &myname, sizeof(myname))) < 0) {
    shutdown(sock, SHUT_RDWR);
    sock = -1;
    return -1;
  }
  
  // send message
  while (message) {
    write(sock, message->buffer, message->sizeBuffer);
    message = message->next;
  }
  
  // no more send
  shutdown(sock, SHUT_WR);
  
  return sock;
}


WlzObject*  
WlzRemoteImage::wlzRemoteReadObj(const char* filename, const char* serverInput, int portInput)
{
  char* wlzServerName = (char*)serverInput;
  int port = portInput;
  if (NULL == wlzServerName) {
    wlzServerName = new char[256];
    strcpy(wlzServerName, "bunnahabhain");
  }

  if (0 >= port)
    port = 80000;

  FCGIMessageStack* message = buildConnectionInfo(filename);
  int cnt = -1;
  int sock = -1;
  
  if (NULL != message)
    sock = sendConnectionInfo(message, (const char*)wlzServerName, port);
  
  if (0 >= sock) {
    if (NULL == serverInput) {
      delete[] wlzServerName;
      wlzServerName = NULL;
    }
    return NULL;
  }
  
  // receive response
  char *messageBody = NULL;
  int retries = TIMEOUT_SERVER;
  int i = 0;
  int endConnection = 1;
  FCGI_Header header ;
  size_t messageSize = 0;
  size_t readSize = 0;
  char* reply = NULL;
  WlzObject* ret = NULL;
  WlzErrorNum  errNum = WLZ_ERR_NONE;
  
  while ((cnt = recv(sock, &header, sizeof(header), MSG_WAITALL)) > 0 && endConnection) {
    messageSize = (((unsigned int) header.contentLengthB1) << 8) + header.contentLengthB0 + header.paddingLength;
    
    if (messageSize > 0) {
      messageBody = (char*)malloc(messageSize+1);
      // initialize
      strcpy(messageBody, "");
      readSize = 0;
      while (readSize<messageSize  && retries-->0)
	{
	  readSize = recv(sock, messageBody, messageSize, MSG_PEEK | MSG_WAITALL);
	}
      
      if (readSize == messageSize)
	readSize = recv(sock, messageBody, messageSize, MSG_DONTWAIT);
      
      if (messageSize != readSize) {
	printf(" Message read: wrong packet size (read: %d; expected: %d)\n", readSize, messageSize);
	close(sock);
	sock = -1;
      }
      // ending the message
      messageBody[messageSize] = 0;
    } else messageBody = NULL;
    
    if (0 >= sock)
      break;
    
    switch (header.type) {
    case FCGI_END_REQUEST:
      ///??? I have not looked into details. At the moment, you have to 
      ///??? read wlz object here. Otherwide it will not work
      if (NULL != reply) {
	if (0 == strcmp(reply, "ok") ||
	    0 == strcmp(reply, "Ok") ||
	    0 == strcmp(reply, "OK")) {
	  
	  FILE *file = fdopen(sock, "r");
	  if (NULL == file)
	    printf("fdopen failed %d\n", sock);
	  else {
	    ret = WlzReadObj(file, &errNum);
	    if (NULL == ret)
	      perror("fail to read wlz object\n");
	    else {
	      if (WLZ_ERR_NONE != errNum) {
		printf("error in reading wlz object\n");
		WlzFreeObj(ret);
		ret = NULL;
	      }
	    }
	    
	    fclose(file);
	    file = NULL;
	  }
	}
      }
      endConnection = 0;
      break;
      // fall-through
    case FCGI_BEGIN_REQUEST:
    case FCGI_PARAMS:
    case FCGI_STDIN:
    case FCGI_STDOUT:
      if (NULL == messageBody ||
	  0 == strcmp(messageBody, "")) {
	if (NULL != messageBody) {
	  free(messageBody);
	  messageBody = NULL;
	}
      } else {
	reply = messageBody;
	messageBody = NULL;
      }
      break;
    case FCGI_DATA:
    case FCGI_ABORT_REQUEST:
      break;
    default:
      break;
    }
    
    if (NULL != messageBody) {
      if (NULL == reply)
	perror("no reply but have message\n");
      
      free(messageBody);
      messageBody = NULL;
    }
  }
  
  // disconnect for all processes
  shutdown(sock, SHUT_RDWR);
  sock = -1;
  
  // free memory

  if (NULL != messageBody) {
    free(messageBody);
    messageBody = NULL;
  }
  if (NULL != reply) {
    free(reply);
    reply = NULL;
  }
  
  FCGIMessageStack* stack = NULL;
  while (NULL != message)
    {
      stack = message->next;
      if (NULL != message->buffer)
	{
	  free(message->buffer);
	  message->buffer = NULL;
	}
      free(message);
      message = stack;
    }

  if (NULL == serverInput) {
    delete[] wlzServerName;
    wlzServerName = NULL;
  }
  
  FILE  *fP = NULL;
  if (NULL == ret)
    perror("cannot get hold wlz obj \n");
  else {
    // make sure the directory exists
    char str[256];
    char dir[256];
    char worker[100];
    
    char separate[2];
    struct stat st;
    
    strcpy(dir, "");
    
    strcpy(str, filename);
    strcpy(separate, "/");
    
    if (str[0] == '/')
      strcpy(dir, "/");
    int i = 0;
    char* token = NULL;
    token = strtok(str, separate);
    while (NULL != token) {
      if (0 < i) {
	sprintf(worker, "/%s", token);
	strcat(dir, worker);
      } else
	strcat(dir, token);
      i++;
      token = strtok(NULL, separate);
      if (NULL != token) {
	if(-1 == lstat(dir,&st)) {
	  if(mkdir(dir,0766)==-1) {
	    printf("error in creating dir %s\n", dir);
	    i = -1;
	    break;
	  }
	} else {
	  if ((st.st_mode & S_IWUSR && st.st_mode & S_IRUSR) ||
	      (st.st_mode & S_IWGRP && st.st_mode & S_IRGRP))
	    ;
	  else {
	    printf("unableable read/write dir %s\n", dir);
	    break;
	  }
	}
      }
    }
    
    // create directory if needed
    if (-1 != i) {
      fP = fopen(filename, "w");
      
      // save to the local machine for non-3D-value wlz object
      if (NULL == fP) 
	printf("cannot create/open file %s\n", filename);
      else {
	if (NULL == ret->domain.core ||
	    WLZ_3D_DOMAINOBJ != ret->type) {
	  WlzWriteObj(fP, ret);
	  fflush(fP);
	  fclose(fP);
	  fP = NULL;
	}
      }
    }
  }

  WlzGreyType	gType = WlzGreyTypeFromObj(ret, &errNum);
  if (WLZ_GREY_ERROR == gType) {
    if (NULL != fP) {
      fclose(fP);
      fP = NULL;
    }
  }

  // only 3D value wlz object, save  to the local machine as tiled memory format for fast access 
  if (NULL != fP) {

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
    }
  }

  return ret;
  }
