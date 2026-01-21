#include "pch.h"
#include <stdio.h>
#include "URLParser.h"
#include <string.h>

bool parseURL(const char* urlTobeParsed, URL* url) {
	
	strcpy(url->protocol, "http");
	url->host[0] = '\0';
	url->port = 80;
	strcpy(url->path, "/");
	url->valid = false;
	url->query[0] = '\0';


	char temp[2048];
	strcpy(temp, urlTobeParsed);
	char* ptr = temp;

	//remove fragment
	char* fragment = strchr(ptr, '#');
	if (fragment) {
		*fragment = '\0';
	}
	
	//extract  scheme
	char* colon = strchr(ptr, ':');
	if (colon && colon[1]=='/' && colon[2]=='/') {
		int len = colon - ptr;
		strncpy(url->protocol,ptr, len);
		url->protocol[len] = '\0';
		ptr = colon + 3;

		if (strcmp(url->protocol, "http") != 0) {
			printf("failed with invalid scheme\n");
			return false;
		}

	}

	//extract query
	char* question = strchr(ptr, '?');
	if (question) {
		strcpy(url->query,question);
		*question = '\0';

	}

	//extract path

	char* slash = strchr(ptr, '/');

	if (slash) {
		strcpy(url->path, slash);
		*slash = '\0';
	}

	//extract port

	char* secondColon = strchr(ptr, ':');

	if (secondColon) {
		url->port = atoi(secondColon + 1);

		if (url->port <= 0 || url->port > 65535 ||
			strlen(secondColon + 1) == 0 ||
			(secondColon[1] == '/' && strlen(secondColon + 1) == 1)) {
			printf("failed with invalid port\n");
			return false;
		}

		*secondColon = '\0';
		
	}

	//extract host
	strcpy(url->host, ptr);

	if (strlen(url->host) > 0) {
		url->valid = true;
	}
	
	return url->valid;

}


void printURL(const URL* url) {
	printf("Protocol: %s\n", url->protocol);
	printf("Host: %s\n", url->host);
	printf("Port: %d\n", url->port);
	printf("Path: %s\n", url->path);
	printf("Query: %s\n", url->query);

}