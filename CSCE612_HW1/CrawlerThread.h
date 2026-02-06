#pragma once
#include <windows.h>
#include <queue>
#include <unordered_set>
#include <string>

struct CommonData {
   std::queue<std::string> urlQueue;
   std::unordered_set<std::string> seenHosts;
   std::unordered_set<std::string> seenIPs;

   volatile long totalDNS;
   volatile long totalExtracted;
   volatile long totalPages;
   volatile long totalBytes;

   CRITICAL_SECTION queueLock;
   HANDLE eventQuit;

   CommonData() : totalExtracted(0), totalDNS(0),
       totalPages(0), totalBytes(0) {
       InitializeCriticalSection(&queueLock);
       eventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
   }

   ~CommonData() {
       DeleteCriticalSection(&queueLock);
       CloseHandle(eventQuit);
   }
};

struct ThreadParameters {
    int threadId;
    CommonData* shared;
};

DWORD WINAPI WorkerThread(LPVOID param);
DWORD WINAPI StatsThread(LPVOID param);
