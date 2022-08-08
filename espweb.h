#ifndef __ESPWEB_H
#define __ESPWEB_H

#include <WebServer.h>

#ifndef __IN_ESPWEB_CPP
extern WebServer server;
#endif // __IN_ESPWEB_CPP

void setup_web();
void loop_web();   // call repeatedly

void http_root();
void http_reset(); // reset timer


#endif // __ESPWEB_H

