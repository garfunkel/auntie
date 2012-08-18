#include "auntie.h"

#include <pthread.h>

const unsigned short IVIEW_PORT = 80;
const char *ABC_MAIN_URL = "www.abc.net.au";
const char *IVIEW_CONFIG_URL = "/iview/xml/config.xml";
const char *HTTP_HEADER_TRANSFER_ENCODING_CHUNKED = "chunked";
const char *IVIEW_CONFIG_API_SERIES_INDEX = "seriesIndex";
const char *IVIEW_CONFIG_API_SERIES = "series=";
const char *IVIEW_CONFIG_TIME_FORMAT = "%d/%m/%Y %H:%M:%S";
const char *IVIEW_CATEGORIES_URL = "xml/categories.xml";
const size_t IVIEW_SERIES_ID_LENGTH = 11;
const size_t IVIEW_PROGRAM_ID_LENGTH = 11;
const ssize_t IVIEW_RTMP_BUFFER_SIZE = 8192;
const int IVIEW_REFRESH_INTERVAL = 3600;
const char *IVIEW_DOWNLOAD_EXT = ".mp4";
const char *IVIEW_SWF_URL = "http://www.abc.net.au/iview/images/iview.jpg";
const uint32_t IVIEW_SWF_SIZE = 2122;
const uint8_t IVIEW_SWF_HASH[] = {0x96, 0xcc, 0x76, 0xf1, 0xd5, 0x38, 0x5f, 0xb5,
								  0xcd, 0xa6, 0xe2, 0xce, 0x5c, 0x73, 0x32, 0x3a,
								  0x39, 0x90, 0x43, 0xd0, 0xbb, 0x6c, 0x68, 0x7e,
								  0xdd, 0x80, 0x7e, 0x5c, 0x73, 0xc4, 0x2b, 0x37};
const uint16_t IVIEW_RTMP_PORT = 1935;
const unsigned int BYTES_IN_MEGABYTE = 1048576;
const char *IVIEW_RTMP_AKAMAI_PROTOCOL = "rtmp://";
const char *IVIEW_RTMP_AKAMAI_HOST = "cp53909.edgefcs.net";
const char *IVIEW_RTMP_AKAMAI_APP_PREFIX = "/ondemand?auth=";
const char *IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX = "mp4:flash/playback/_definst_/";

size_t count(const char *input, const char *seps)
{
	size_t numTokens = 0;
	char *token = NULL;
	char *inputDup = strdup(input);

	token = strtok(inputDup, seps);

	while (token)
	{
		numTokens++;
		token = strtok(NULL, seps);
	}

	// We don't count the end of the string as a match.
	if (numTokens > 0)
		numTokens--;

	free_null(inputDup);

	return numTokens;
}

unsigned int split(const char *input, const char *seps, char ***output)
{
	size_t numTokens = count(input, seps) + 1;
	char *inputDup = strdup(input);

	*output = (char **)calloc(numTokens, sizeof(char *));
	(*output)[0] = strdup(strtok(inputDup, seps));

	for (unsigned int i = 1; i < numTokens; i++)
		(*output)[i] = strdup(strtok(NULL, seps));

	free_null(inputDup);

	return numTokens;
}

char http_header_parse_field(struct HttpHeader *header, const char *field)
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

unsigned int getline_fd(char **buffer, size_t *bufferSize, int fd)
{
	return 0;
}

struct Uri uri_parse(const char *uriStr)
{
	struct Uri uri;
	unsigned int protocolEnd = -1;
	unsigned int hostEnd = -1;

	for (unsigned int index = 1; index < strlen(uriStr); index++)
	{
		// We have found a protocol.
		if (uriStr[index] == ':')
		{
			uri.protocol = malloc(index + 1);
			protocolEnd = index + 3;

			strncpy(uri.protocol, uriStr, index);
			
			uri.protocol[index] = '\0';

			break;
		}
	}

	for (unsigned int index = 1; index + protocolEnd < strlen(uriStr); index++)
	{
		if (uriStr[index + protocolEnd - 1] != '/' && uriStr[index + protocolEnd] == '/')
		{
			uri.host = malloc(index + 1);
			hostEnd = index + protocolEnd;

			strncpy(uri.host, uriStr + protocolEnd, index);
			
			uri.host[index] = '\0';

			break;
		}
	}

