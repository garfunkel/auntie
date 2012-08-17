#ifndef HTTP_H
#define HTTP_H

#include <unistd.h>

#define HTTP_BUFFER_SIZE (const int)sysconf(_SC_PAGESIZE)

struct HttpRequest
{
	char *method;
	char *protocol;
	char *uri;
	char *host;
};

struct Uri
{
	char *protocol;
	char *host;
	char *path;
};

struct HttpHeader
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
};

#endif // HTTP_H
