#include "http.h"
#include "utils.h"

const char HTTP_PROTOCOL_1_1[] = "HTTP/1.1";
const char HTTP_METHOD_GET[] = "GET";

HttpRequest *http_request_new()
{
	HttpRequest *request = malloc(sizeof(HttpRequest));

	memset(request, 0, sizeof(*request));

	return request;
}

char http_header_parse_field(HttpHeader *header, const char *field)
{
	unsigned int fieldLen = strlen(field);

	char *key = strtok((char *)field, ":");

	if (strlen(key) == fieldLen)
	{
		header->protocol = strtok(key, " ");
		header->status = atoi(strtok(NULL, " "));
		header->statusMsg = strtok(NULL, "\r");
	}

	else
	{
		char *value = strtok(NULL, "\r");

		if (!strcmp(key, "Server"))
			header->server = value;

		else if (!strcmp(key, "Last-Modified"))
			header->lastModified = value;

		else if (!strcmp(key, "ETag"))
			header->eTag = value;

		else if (!strcmp(key, "Content-Type"))
			header->contentType = value;

		else if (!strcmp(key, "Expires"))
			header->expires = value;

		else if (!strcmp(key, "Cache-Control"))
			header->cacheControl = value;

		else if (!strcmp(key, "Pragma"))
			header->pragma = value;

		else if (!strcmp(key, "Date"))
			header->date = value;

		else if (!strcmp(key, "Content-Length"))
			header->contentLength = atoi(value);

		else if (!strcmp(key, "Connection"))
			header->connection = value;

		else if (!strcmp(key, "Set-Cookie"))
			header->setCookie = value;
	}

	return 0;
}

void http_request_free(HttpRequest *request)
{
	if (request->method)
		free_null(request->method);
	
	if (request->protocol)
		free_null(request->protocol);
	
	if (request->uri)
		free_null(request->uri);
	
	if (request->host)
		free_null(request->host);

	free_null(request);
}

Uri *uri_new()
{
	Uri *uri = malloc(sizeof(Uri));

	memset(uri, 0, sizeof(*uri));

	return uri;
}

Uri *uri_parse(const char *uriStr)
{
	Uri *uri = uri_new();
	unsigned int protocolEnd = 0;
	unsigned int hostEnd = 0;

	for (unsigned int index = 1; index < strlen(uriStr); index++)
	{
		// We have found a protocol.
		if (uriStr[index] == ':')
		{
			uri->protocol = malloc(index + 1);
			protocolEnd = index + 3;

			strncpy(uri->protocol, uriStr, index);
			
			uri->protocol[index] = '\0';

			break;
		}
	}

	for (unsigned int index = 1; index + protocolEnd < strlen(uriStr); index++)
	{
		if (uriStr[index + protocolEnd - 1] != '/' && uriStr[index + protocolEnd] == '/')
		{
			uri->host = malloc(index + 1);
			hostEnd = index + protocolEnd;

			strncpy(uri->host, uriStr + protocolEnd, index);
			
			uri->host[index] = '\0';

			break;
		}
	}

	uri->path = malloc(strlen(uriStr) - hostEnd + 1);

	strcpy(uri->path, uriStr + hostEnd);

	return uri;
}

void uri_free(Uri *uri)
{
	if (uri->protocol)
		free_null(uri->protocol);

	if (uri->host)
		free_null(uri->host);
	
	if (uri->path)
		free_null(uri->path);
	
	free_null(uri);
}
