#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdbool.h>

typedef struct {
	char protocol[16];
	char host[256];
	int port;
	char path[512];
	char query[512];
	bool valid;
} URL;

bool parseURL(const char* urlTobeParsed, URL* url);
void printURL(const URL* url);

#endif
