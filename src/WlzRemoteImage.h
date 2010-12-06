#ifndef WLZ_REMOTE_IMAGE_H
#define WLZ_REMOTE_IMAGE_H

#include "Wlz.h"

class WlzRemoteImage {
/// buffer size for reading and parsing files

 protected:
  
  static int
    parse_URL(const char *url, char *hostname, int *port, char *identifier);
  
  static WlzObject*  
    wlzHttpRead(char const* url);
  
 public:
  
  static WlzObject*  
    wlzRemoteReadObj(const char* filename, const char* server, int port);
};

#endif
