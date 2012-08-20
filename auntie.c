#include "auntie.h"
#include "http.h"
#include "utils.h"

#include <librtmp/rtmp.h>
#include <jansson.h>
#include <libxml/globals.h>
#include <libxml/xmlreader.h>

const uint16_t IVIEW_PORT = 80;
const char ABC_MAIN_URL[] = "www.abc.net.au";
const char IVIEW_CONFIG_URL[] = "/iview/xml/config.xml";
const char HTTP_HEADER_TRANSFER_ENCODING_CHUNKED[] = "chunked";
const char IVIEW_CONFIG_API_SERIES_INDEX[] = "seriesIndex";
const char IVIEW_CONFIG_API_SERIES[] = "series=";
const char IVIEW_CONFIG_TIME_FORMAT[] = "%d/%m/%Y %H:%M:%S";
const char IVIEW_PROGRAM_TIME_FORMAT[] = "%Y-%m-%d %H:%M:%S";
const char IVIEW_CATEGORIES_URL[] = "xml/categories.xml";
const size_t IVIEW_SERIES_ID_LENGTH = 11;
const size_t IVIEW_PROGRAM_ID_LENGTH = 11;
const ssize_t IVIEW_RTMP_BUFFER_SIZE = 8192;
const int IVIEW_REFRESH_INTERVAL = 3600;
const char IVIEW_DOWNLOAD_EXT[] = ".mp4";
const char IVIEW_SWF_URL[] = "http://www.abc.net.au/iview/images/iview.jpg";
const uint32_t IVIEW_SWF_SIZE = 2122;
const uint8_t IVIEW_SWF_HASH[] = {0x96, 0xcc, 0x76, 0xf1, 0xd5, 0x38, 0x5f, 0xb5,
								  0xcd, 0xa6, 0xe2, 0xce, 0x5c, 0x73, 0x32, 0x3a,
								  0x39, 0x90, 0x43, 0xd0, 0xbb, 0x6c, 0x68, 0x7e,
								  0xdd, 0x80, 0x7e, 0x5c, 0x73, 0xc4, 0x2b, 0x37};
const uint16_t IVIEW_RTMP_PORT = 1935;
const unsigned int BYTES_IN_MEGABYTE = 1048576;
const char IVIEW_RTMP_AKAMAI_PROTOCOL[] = "rtmp://";
const char IVIEW_RTMP_AKAMAI_HOST[] = "cp53909.edgefcs.net";
const char IVIEW_RTMP_AKAMAI_APP_PREFIX[] = "/ondemand?auth=";
const char IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX[] = "mp4:flash/playback/_definst_/";

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

IviewConfig *config_parse(const char *xml)
{
	xmlDocPtr reader = xmlParseMemory(xml, strlen(xml));

	if (strcmp((const char *)reader->children->name, "config"))
		return NULL;

	IviewConfig *config = iview_config_new();
	xmlNodePtr node = reader->children->children;

	while (node)
	{
		if (!strcmp((const char *)node->name, "param"))
		{
			xmlAttrPtr property = node->properties;
			char *name = NULL;
			char *value = NULL;

			while (property)
			{
				if (!strcmp((const char *)property->name, "name"))
					name = (char *)property->children->content;

				else if (!strcmp((const char *)property->name, "value"))
					value = (char *)property->children->content;

				if (name && value)
					break;

				property = property->next;
			}

			if (name && value)
			{
				if (!strcmp((char *)name, "api"))
					config->api = strdup(value);

				else if (!strcmp((char *)name, "auth"))
					config->auth = strdup(value);

				else if (!strcmp((char *)name, "tray"))
					config->tray = strdup(value);

				else if (!strcmp((char *)name, "categories"))
					config->categories = strdup(value);

				else if (!strcmp((char *)name, "classifications"))
					config->classifications = strdup(value);

				else if (!strcmp((char *)name, "captions"))
					config->captions = strdup(value);

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
					config->serverStreaming = strdup(value);

				else if (!strcmp((char *)name, "server_fallback"))
					config->serverFallback = strdup(value);

				else if (!strcmp((char *)name, "highlights"))
					config->highlights = strdup(value);

				else if (!strcmp((char *)name, "home"))
					config->home = strdup(value);

				else if (!strcmp((char *)name, "geo"))
					config->geo = strdup(value);

				else if (!strcmp((char *)name, "time"))
					config->time = strdup(value);

				else if (!strcmp((char *)name, "feedback_url"))
					config->feedbackUrl = strdup(value);
			}
		}

		node = node->next;
	}

	xmlFreeDoc(reader);

	return config;
}

