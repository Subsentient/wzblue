/*
WZBlue is a standalone version of aqu4bot's wz.c Warzone 2100 lobby facilities and aqu4bot's networking core.
See http://github.com/Subsentient/aqu4bot for aqu4bot's source code.
Public domain.
See the included file UNLICENSE.TXT for more information.
*/

#include <stdio.h>
#include <curl/curl.h>
#include "substrings/substrings.h"
#include "wzblue.h"

struct HTTPStruct
{
	char *OutStream;
	unsigned MaxWriteSize;
	unsigned Written;
};

static size_t _HTTPWriteHandler(void *InStream, size_t SizePerUnit, size_t NumMembers, void *HTTPDesc_)
{	
	struct HTTPStruct *HTTPDesc = (struct HTTPStruct*)HTTPDesc_;
	size_t ChunkSize = SizePerUnit * NumMembers;
	size_t RemainingSpace = HTTPDesc->MaxWriteSize - HTTPDesc->Written;
	
	// Don't write more than we have space for
	if (ChunkSize > RemainingSpace) {
		ChunkSize = RemainingSpace;
	}
	
	// Copy raw data directly, not as a string
	memcpy(HTTPDesc->OutStream + HTTPDesc->Written, InStream, ChunkSize);
	HTTPDesc->Written += ChunkSize;
	
	return SizePerUnit * NumMembers;
}

bool Curl_GetLobbyHTTP(const char *const URL, void *const OutStream_, const unsigned MaxOutBytes)
{
	struct HTTPStruct HTTPDescriptor = { .OutStream = (char*)OutStream_, .MaxWriteSize = MaxOutBytes, .Written = 0 };
	int AttemptsRemaining = 3; /*Three tries by default.*/
	CURLcode Code;
	CURL *Curl = NULL;
	
	do
	{
		memset(HTTPDescriptor.OutStream, 0, MaxOutBytes); /*Clear it.*/
		
		Curl = curl_easy_init(); /*Fire up curl.*/
		
		/*Add the URL*/
		curl_easy_setopt(Curl, CURLOPT_URL, URL);
		
		curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L); /*My own cert is self-signed, and I want it to work.*/
		
		/*Follow paths that redirect.*/
		curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(Curl, CURLOPT_USERAGENT, "WZBlue v" WZBLUE_VERSION);
		
		/*No progress bar on stdout please.*/
		curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 1L);
		
		curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, _HTTPWriteHandler);
		curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &HTTPDescriptor);
		curl_easy_setopt(Curl, CURLOPT_VERBOSE, 0L); /*No verbose please.*/
		
		/*Request maximum 5 second timeout for each try of the operation.*/
		curl_easy_setopt(Curl, CURLOPT_CONNECTTIMEOUT, 5L); /*This will not save us in some cases.*/
		
		
		/*Request that we NOT download files larger than what we want.*/
		curl_easy_setopt(Curl, CURLOPT_MAXFILESIZE, MaxOutBytes);
		
		Code = curl_easy_perform(Curl);
		
		printf("Received complete JSON: BEGIN\n%s\nEND\n", HTTPDescriptor.OutStream);
		
		curl_easy_cleanup(Curl);
	} while (--AttemptsRemaining, (Code != CURLE_OK && AttemptsRemaining > 0));
	
	return Code == CURLE_OK;
		
}


