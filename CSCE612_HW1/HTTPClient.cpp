#include "pch.h"
#include "HTTPClient.h"
#include "Socket.h"
#include <stdio.h>
#include <time.h>
#include "HTMLParserBase.h"

int getElapsedMs(clock_t start) {
    return (int)((clock() - start) * 1000.0 / CLOCKS_PER_SEC);
}

double getElapsedTimeHighRes(LARGE_INTEGER start, LARGE_INTEGER freq) {
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    return 1000.0 * (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
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

int downloadResource(const char* host, const char* path, const char* method,
    struct in_addr ipAddr, int port,
    int maxSize, bool showAsterisk, char** outBuffer, int* outSize) {

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("  Socket creation failed with %d\n", WSAGetLastError());
        return DOWNLOAD_FAILED;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr = ipAddr;
    server.sin_port = htons(port);

    bool isRobots = (strstr(path, "robots") != NULL);
    if (showAsterisk) printf("  * ");
    printf("Connecting on %s... ", isRobots ? "robots" : "page");

    LARGE_INTEGER startConn;
    QueryPerformanceCounter(&startConn);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("failed with %d\n", WSAGetLastError());
        closesocket(sock);
        return DOWNLOAD_FAILED;
    }

    printf("done in %.0f ms\n", getElapsedTimeHighRes(startConn, freq));

    char request[2048];
    sprintf(request,
        "%s %s HTTP/1.0\r\n"
        "User-agent: CSCE612KipkoechWebCrawler/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        method, path, host);

    
    if (send(sock, request, static_cast<int>(strlen(request)), 0) == SOCKET_ERROR) {
        printf("  Send failed with %d\n", WSAGetLastError());
        closesocket(sock);
        return DOWNLOAD_FAILED;
    }

    char* buffer = new char[maxSize + 1];
    int curPos = 0;

    if (showAsterisk) printf("  * ");
    printf("Loading... ");

    LARGE_INTEGER startLoad;
    QueryPerformanceCounter(&startLoad);

    while (true) {
        double elapsed = getElapsedTimeHighRes(startLoad, freq);

        if (elapsed > 10000) {  
            printf("failed with slow download\n");
            delete[] buffer;
            closesocket(sock);
            return DOWNLOAD_FAILED;
        }

        int remainingMs = (int)(10000 - elapsed);
        struct timeval timeout;
        timeout.tv_sec = remainingMs / 1000;
        timeout.tv_usec = (remainingMs % 1000) * 1000;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int ret = select(0, &readfds, NULL, NULL, &timeout);
        if (ret == 0) {
            printf("failed with slow download\n");
            delete[] buffer;
            closesocket(sock);
            return DOWNLOAD_FAILED;
        }
        if (ret == SOCKET_ERROR) {
            printf("failed with %d on select\n", WSAGetLastError());
            delete[] buffer;
            closesocket(sock);
            return DOWNLOAD_FAILED;
        }

        int bytes = recv(sock, buffer + curPos, maxSize - curPos, 0);

        if (bytes == 0) {
            break;
        }
        if (bytes == SOCKET_ERROR) {
            printf("failed with %d on recv\n", WSAGetLastError());
            delete[] buffer;
            closesocket(sock);
            return DOWNLOAD_FAILED;
        }

        curPos += bytes;

        if (curPos >= maxSize) {
            printf("failed with exceeding max size\n");
            delete[] buffer;
            closesocket(sock);
            return DOWNLOAD_FAILED;
        }
    }

    buffer[curPos] = '\0';

    printf("done in %.0f ms with %d bytes\n", getElapsedTimeHighRes(startLoad, freq), curPos);

    closesocket(sock);

    if (showAsterisk) printf("  * ");
    printf("Verifying header... ");

    char* statusLine = strstr(buffer, "HTTP/");
    if (!statusLine) {
        printf("failed (no status line)\n");
        delete[] buffer;
        return DOWNLOAD_FAILED;
    }

    int statusCode;
    sscanf(statusLine, "HTTP/%*s %d", &statusCode);
    printf("status code %d\n", statusCode);

    if (isRobots) {
        if (statusCode >= 400 && statusCode < 500) {
            delete[] buffer;
            return DOWNLOAD_SUCCESS;
        }
        else {
            delete[] buffer;
            return DOWNLOAD_BLOCKED;
        }
    }
    else {
        if (statusCode >= 200 && statusCode < 300) {
            *outBuffer = buffer;
            *outSize = curPos;
            return DOWNLOAD_SUCCESS;
        }
        else {
            delete[] buffer;
            return DOWNLOAD_FAILED;
        }
    }
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

    if (!socket.Send(request, static_cast<int>(strlen(request)))) {
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

bool processHTTPRequestPart2(const char* urlString,
    std::unordered_set<std::string>& seenHosts,
    std::unordered_set<std::string>& seenIPs) {
    printf("\nURL: %s\n", urlString);

    URL url;
    printf("  Parsing URL... ");
    if (!parseURL(urlString, &url)) {
        printf("failed\n");
        return false;
    }
    printf("host %s, port %d\n", url.host, url.port);

    if (strcmp(url.protocol, "https") == 0 || url.port == 443) {
        printf("  Error: HTTPS not supported\n");
        return false;
    }

    printf("  Checking host uniqueness... ");
    std::string hostStr(url.host);
    if (seenHosts.find(hostStr) != seenHosts.end()) {
        printf("failed\n");
        return false;
    }
    printf("passed\n");
    seenHosts.insert(hostStr);

    printf("  Doing DNS... ");
    LARGE_INTEGER startDNS, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&startDNS);

    struct hostent* remote = gethostbyname(url.host);

    if (remote == NULL) {
        printf("failed with %d\n", WSAGetLastError());
        return false;
    }

    struct in_addr addr;
    memcpy(&addr, remote->h_addr, 4);
    char* ipStr = inet_ntoa(addr);

    printf("done in %.0f ms, found %s\n", getElapsedTimeHighRes(startDNS, freq), ipStr);

    printf("  Checking IP uniqueness... ");
    std::string ipString(ipStr);
    if (seenIPs.find(ipString) != seenIPs.end()) {
        printf("failed\n");
        return false;
    }
    printf("passed\n");
    seenIPs.insert(ipString);

    char* robotsBuffer = NULL;
    int robotsSize = 0;
    int robotsResult = downloadResource(url.host, "/robots.txt", "HEAD",
        addr, url.port, 16384, true,
        &robotsBuffer, &robotsSize);
    if (robotsBuffer) delete[] robotsBuffer;

    if (robotsResult == DOWNLOAD_BLOCKED) {
        return false;
    }
    if (robotsResult == DOWNLOAD_FAILED) {
        return false;
    }

    char* pageBuffer = NULL;
    int pageSize = 0;
    int pageResult = downloadResource(url.host, url.path, "GET",
        addr, url.port, 2 * 1024 * 1024, true,
        &pageBuffer, &pageSize);

    if (pageResult == DOWNLOAD_SUCCESS && pageBuffer) {
        
        char* headerEnd = strstr(pageBuffer, "\r\n\r\n");
        if (headerEnd) {
            char* htmlBody = headerEnd + 4;
            int bodySize = pageSize - (htmlBody - pageBuffer);

            printf("  + Parsing page... ");
            clock_t parseStart = clock();

            char baseUrl[512];
            sprintf(baseUrl, "http://%s:%d%s", url.host, url.port, url.path);
            int nLinks = parseHTML(htmlBody, bodySize, baseUrl);

            printf("done in %d ms with %d links\n", getElapsedMs(parseStart), nLinks);
        }
        delete[] pageBuffer;
    }

    return (pageResult == DOWNLOAD_SUCCESS);
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