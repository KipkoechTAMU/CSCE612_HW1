#include "pch.h"
#include "Socket.h"
#include <stdio.h>

//from hw guideline
Socket::Socket() {
    sock = INVALID_SOCKET;
    buf = new char[INITIAL_BUF_SIZE];
    allocatedSize = INITIAL_BUF_SIZE;
    curPos = 0;
}

Socket::~Socket() {
    if (buf != nullptr) {
        delete[] buf;
    }
    Close();
}

bool Socket::Connect(const char* host, int port) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("socket creation failed: %d\n", WSAGetLastError());
        return false;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // from sample code provided in class
    // Try IP address first
    DWORD IP = inet_addr(host);
    if (IP == INADDR_NONE) {
        // DNS lookup needed
        struct hostent* remote = gethostbyname(host);
        if (remote == NULL) {
            printf("DNS lookup failed: %d\n", WSAGetLastError());
            return false;
        }
        memcpy(&(server.sin_addr), remote->h_addr, remote->h_length);
    }
    else {
        server.sin_addr.S_un.S_addr = IP;
    }

    printf("found %s\n", inet_ntoa(server.sin_addr));

    // Connect
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("connect() failed: %d\n", WSAGetLastError());
        return false;
    }

    return true;
}

bool Socket::Send(const char* request, int requestSize) {
    int bytesSent = send(sock, request, requestSize, 0);
    if (bytesSent == SOCKET_ERROR) {
        printf("send() failed: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

bool Socket::Read() {
    // Setting  timeout to 10 seconds
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int ret = select(0, &readfds, NULL, NULL, &timeout);

        if (ret > 0) {
            // resize buffer to accomodate larger page
            if (allocatedSize - curPos < THRESHOLD) {

                // increase buffer size by 2
                int newSize = allocatedSize * 2;
                char* newBuf = new char[newSize];
                memcpy(newBuf, buf, curPos);
                delete[] buf;
                buf = newBuf;
                allocatedSize = newSize;
            }

            int bytesReceived = recv(sock, buf + curPos,
                allocatedSize - curPos, 0);

            if (bytesReceived == SOCKET_ERROR) {
                printf("recv() failed: %d\n", WSAGetLastError());
                return false;
            }

            if (bytesReceived == 0) {
                buf[curPos] = '\0'; 
                return true;
            }

            curPos += bytesReceived;
        }
        else if (ret == 0) {
            printf("failed with timeouts\n");
            return false;
        }
        else {
            printf("select() failed: %d\n", WSAGetLastError());
            return false;
        }
    }
}

void Socket::Close() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}