IviewSeries *index_parse(const char *json)
{
	json_error_t error;
	json_t *root = json_loads(json, 0, &error);
	IviewSeries *series = NULL;
	IviewSeries *seriesHead = NULL;
	json_t *field = NULL;
	json_t *seriesNode = NULL;

	if (json_is_array(root))
	{
		for (unsigned int seriesIndex = 0; seriesIndex < json_array_size(root); seriesIndex++)
		{
			seriesNode = json_array_get(root, seriesIndex);

			if (json_is_object(seriesNode))
			{
				if (series)
				{
					series->next = iview_series_new();
					series = series->next;
				}

				else
				{
					series = iview_series_new();
					seriesHead = series;
				}

				series->next = NULL;

				field = json_object_get(seriesNode, "a");

				if (json_is_string(field))
					series->id = atoi(json_string_value(field));

				field = json_object_get(seriesNode, "b");

				if (json_is_string(field))
					series->name = strdup(json_string_value(field));

				field = json_object_get(seriesNode, "e");

				if (json_is_string(field))
				{
					char **keywordArray = NULL;
					size_t numKeywords = split(json_string_value(field), " ", &keywordArray);
					IviewKeyword *keyword = NULL;
					IviewKeyword *keywordHead = NULL;

					for (unsigned int keywordIndex = 0; keywordIndex < numKeywords; keywordIndex++)
					{
						if (keyword)
						{
							keyword->next = iview_keyword_new();
							keyword = keyword->next;
						}

						else
						{
							keyword = iview_keyword_new();
							keywordHead = keyword;
						}

						keyword->next = NULL;
						keyword->text = keywordArray[keywordIndex];
					}

					free_null(keywordArray);

					series->keyword = keywordHead;
				}
			}

			series->program = NULL;
		}
	}

	json_decref(root);

	return seriesHead;
}

IviewProgram *series_parse(IviewSeries *series, const char *json)
{
	json_error_t error;
	json_t *root = json_loads(json, 0, &error);
	IviewProgram *program = NULL;
	IviewProgram *programHead = NULL;
	json_t *field = NULL;
	json_t *seriesNode = NULL;
	json_t *programsNode = NULL;
	json_t *programNode = NULL;

	if (json_is_array(root))
	{
		for (unsigned int seriesIndex = 0; seriesIndex < json_array_size(root); seriesIndex++)
		{
			seriesNode = json_array_get(root, seriesIndex);

			if (json_is_object(seriesNode))
			{
				field = json_object_get(seriesNode, "c");

				if (json_is_string(field))
					series->desc = strdup(json_string_value(field));

				field = json_object_get(seriesNode, "d");

				if (json_is_string(field))
					series->image = strdup(json_string_value(field));

				// FIXME: Check that data is consistent with series index.
				programsNode = json_object_get(seriesNode, "f");

				if (json_is_array(programsNode))
				{
					for (unsigned int programIndex = 0; programIndex < json_array_size(programsNode); programIndex++)
					{
						if (!program)
						{
							program = iview_program_new();
							programHead = program;
						}

						else
						{
							program->next = iview_program_new();
							program = program->next;
						}

						program->next = NULL;
						programNode = json_array_get(programsNode, programIndex);

						program->transmissionTime = malloc(sizeof(struct tm));
						program->iviewExpiry = malloc(sizeof(struct tm));
						program->unknownHValue = malloc(sizeof(struct tm));

						memset(program->transmissionTime, 0, sizeof(*program->transmissionTime));
						memset(program->iviewExpiry, 0, sizeof(*program->iviewExpiry));
						memset(program->unknownHValue, 0, sizeof(*program->unknownHValue));

						if (json_is_object(programNode))
						{
							field = json_object_get(programNode, "a");

							if (json_is_string(field))
								program->id = atoi(json_string_value(field));

							field = json_object_get(programNode, "b");

							if (json_is_string(field))
								program->name = strdup(json_string_value(field));

							field = json_object_get(programNode, "d");

							if (json_is_string(field))
								program->desc = strdup(json_string_value(field));

							field = json_object_get(programNode, "f");

							if (json_is_string(field))
								strptime((char *)json_string_value(field), IVIEW_PROGRAM_TIME_FORMAT, program->transmissionTime);

							field = json_object_get(programNode, "g");

							if (json_is_string(field))
								strptime((char *)json_string_value(field), IVIEW_PROGRAM_TIME_FORMAT, program->iviewExpiry);

							field = json_object_get(programNode, "h");

							if (json_is_string(field))
								strptime((char *)json_string_value(field), IVIEW_PROGRAM_TIME_FORMAT, program->unknownHValue);

							field = json_object_get(programNode, "i");

							if (json_is_string(field))
								program->size = atoi((char *)json_string_value(field)) * BYTES_IN_MEGABYTE + BYTES_IN_MEGABYTE;

							field = json_object_get(programNode, "n");

							if (json_is_string(field))
								program->uri = strdup(json_string_value(field));
						}
					}
				}
			}
		}
	}

	json_decref(root);

	return programHead;
}

