#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#pragma once
#include <unordered_set>
#include <string>

#include <winsock2.h>
#include <stdbool.h>
#include "URLParser.h"


#pragma comment(lib, "ws2_32.lib")

#define DOWNLOAD_SUCCESS 0
#define DOWNLOAD_FAILED 1
#define DOWNLOAD_BLOCKED 2

bool processHTTPRequest(const char* urlString);

bool initWinsock(void);

int parseHTML(const char* htmlContent, int htmlSize, const char* baseUrl);

bool processHTTPRequestPart2(const char* urlString,
    std::unordered_set<std::string>& seenHosts,
    std::unordered_set<std::string>& seenIPs);
int downloadResource(const char* host, const char* path, const char* method,
    struct in_addr ipAddr, int port,
    int maxSize, bool showAsterisk, char** outBuffer, int* outSize);
double getElapsedTimeHighRes(LARGE_INTEGER start, LARGE_INTEGER freq);


#endif

