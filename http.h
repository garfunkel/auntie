#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HTTP_BUFFER_SIZE (const int)sysconf(_SC_PAGESIZE)

typedef struct HttpRequest
{
	char *method;
	char *protocol;
	char *uri;
	char *host;
} HttpRequest;

typedef struct Uri
{
	char *protocol;
	char *host;
	char *path;
} Uri;

typedef struct HttpHeader
{
	char *protocol;
	unsigned short status;
	char *statusMsg;
	char *server;
	char *lastModified;
	char *eTag;
	char *contentType;
	char *transferEncoding;
	char *expires;
	char *cacheControl;
	char *pragma;
	char *date;
	unsigned int contentLength;
	char *connection;
	char *setCookie;
} HttpHeader;

extern const char HTTP_PROTOCOL_1_1[];
extern const char HTTP_METHOD_GET[];

HttpRequest *http_request_new();
void http_request_free(HttpRequest *request);

Uri *uri_new();
Uri *uri_parse(const char *uriStr);
void uri_free(Uri *uri);

#endif // HTTP_H
