
#include "pch.h"
#include <stdio.h>
#include <vector>
#include <unordered_set>
#include <string>
#include "HTTPClient.h"
#include "URLParser.h"

using namespace std;


int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s numThreads inputFile\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[1]);
    if (numThreads != 1) {
        printf("Error: number of threads must be 1 for this app\n");
        return 1;
    }

    char* filename = argv[2];

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error: cannot open file %s\n", filename);
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    printf("Opened %s with size %d\n", filename, fileSize);

    char* fileBuf = new char[fileSize + 1];
    DWORD bytesRead;

    if (!ReadFile(hFile, fileBuf, fileSize, &bytesRead, NULL)) {
        printf("Error: failed to read file\n");
        CloseHandle(hFile);
        delete[] fileBuf;
        return 1;
    }

    fileBuf[fileSize] = '\0';
    CloseHandle(hFile);

    vector<string> urls;
    char* lineStart = fileBuf;

    for (DWORD i = 0; i <= fileSize; i++) {
        if (i == fileSize || fileBuf[i] == '\n' || fileBuf[i] == '\r') {
            if (lineStart < &fileBuf[i]) {
                string url(lineStart, &fileBuf[i]);
                if (!url.empty()) {
                    urls.push_back(url);
                }
            }
            if (fileBuf[i] == '\r' && i + 1 < fileSize && fileBuf[i + 1] == '\n') {
                i++;
            }
            lineStart = &fileBuf[i + 1];
        }
    }

    delete[] fileBuf;

    if (!initWinsock()) {
        return 1;
    }

    unordered_set<string> seenHosts;
    unordered_set<string> seenIPs;

    for (size_t i = 0; i < urls.size(); i++) {
        processHTTPRequestPart2(urls[i].c_str(), seenHosts, seenIPs);
    }

    WSACleanup();
    return 0;
}
