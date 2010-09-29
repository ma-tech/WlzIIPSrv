#ifndef WLZ_REMOTE_IMAGE_H
#define WLZ_REMOTE_IMAGE_H

#include <fastcgi.h>
#include "Wlz.h"


/*!
 * \struct _FCGImessageStack
 * \ingroup WlzIIPProxy
 * \brief Singly-linked list for list of stored FCGI packages
 */
typedef struct _FCGIMessageStack {
  /// message buffer
  char *buffer;
  
  /// message buffer size
  size_t sizeBuffer;
  
  ///next pointer
  struct _FCGIMessageStack * next;
} FCGIMessageStack;

class WlzRemoteImage {
/// buffer size for reading and parsing files

 protected:

static int
init_header(int type, int requestId, size_t contentLength,
            size_t paddingLength, FCGI_Header * header);

static void
init_begin_request_body(int role,
                        FCGI_BeginRequestBody * begin_request_body);
static size_t
init_param_body(char* name,  
		unsigned long namelen, 
		char* value, 
		unsigned long valuelen,
		char* buf);
static FCGIMessageStack*  
  buildConnectionInfo(const char* filename);

static int 
  sendConnectionInfo(FCGIMessageStack* message, const char* server, int port);

static int
  parse_URL(const char *url, char *hostname, int *port, char *identifier);

public:

static WlzObject*  
  wlzRemoteReadObj(const char* filename, const char* server, int port);

static WlzObject*  
  wlzFcgiRead(const char* filename, const char* server, int port);

static WlzObject*  
  wlzHttpRead(const char* url);
};

#endif
