#pragma once
#include <winsock2.h>

#define INITIAL_BUF_SIZE 8192
#define THRESHOLD 1024

class Socket {
private:
    SOCKET sock;
    char* buf;
    int allocatedSize;
    int curPos;

public:
    Socket();
    ~Socket();

    bool Connect(const char* host, int port);
    bool Send(const char* request, int requestSize);
    bool Read();

    char* GetBuffer() { return buf; }
    int GetCurPos() { return curPos; }
    SOCKET GetSocket() { return sock; }

    void Close();
};