	uri.path = malloc(strlen(uriStr - hostEnd + 1));

	strcpy(uri.path, uriStr + hostEnd);

	return uri;
}

struct IviewConfig *config_parse(const char *xml)
{
	xmlDocPtr reader = xmlParseMemory(xml, strlen(xml));
	struct IviewConfig *config = malloc(sizeof(struct IviewConfig));

	if (strcmp((const char *)reader->children->name, "config"))
		return NULL;

	xmlNodePtr node = reader->children->children;

	while (node)
	{
		if (!strcmp((const char *)node->name, "param"))
		{
			xmlAttrPtr property = node->properties;
			unsigned char *name = NULL;
			unsigned char *value = NULL;

			while (property)
			{
				if (!strcmp((const char *)property->name, "name"))
					name = property->children->content;

				else if (!strcmp((const char *)property->name, "value"))
					value = property->children->content;

				if (name && value)
					break;

				property = property->next;
			}

			if (name && value)
			{
				if (!strcmp((char *)name, "api"))
					config->api = value;

				else if (!strcmp((char *)name, "auth"))
					config->auth = value;

				else if (!strcmp((char *)name, "tray"))
					config->tray = value;

				else if (!strcmp((char *)name, "categories"))
					config->categories = value;

				else if (!strcmp((char *)name, "classifications"))
					config->classifications = value;

				else if (!strcmp((char *)name, "captions"))
					config->captions = value;

				else if (!strcmp((char *)name, "captions_offset"))
					config->captionsOffset = atoi((char *)value);

				else if (!strcmp((char *)name, "captions_live_offset"))
					config->captionsLiveOffset = atoi((char *)value);

				else if (!strcmp((char *)name, "lie_streaming"))
				{
					if (!strcmp((char *)value, "true"))
						config->liveStreaming = TRUE;

					else
						config->liveStreaming = FALSE;
				}

				else if (!strcmp((char *)name, "server_streaming"))
					config->serverStreaming = value;

				else if (!strcmp((char *)name, "server_fallback"))
					config->serverFallback = value;

				else if (!strcmp((char *)name, "highlights"))
					config->highlights = value;

				else if (!strcmp((char *)name, "home"))
					config->home = value;

				else if (!strcmp((char *)name, "geo"))
					config->geo = value;

				else if (!strcmp((char *)name, "time"))
					config->time = value;

				else if (!strcmp((char *)name, "feedback_url"))
					config->feedbackUrl = value;
			}
		}

		node = node->next;
	}

	return config;
}

struct IviewSeries *index_parse(const char *json)
{
	json_error_t error;
	json_t *root = json_loads(json, 0, &error);
	struct IviewSeries *series = NULL;
	struct IviewSeries *seriesHead = NULL;
	char *keywords;

	if (json_is_array(root))
	{
		for (unsigned int seriesIndex = 0; seriesIndex < json_array_size(root); seriesIndex++)
		{
			json_t *nextSeries = json_array_get(root, seriesIndex);
			json_t *tmp;

			if (json_is_object(nextSeries))
			{
				if (series)
				{
					series->next = malloc(sizeof(struct IviewSeries));
					series = series->next;
				}

				else
				{
					series = malloc(sizeof(struct IviewSeries));
					seriesHead = series;
				}

				series->next = NULL;

				tmp = json_object_get(nextSeries, "a");

				if (json_is_string(tmp))
					series->id = atoi(json_string_value(tmp));

				tmp = json_object_get(nextSeries, "b");

				if (json_is_string(tmp))
					series->name = (char *)json_string_value(tmp);

				tmp = json_object_get(nextSeries, "e");

				if (json_is_string(tmp))
				{
					keywords = (char *)json_string_value(tmp);
					char **keywordArray = NULL;
					size_t numKeywords = split(keywords, " ", &keywordArray);
					struct IviewKeyword *keyword = NULL;
					struct IviewKeyword *keywordHead = NULL;

					for (unsigned int keywordIndex = 0; keywordIndex < numKeywords; keywordIndex++)
					{
						if (keyword)
						{
							keyword->next = malloc(sizeof(struct IviewKeyword));
							keyword = keyword->next;
						}

						else
						{
							keyword = malloc(sizeof(struct IviewKeyword));
							keywordHead = keyword;
						}

						keyword->next = NULL;
						keyword->text = keywordArray[keywordIndex];
					}

					series->keyword = keywordHead;
				}
			}

			series->program = NULL;
		}
	}

