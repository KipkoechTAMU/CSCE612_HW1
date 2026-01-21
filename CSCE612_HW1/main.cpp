
#include "pch.h"
#include <stdio.h>
#include "HTTPClient.h"
#include "URLParser.h"

int main(int argc, char* argv[])
{
    
    if (argc != 2) {
        printf("Need 2 arguments for usage: %s<URL>\n", argv[0]);
        return 1;
    }
    
    processHTTPRequest(argv[1]);
    return 0;
}