IviewCategories *categories_parse(const char *xml)
{
	return NULL;
}

IviewAuth *auth_parse(const char *xml)
{
	xmlDocPtr reader = xmlParseMemory(xml, strlen(xml));

	if (strcmp((char *)reader->children->name, "iview"))
		return NULL;

	IviewAuth *auth = iview_auth_new();
	xmlNodePtr node = reader->children->children;

	while (node)
	{
		if (node->type == XML_ELEMENT_NODE)
		{
			if (!strcmp((char *)node->name, "ip"))
				auth->ip = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "isp"))
				auth->isp = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "desc"))
				auth->desc = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "host"))
				auth->host = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "server"))
				auth->server = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "bwtest"))
				auth->bwTest = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "token"))
				auth->token = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "text"))
				auth->text = strdup((char *)node->children->content);

			else if (!strcmp((char *)node->name, "free"))
			{
				if (!strcmp((char *)node->children->content, "yes"))
					auth->free = TRUE;

				else
					auth->free = FALSE;
			}
		}

		node = node->next;
	}

	xmlFreeDoc(reader);

	return auth;
}

struct tm *time_parse(const char *timeStr)
{
	struct tm *lastRefresh = malloc(sizeof(struct tm));

	// strptime() doesn't set all values, but mktime doesn't like this.
	memset(lastRefresh, 0, sizeof(struct tm));

	strptime(timeStr, IVIEW_CONFIG_TIME_FORMAT, lastRefresh);

	// We also need to set tm_isdst to -1 so that the system has to find out DST settings.
	lastRefresh->tm_isdst = -1;

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

	free_null(line);

	return allChunks;
}

char *fetch_uri(const HttpRequest *request)
{
	struct sockaddr_in serverAddress;
	struct hostent *host = gethostbyname(request->host);

	memset((char *)&serverAddress, 0, sizeof serverAddress);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = ((struct in_addr*)host->h_addr_list[0])->s_addr;
	serverAddress.sin_port = htons(IVIEW_PORT);

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	connect(socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
	
	unsigned short requestSize = strlen(request->method)
							   + strlen(request->uri)
							   + strlen(request->protocol)
							   + strlen(request->host)
							   + 15;

	char requestStr[requestSize];

	sprintf(requestStr, "%s %s %s\r\nHost: %s\r\n\r\n", request->method, request->uri, request->protocol, request->host);

	send(socketFd, requestStr, strlen(requestStr), 0);

	FILE *file = fdopen(socketFd, "r");
	
	// Parse HTTP header.
	size_t size = 0;
	char *line = NULL;
	ssize_t z = 0;
	HttpHeader header;
	header.contentLength = 0;

	do
	{
		z = getline(&line, &size, file);
		
		if (z < 3)
			break;

		http_header_parse_field(&header, line);
	} while(z > 2);

	free_null(line);

	char *page = NULL;

	if (header.contentLength == 0)
		page = fetch_uri_parse_transfer_chunks(file);

	else
	{
		page = malloc(header.contentLength + 1);

		memset(page, '\0', header.contentLength + 1);

		fread(page, header.contentLength, 1, file);
	}

	fclose(file);

	return page;
}

void config_fetch(IviewCache *cache)
{
	HttpRequest *request = http_request_new();

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);
	request->uri = strdup(IVIEW_CONFIG_URL);
	request->host = strdup(ABC_MAIN_URL);

	char *page = fetch_uri(request);

	cache->config = config_parse(page);

	free_null(page);

	http_request_free(request);
}

void index_fetch(IviewCache *cache)
{
	HttpRequest *request = http_request_new();

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);
	
	Uri *uri = uri_parse((char *)cache->config->api);

	request->host = strdup(uri->host);
	request->uri = malloc(strlen((char *)uri->path) + strlen(IVIEW_CONFIG_API_SERIES_INDEX) + 1);

	strcpy(request->uri, uri->path);
	strcat(request->uri, IVIEW_CONFIG_API_SERIES_INDEX);

	char *page = fetch_uri(request);

	cache->index = index_parse(page);

	free_null(page);

	http_request_free(request);
	uri_free(uri);
}