	return seriesHead;
}

struct IviewProgram *series_parse(const char *json)
{
	json_error_t error;
	json_t *root = json_loads(json, 0, &error);
	struct IviewProgram *program = NULL;
	struct IviewProgram *programHead = NULL;

	if (json_is_array(root))
	{
		for (unsigned int seriesIndex = 0; seriesIndex < json_array_size(root); seriesIndex++)
		{
			json_t *seriesNode = json_array_get(root, seriesIndex);

			if (json_is_object(seriesNode))
			{
				// FIXME: Check that data is consistent with series index.
				json_t *programsNode = json_object_get(seriesNode, "f");

				if (json_is_array(programsNode))
				{
					for (unsigned int programIndex = 0; programIndex < json_array_size(programsNode); programIndex++)
					{
						if (!program)
						{
							program = malloc(sizeof(struct IviewProgram));
							programHead = program;
						}

						else
						{
							program->next = malloc(sizeof(struct IviewProgram));
							program = program->next;
						}

						program->next = NULL;
						json_t *programNode = json_array_get(programsNode, programIndex);
						json_t *tmp;

						if (json_is_object(programNode))
						{
							tmp = json_object_get(programNode, "a");

							if (json_is_string(tmp))
								program->id = atoi(json_string_value(tmp));

							tmp = json_object_get(programNode, "b");

							if (json_is_string(tmp))
								program->name = (char *)json_string_value(tmp);

							tmp = json_object_get(programNode, "d");

							if (json_is_string(tmp))
								program->desc = (char *)json_string_value(tmp);

							// TODO: Do others.
							tmp = json_object_get(programNode, "i");

							if (json_is_string(tmp))
							{
								char *sizeStr = (char *)json_string_value(tmp);

								program->size = atoi((char *)sizeStr) * BYTES_IN_MEGABYTE + BYTES_IN_MEGABYTE;

								free_null(sizeStr);
							}

							tmp = json_object_get(programNode, "n");

							if (json_is_string(tmp))
								program->uri = (char *)json_string_value(tmp);
						}
					}
				}
			}
		}
	}

	return programHead;
}

struct IviewCategories *categories_parse(const char *xml)
{
	return NULL;
}

struct IviewAuth *auth_parse(const char *xml)
{
	xmlDocPtr reader = xmlParseMemory(xml, strlen(xml));
	xmlNodePtr node = xmlFirstElementChild(reader->children);
	struct IviewAuth *auth = malloc(sizeof(struct IviewAuth));

	while (node)
	{
		if (!strcmp((char *)node->name, "ip"))
			auth->ip = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "isp"))
			auth->isp = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "desc"))
			auth->desc = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "host"))
			auth->host = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "server"))
			auth->server = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "bwtest"))
			auth->bwTest = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "token"))
			auth->token = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "text"))
			auth->text = (char *)xmlNodeGetContent(node);

		else if (!strcmp((char *)node->name, "free"))
		{
			if (!strcmp((char *)xmlNodeGetContent(node), "yes"))
				auth->free = TRUE;

			else
				auth->free = FALSE;
		}

		node = xmlNextElementSibling(node);
	}

	return auth;
}

struct tm *time_parse(const char *timeStr)
{
	struct tm *lastRefresh = malloc(sizeof(struct tm));

	strptime(timeStr, IVIEW_CONFIG_TIME_FORMAT, lastRefresh);

	return lastRefresh;
}

