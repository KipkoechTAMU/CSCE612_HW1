#include "pch.h"
#include "CrawlerThread.h"
#include "HTMLParserBase.h"
#include "HTTPClient.h"
#include "URLParser.h"
#include <stdio.h>

DWORD WINAPI CrawlerThread(LPVOID param) {
	ThreadParameters* p = (ThreadParameters*)param;
	CommonData* shared = p->shared;
	int threadID = p->threadId;

	//Create Parser once per thread
	HTMLParserBase* parser = new HTMLParserBase();

	while (true) { //while what is true
		std::string url;

		EnterCriticalSection(&shared->queueLock);

		if (shared->urlQueue.empty()) {
			LeaveCriticalSection(&shared->queueLock);
			break;
		}

		url = shared->urlQueue.front();
		shared->urlQueue.pop();
		
		LeaveCriticalSection(&shared->queueLock);

		printf("\n[%d] URL: %s\n", threadID, url.c_str());

		URL parsedUrl;

		//process the URL

		
}