void categories_fetch(IviewCache *cache)
{
	HttpRequest *request = http_request_new();

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);
	request->host = strdup(ABC_MAIN_URL);
	request->uri = malloc(strlen((char *)cache->config->categories) + 2);

	strcpy(request->uri, "/");
	strcat(request->uri, (char *)cache->config->categories);

	char *page = fetch_uri(request);

	cache->categories = categories_parse(page);

	free_null(page);

	http_request_free(request);
}

void auth_fetch(IviewCache *cache)
{
	HttpRequest *request = http_request_new();

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);

	Uri *uri = uri_parse((char *)cache->config->auth);

	request->host = strdup(uri->host);
	request->uri = strdup(uri->path);

	char *page = fetch_uri(request);

	cache->auth = auth_parse(page);

	free_null(page);

	http_request_free(request);
	uri_free(uri);
}

void time_fetch(IviewCache *cache)
{
	HttpRequest *request = http_request_new();

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);

	Uri *uri = uri_parse((char *)cache->config->time);

	request->host = strdup(uri->host);
	request->uri = strdup(uri->path);

	char *page = fetch_uri(request);

	cache->lastRefresh = time_parse(page);

	free_null(page);

	http_request_free(request);
	uri_free(uri);
}

void get_programs(const IviewCache *cache, IviewSeries *series)
{
	iview_cache_program_free(series->program);

	HttpRequest *request = http_request_new();
	char seriesIdStr[IVIEW_SERIES_ID_LENGTH];

	request->method = strdup(HTTP_METHOD_GET);
	request->protocol = strdup(HTTP_PROTOCOL_1_1);

	Uri *uri = uri_parse((char *)cache->config->api);

	sprintf(seriesIdStr, "%u", series->id);

	request->host = strdup(uri->host);
	request->uri = malloc(strlen((char *)uri->path) + strlen(IVIEW_CONFIG_API_SERIES) + strlen(seriesIdStr) + 1);

	strcpy(request->uri, uri->path);
	strcat(request->uri, IVIEW_CONFIG_API_SERIES);
	strcat(request->uri, seriesIdStr);

	char *page = fetch_uri(request);

	series->program = series_parse(series, page);

	free_null(page);

	http_request_free(request);
	uri_free(uri);
}

RTMP *download_program_open(IviewCache *cache, const IviewProgram *program)
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
	AVal sockshost = {NULL, 0};
	AVal playpath = {playpathUrl, strlen(playpathUrl)};
	AVal tcUrl = {targetUrl, strlen(targetUrl)};
	AVal swf = {(char *)IVIEW_SWF_URL, strlen((char *)IVIEW_SWF_URL)};
	AVal app = {appUrl, strlen(appUrl)};
	AVal auth = {cache->auth->token, strlen(cache->auth->token)};
	AVal hash = {(char *)IVIEW_SWF_HASH, RTMP_SWF_HASHLEN};

	RTMP_Init(rtmp);
	RTMP_SetupStream(rtmp, RTMP_PROTOCOL_RTMP,
		&host, IVIEW_RTMP_PORT, &sockshost, &playpath, &tcUrl, &swf,
		NULL, &app, &auth, &hash, IVIEW_SWF_SIZE, NULL, NULL, 0, 0, FALSE, 30);

	#ifdef DEBUG
	syslog(LOG_INFO, "\nProtocol: %s", RTMPProtocolStringsLower[rtmp->Link.protocol&7]);
	syslog(LOG_INFO, "\nHost: %s", rtmp->Link.hostname.av_val);
	syslog(LOG_INFO, "\nPort: %i", rtmp->Link.port);
	syslog(LOG_INFO, "\nSockshost: %s", rtmp->Link.sockshost.av_val);
	syslog(LOG_INFO, "\nPlaypath: %s", rtmp->Link.playpath.av_val);
	syslog(LOG_INFO, "\nTcUrl: %s", rtmp->Link.tcUrl.av_val);
	syslog(LOG_INFO, "\nSwfUrl: %s", rtmp->Link.swfUrl.av_val);
	syslog(LOG_INFO, "\nPageUrl: %s", rtmp->Link.pageUrl.av_val);
	syslog(LOG_INFO, "\nApp: %s", rtmp->Link.app.av_val);
	syslog(LOG_INFO, "\nAuth: %s", rtmp->Link.auth.av_val);
	syslog(LOG_INFO, "\nApp: %s %i", app.av_val, app.av_len);
	syslog(LOG_INFO, "\nHash: %s %i", hash.av_val, hash.av_len);
	#endif // DEBUG

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

	RTMP_DeleteStream(rtmp);
	RTMP_Close(rtmp);
	RTMP_Free(rtmp);

	rtmp = NULL;

	return 0;
}

