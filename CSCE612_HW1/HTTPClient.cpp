#include "pch.h"
#include "HTTPClient.h"
#include "Socket.h"
#include <stdio.h>
#include <time.h>
#include "HTMLParserBase.h"

int getElapsedMs(clock_t start) {
    return (int)((clock() - start) * 1000.0 / CLOCKS_PER_SEC);
}

bool initWinsock(void) {
    WSADATA wsaData;
    WORD version = MAKEWORD(2, 2);

    int result = WSAStartup(version, &wsaData);
    if (result != 0) {
        printf("Winsock startup failed: %d\n", result);
        return false;
    }
    return true;
}

bool processHTTPRequest(const char* urlString) {
    clock_t dnsStart, connectStart, loadStart, parseStart;

    printf("URL: %s\n", urlString);

    URL url;
    printf("Parsing URL... ");
    if (!parseURL(urlString, &url)) {
        return false;
    }
    printf("host %s, port %d, request %s\n", url.host, url.port, url.path);

  
    if (strcmp(url.protocol, "https") == 0 || url.port == 443) {
        printf("Error: HTTPS not supported\n");
        return false;
    }

    if (!initWinsock()) {
        return false;
    }


    Socket socket;

  
    dnsStart = clock();
    printf("\tDoing DNS... ");
    connectStart = clock();
    printf("* Connecting on page... ");

    if (!socket.Connect(url.host, url.port)) {
        WSACleanup();
        return false;
    }
    printf("done in %d ms\n", getElapsedMs(connectStart));

    char request[2048];
    sprintf(request,
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: CSCE612KipkoechWebCrawler/1.0\r\n"
        "Connection: close\r\n"
        "\r\n",
        url.path, url.host);

    loadStart = clock();
    printf("Loading... ");

    if (!socket.Send(request, strlen(request))) {
        WSACleanup();
        return false;
    }

    if (!socket.Read()) {
        WSACleanup();
        return false;
    }

    int totalBytes = socket.GetCurPos();
    printf("done in %d ms with %d bytes\n", getElapsedMs(loadStart), totalBytes);

   
    char* buffer = socket.GetBuffer();

    printf("Verifying header... ");
    int statusCode = 0;
    if (sscanf(buffer, "HTTP/%*s %d", &statusCode) == 1) {
        printf("status code %d\n", statusCode);
    }

    printf("----------------------------------------\n");

    char* headerEnd = strstr(buffer, "\r\n\r\n");
    if (headerEnd != NULL) {
        int headerSize = headerEnd - buffer;
        char* headers = new char[headerSize + 1];
        memcpy(headers, buffer, headerSize);
        headers[headerSize] = '\0';
        printf("%s\n", headers);
        delete[] headers;

        int bodySize = totalBytes - headerSize - 4;

        if (statusCode == 200) {
            parseStart = clock();
            printf("+ Parsing page... ");

            char* htmlBody = headerEnd + 4;
            char baseUrl[512];
            sprintf(baseUrl, "http://%s:%d%s", url.host, url.port, url.path);

            int nLinks = parseHTML(htmlBody, bodySize, baseUrl);
            printf("done in %d ms with %d links\n", getElapsedMs(parseStart), nLinks);
        }
    }

    WSACleanup();
    return true;
}

int parseHTML(const char* htmlContent, int htmlSize, const char* baseUrl) {
    HTMLParserBase* parser = new HTMLParserBase;
    int nLinks = 0;

    char* linkBuffer = parser->Parse(
        (char*)htmlContent,
        htmlSize,
        (char*)baseUrl,
        strlen(baseUrl),
        &nLinks
    );

    if (nLinks < 0) {
        nLinks = 0;
    }

    delete parser;
    return nLinks;
}