char *fetch_uri_parse_transfer_chunks(FILE *file)
{
	size_t lineSize = 0;
	char *line = NULL;
	size_t chunkSize = 0;
	char *chunk = NULL;
	char *allChunks = NULL;
	char *tmpAllChunks = NULL;
	char chunkSep[2];
	bool firstChunk = TRUE;
	size_t allChunksSize = 0;

	do 
	{
		getline(&line, &lineSize, file);
		sscanf(line, "%x", (unsigned int *)&chunkSize);

		if (chunkSize == 0)
			break;

		chunk = malloc(chunkSize + 1);

		fread(chunk, chunkSize, 1, file);
		fread(chunkSep, 2, 1, file);

		if (firstChunk)
			allChunks = chunk;

		else
		{
			tmpAllChunks = malloc(allChunksSize + chunkSize + 1);

			memcpy(tmpAllChunks, allChunks, allChunksSize);
			memcpy(tmpAllChunks + allChunksSize, chunk, chunkSize);

			allChunks = tmpAllChunks;
		}

		allChunksSize += chunkSize;
		allChunks[allChunksSize] = '\0';
		firstChunk = FALSE;
	} while (chunkSize > 0);

	return allChunks;
}

char *fetch_uri(const struct HttpRequest request)
{
	struct sockaddr_in serverAddress;
	struct hostent *host = gethostbyname(request.host);

	memset((char *)&serverAddress, 0, sizeof serverAddress);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = ((struct in_addr*)host->h_addr_list[0])->s_addr;
	serverAddress.sin_port = htons(IVIEW_PORT);

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	connect(socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
	
	unsigned short requestSize = strlen(request.method)
							   + strlen(request.uri)
							   + strlen(request.protocol)
							   + strlen(request.host)
							   + 15;

	char requestStr[requestSize];

	sprintf(requestStr, "%s %s %s\r\nHost: %s\r\n\r\n", request.method, request.uri, request.protocol, request.host);

	send(socketFd, requestStr, strlen(requestStr), 0);

	FILE *file = fdopen(socketFd, "r");
	
	// Parse HTTP header.
	size_t size = 0;
	char *line = NULL;
	ssize_t z = 0;
	struct HttpHeader header;
	header.contentLength = 0;

	do
	{
		z = getline(&line, &size, file);
		
		if (z < 3)
			break;

		http_header_parse_field(&header, line);
	} while(z > 2);

	char *page = NULL;

	if (header.contentLength == 0)
		page = fetch_uri_parse_transfer_chunks(file);

	else
	{
		page = malloc(header.contentLength + 1);

		memset(page, '\0', header.contentLength);

		fread(page, header.contentLength, 1, file);
	}

	//printf("\n%s\n", page);

	return page;
}

void config_fetch(struct Cache *cache)
{
	struct HttpRequest request;

	request.method = "GET";
	request.protocol = "HTTP/1.1";
	request.uri = "/iview/xml/config.xml";
	request.host = "www.abc.net.au";

	char *page = fetch_uri(request);

	cache->config = config_parse(page);
}

void index_fetch(struct Cache *cache)
{
	struct HttpRequest request;

	request.method = "GET";
	request.protocol = "HTTP/1.1";
	
	struct Uri uri = uri_parse((char *)cache->config->api);

	request.host = uri.host;
	request.uri = malloc(strlen((char *)uri.path) + strlen(IVIEW_CONFIG_API_SERIES_INDEX) + 1);

	strcpy(request.uri, uri.path);
	strcat(request.uri, IVIEW_CONFIG_API_SERIES_INDEX);

	char *page = fetch_uri(request);

	cache->index = index_parse(page);
}

void categories_fetch(struct Cache *cache)
{
	struct HttpRequest request;

	request.method = "GET";
	request.protocol = "HTTP/1.1";
	request.host = (char *)ABC_MAIN_URL;
	request.uri = malloc(strlen((char *)cache->config->categories) + 2);

	strcpy(request.uri, "/");
	strcat(request.uri, (char *)cache->config->categories);

	char *page = fetch_uri(request);

	cache->categories = categories_parse(page);
}

void auth_fetch(struct Cache *cache)
{
	struct HttpRequest request;

	request.method = "GET";
	request.protocol = "HTTP/1.1";

	struct Uri uri = uri_parse((char *)cache->config->auth);

	request.host = uri.host;
	request.uri = uri.path;

	char *page = fetch_uri(request);

	cache->auth = auth_parse(page);
}

void time_fetch(struct Cache *cache)
{
	struct HttpRequest request;

	request.method = "GET";
	request.protocol = "HTTP/1.1";

	struct Uri uri = uri_parse((char *)cache->config->time);

	request.host = uri.host;
	request.uri = uri.path;

	char *page = fetch_uri(request);

	cache->lastRefresh = time_parse(page);
}

struct IviewSeries *get_series(const struct Cache *cache)
{
	struct IviewSeries *series = cache->index;
	char input[IVIEW_SERIES_ID_LENGTH];
	unsigned int seriesId = 0;

	while (series)
	{
		printf("%u\t%s\n", series->id, series->name);

		series = series->next;
	}

	printf("\nEnter series ID: ");

	char *inputPtr = fgets(input, IVIEW_SERIES_ID_LENGTH, stdin);

	if (inputPtr)
		seriesId = atoi(input);

	if (seriesId < 1)
		return NULL;

	series = cache->index;

	while (series)
	{
		if (series->id == seriesId)
			return series;

		series = series->next;
	}

	return NULL;
}

void get_programs(const struct Cache *cache, struct IviewSeries *series)
{
	iview_cache_program_free(series->program);

	struct HttpRequest request;
	char seriesIdStr[IVIEW_SERIES_ID_LENGTH];

	request.method = "GET";
	request.protocol = "HTTP/1.1";

	struct Uri uri = uri_parse((char *)cache->config->api);

	sprintf(seriesIdStr, "%u", series->id);

	request.host = uri.host;
	request.uri = malloc(strlen((char *)uri.path) + strlen(IVIEW_CONFIG_API_SERIES) + strlen(seriesIdStr) + 1);

	strcpy(request.uri, uri.path);
	strcat(request.uri, IVIEW_CONFIG_API_SERIES);
	strcat(request.uri, seriesIdStr);

	char *page = fetch_uri(request);

	series->program = series_parse(page);
}

struct IviewProgram *select_program(const struct IviewSeries *series)
{
	struct IviewProgram *program = series->program;
	char input[IVIEW_PROGRAM_ID_LENGTH];
	unsigned int programId = 0;

	while (program)
	{
		printf("%u\t%s\n", program->id, program->name);

		program = program->next;
	}

	printf("\nEnter program ID: ");

	char *inputPtr = fgets(input, IVIEW_PROGRAM_ID_LENGTH, stdin);

	if (inputPtr)
		programId = atoi(input);

	if (programId > 0)
	{
		program = series->program;

		while (program)
		{
			if (program->id == programId)
				return program;

			program = program->next;
		}
	}

	return NULL;
}

RTMP *download_program_open(struct Cache *cache, const struct IviewProgram *program)
{
	// Create session handle and initialise.
	RTMP *rtmp = RTMP_Alloc();

	if (!rtmp)
	{
		#ifdef DEBUG
		syslog(LOG_ERR, "Could not allocate RTMP structure.");
		#endif // DEBUG

		return 0;
	}

	auth_fetch(cache);

	char targetUrl[strlen(IVIEW_RTMP_AKAMAI_PROTOCOL)
		+ strlen(IVIEW_RTMP_AKAMAI_HOST)
		+ strlen(IVIEW_RTMP_AKAMAI_APP_PREFIX)
		+ strlen(cache->auth->token) + 1];
	strcpy(targetUrl, IVIEW_RTMP_AKAMAI_PROTOCOL);
	strcat(targetUrl, IVIEW_RTMP_AKAMAI_HOST);
	strcat(targetUrl, IVIEW_RTMP_AKAMAI_APP_PREFIX);
	strcat(targetUrl, cache->auth->token);

	char appUrl[strlen(IVIEW_RTMP_AKAMAI_APP_PREFIX)
		+ strlen(cache->auth->token) + 1];
	strcpy(appUrl, IVIEW_RTMP_AKAMAI_APP_PREFIX);
	strcat(appUrl, cache->auth->token);

	char playpathUrl[strlen(IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX)
		+ strlen(program->uri) + 1];
	strcpy(playpathUrl, IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX);
	strcat(playpathUrl, program->uri);

	AVal host = {(char *)IVIEW_RTMP_AKAMAI_HOST, strlen((char *)IVIEW_RTMP_AKAMAI_HOST)};
	AVal flashVer = {RTMP_DefaultFlashVer.av_val, RTMP_DefaultFlashVer.av_len};
	AVal sockshost = {NULL, 0};
	AVal playpath = {playpathUrl, strlen(playpathUrl)};
	AVal tcUrl = {targetUrl, strlen(targetUrl)};
	AVal swf = {(char *)IVIEW_SWF_URL, strlen((char *)IVIEW_SWF_URL)};
	AVal page = {NULL, 0};
	AVal app = {appUrl, strlen(appUrl)};
	AVal auth = {cache->auth->token, strlen(cache->auth->token)};
	AVal hash = {(char *)IVIEW_SWF_HASH, RTMP_SWF_HASHLEN};
	AVal sub = {NULL, 0};

	RTMP_Init(rtmp);
	RTMP_SetupStream(rtmp, RTMP_PROTOCOL_RTMP,
		&host, IVIEW_RTMP_PORT, &sockshost, &playpath, &tcUrl, &swf,
		&page, &app, &auth, &hash, IVIEW_SWF_SIZE, &flashVer, &sub, 0, 0, FALSE, 30);

	printf("\nProtocol: %s", RTMPProtocolStringsLower[rtmp->Link.protocol&7]);
	printf("\nHost: %s", rtmp->Link.hostname.av_val);
	printf("\nPort: %i", rtmp->Link.port);
	printf("\nSockshost: %s", rtmp->Link.sockshost.av_val);
	printf("\nPlaypath: %s", rtmp->Link.playpath.av_val);
	printf("\nTcUrl: %s", rtmp->Link.tcUrl.av_val);
	printf("\nSwfUrl: %s", rtmp->Link.swfUrl.av_val);
	printf("\nPageUrl: %s", rtmp->Link.pageUrl.av_val);
	printf("\nApp: %s", rtmp->Link.app.av_val);
	printf("\nAuth: %s", rtmp->Link.auth.av_val);
	printf("\nApp: %s %i", app.av_val, app.av_len);
	printf("\nHash: %s %i", hash.av_val, hash.av_len);

	if (!RTMP_Connect(rtmp, NULL))
	{
		#ifdef DEBUG
		syslog(LOG_ERR, "Could not connect to RTMP server.");
		#endif // DEBUG

		return 0;
	}

	if (!RTMP_ConnectStream(rtmp, 0))
	{
		#ifdef DEBUG
		syslog(LOG_ERR, "Could not connect to RTMP stream.");
		#endif // DEBUG

		return 0;
	}

	#ifdef DEBUG
	syslog(LOG_INFO, "RTMP Stream started...");
	#endif // DEBUG

	return rtmp;
}

int download_program_read(RTMP *rtmp, char *buffer, size_t size, off_t offset)
{
	int total = 0, num = 0;

	do
	{
		num = RTMP_Read(rtmp, buffer + total, size - total);

		if (num > 0)
			total += num;

	} while (num > -1 && total < size && !RTMP_ctrlC && RTMP_IsConnected(rtmp) && !RTMP_IsTimedout(rtmp));

	return total;
}

int download_program_close(RTMP *rtmp)
{
	#ifdef DEBUG
	syslog(LOG_INFO, "Stream finished, freeing RTMP structure...");
	#endif // DEBUG

	RTMP_Free(rtmp);

	return 0;
}

bool iview_cache_index_needs_refresh(const struct Cache *cache)
{
	if (!cache->index || !cache->lastRefresh)
		return TRUE;

	time_t refreshTs = mktime(cache->lastRefresh);
	time_t currentTs = time(NULL);

	return (currentTs - refreshTs) > IVIEW_REFRESH_INTERVAL;
}

void iview_cache_index_refresh(struct Cache *cache)
{
	index_fetch(cache);
	categories_fetch(cache);
	time_fetch(cache);
}

void iview_cache_config_free(struct IviewConfig *config)
{
	if (!config)
		return;

	free_null(config->api);
	free_null(config->auth);
	free_null(config->tray);
	free_null(config->categories);
	free_null(config->classifications);
	free_null(config->captions);
	free_null(config->serverStreaming);
	free_null(config->serverFallback);
	free_null(config->highlights);
	free_null(config->home);
	free_null(config->geo);
	free_null(config->time);
	free_null(config->feedbackUrl);
}

void iview_cache_auth_free(struct IviewAuth *auth)
{
	if (!auth)
		return;

	free(auth->ip);
	free(auth->isp);
	free(auth->desc);
	free(auth->host);
	free(auth->server);
	free(auth->bwTest);
	free(auth->token);
	free(auth->text);
}

void iview_cache_index_free(struct IviewSeries *series)
{
	if (!series)
		return;

	struct IviewSeries *nextSeries = series->next;

	while (series)
	{
		iview_cache_program_free(series->program);

		free_null(series->name);
		free_null(series->desc);
		free_null(series->image);
		free(series);

		series = nextSeries;
		nextSeries = series->next;
	}
}

void iview_cache_program_free(struct IviewProgram *program)
{
	if (!program)
		return;

	struct IviewProgram *nextProgram = program->next;

	while (program)
	{
		free_null(program->name);
		free_null(program->desc);
		free_null(program->uri);
		free_null(program->transmissionTime);
		free_null(program->iviewExpiry);
		free(program);

		program = nextProgram;
		nextProgram = program->next;
	}
}

void iview_cache_keyword_free(struct IviewKeyword *keyword)
{
	if (!keyword)
		return;

	struct IviewKeyword *nextKeyword = keyword->next;

	while (keyword)
	{
		free_null(keyword->text);
		free(keyword);

		keyword = nextKeyword;
		nextKeyword = keyword->next;
	}
}

void iview_cache_categories_free(struct IviewCategories *categories)
{

}

void handle_sigint(int sig)
{
	printf("\nRecieved SIGINT, exiting...\n");

	exit(0);
}

char *strjoin(const char *first, const char *second)
{
	unsigned int joinedLen = strlen(first) + strlen(second) + 1;

	char *joined = malloc(joinedLen);

	strcpy(joined, first);
	strcat(joined, second);

	return joined;
}

char *strcreplace(const char *str, const char from, const char to)
{
	char *retStr = strdup(str);

	for (unsigned int i = 0; i < strlen(retStr); i++)
	{
		if (retStr[i] == from)
			retStr[i] = to;
	}

	return retStr;
}

struct Cache *iview_cache_new()
{
	struct Cache *cache = malloc(sizeof(struct Cache));

	memset(cache, 0, sizeof(struct Cache));

	return cache;
}

unsigned char *iview_filename_encode(const unsigned char *fileName)
{
	if (!fileName)
		return NULL;

	ssize_t bufferSize = strlen((char *)fileName) + 1;

	for (unsigned int i = 0; i < strlen((char *)fileName); i++)
	{
		if (fileName[i] == '/')
			bufferSize += 2;
	}

	unsigned char *outName = malloc(bufferSize);
	unsigned int nextIndex = 0;

	for (unsigned int i = 0; i < bufferSize; i++)
	{
		if (fileName[nextIndex] == '/')
		{
			outName[i] = 0xE2;
			outName[++i] = 0x88;
			outName[++i] = 0x95;
		}

		else
			outName[i] = fileName[nextIndex];

		nextIndex++;
	}

	return outName;
}

unsigned char *iview_filename_decode(const unsigned char *fileName)
{
	if (!fileName)
		return NULL;

	ssize_t bufferSize = strlen((char *)fileName) + 1;

	for (unsigned int i = 0; i < strlen((char *)fileName); i++)
	{
		if (fileName[i] == 0xE2 && fileName[i + 1] == 0x88 && fileName[i + 2] == 0x95)
			bufferSize -= 2;
	}

	unsigned char *outName = malloc(bufferSize);
	unsigned int nextIndex = 0;

	for (unsigned int i = 0; i <= strlen((char *)fileName); i++)
	{
		if (fileName[i] == 0xE2 && fileName[i + 1] == 0x88 && fileName[i + 2] == 0x95)
		{
			outName[nextIndex] = '/';
			i += 2;
		}

		else
			outName[nextIndex] = fileName[i];

		nextIndex += 1;
	}

	return outName;
}

void free_null(void *ptr)
{
	free(ptr);

	ptr = NULL;
}