bool iview_cache_index_needs_refresh(const IviewCache *cache)
{
	if (!cache->index || !cache->lastRefresh)
		return TRUE;

	return (time(NULL) - mktime(cache->lastRefresh)) > IVIEW_REFRESH_INTERVAL;
}

void iview_cache_index_refresh(IviewCache *cache)
{
	index_fetch(cache);
	categories_fetch(cache);
	time_fetch(cache);
}

void iview_cache_free(IviewCache *cache)
{
	if (!cache)
		return;

	xmlCleanupParser();

	iview_cache_config_free(cache->config);
	iview_cache_auth_free(cache->auth);
	iview_cache_index_free(cache->index);
	iview_cache_categories_free(cache->categories);

	free_null(cache->lastRefresh);
	free_null(cache);	
}

void iview_cache_config_free(IviewConfig *config)
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

	free_null(config);
}

void iview_cache_auth_free(IviewAuth *auth)
{
	if (!auth)
		return;

	free_null(auth->ip);
	free_null(auth->isp);
	free_null(auth->desc);
	free_null(auth->host);
	free_null(auth->server);
	free_null(auth->bwTest);
	free_null(auth->token);
	free_null(auth->text);

	free_null(auth);
}

IviewSeries *iview_series_new()
{
	IviewSeries *series = malloc(sizeof(IviewSeries));

	memset(series, 0, sizeof(*series));

	return series;
}

IviewProgram *iview_program_new()
{
	IviewProgram *program = malloc(sizeof(IviewProgram));

	memset(program, 0, sizeof(*program));

	return program;
}

IviewKeyword *iview_keyword_new()
{
	IviewKeyword *keyword = malloc(sizeof(IviewKeyword));

	memset(keyword, 0, sizeof(*keyword));

	return keyword;
}

void iview_cache_index_free(IviewSeries *series)
{
	if (!series)
		return;

	IviewSeries *nextSeries = NULL;

	while (series)
	{
		nextSeries = series->next;

		iview_cache_program_free(series->program);
		iview_cache_keyword_free(series->keyword);

		free_null(series->name);

		// Desc is only set if programs are loaded for the series.
		if (series->desc)
			free_null(series->desc);
		
		if (series->image)
			free_null(series->image);
		
		free(series);

		series = nextSeries;
	}
}

void iview_cache_program_free(IviewProgram *program)
{
	if (!program)
		return;

	IviewProgram *nextProgram = NULL;

	while (program)
	{
		nextProgram = program->next;

		free_null(program->name);
		free_null(program->desc);
		free_null(program->uri);
		free_null(program->transmissionTime);
		free_null(program->iviewExpiry);
		free_null(program->unknownHValue);
		free(program);

		program = nextProgram;
	}
}

void iview_cache_keyword_free(IviewKeyword *keyword)
{
	if (!keyword)
		return;

	IviewKeyword *nextKeyword = NULL;

	while (keyword)
	{
		nextKeyword = keyword->next;

		free_null(keyword->text);
		free(keyword);

		keyword = nextKeyword;
	}
}

void iview_cache_categories_free(IviewCategories *categories)
{

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

IviewCache *iview_cache_new()
{
	IviewCache *cache = malloc(sizeof(IviewCache));

	memset(cache, 0, sizeof(*cache));

	return cache;
}

IviewConfig *iview_config_new()
{
	IviewConfig *config = malloc(sizeof(IviewConfig));

	memset(config, 0, sizeof(*config));

	return config;
}

IviewAuth *iview_auth_new()
{
	IviewAuth *auth = malloc(sizeof(IviewAuth));

	memset(auth, 0, sizeof(*auth));

	return auth;
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

IviewSeries *iview_get_series(const IviewCache *cache, const char *seriesName)
{
	IviewSeries *series = cache->index;

	while (series)
	{
		if (!strcmp(series->name, seriesName))
			return series;

		series = series->next;
	}

	return NULL;
}

IviewProgram *iview_get_program(const IviewSeries *series, const char *programName)
{
	IviewProgram *program = series->program;

	while (program)
	{
		if (!strcmp(program->name, programName))
			return program;

		program = program->next;
	}

	return NULL;
}
