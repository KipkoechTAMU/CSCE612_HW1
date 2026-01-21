#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <winsock2.h>
#include <stdbool.h>
#include "URLParser.h"


#pragma comment(lib, "ws2_32.lib")

bool processHTTPRequest(const char* urlString);

bool initWinsock(void);

int parseHTML(const char* htmlContent, int htmlSize, const char* baseUrl);


#endif

