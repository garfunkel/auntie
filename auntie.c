#include "auntie.h"

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

	free(inputDup);

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

	free(inputDup);

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

struct IviewProgram *get_programs(const struct Cache *cache, const struct IviewSeries *series)
{
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

	return series_parse(page);
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

void download_program(struct Cache *cache, const struct IviewProgram *program)
{
	int status = 0;

	auth_fetch(cache);

	printf("\n%s %s", cache->auth->host, program->uri);

	// Create session handle and initialise.
	RTMP *rtmp = RTMP_Alloc();

	RTMP_Init(rtmp);
	char tcUrl[256];
	//char *swfVfy = "http://www.abc.net.au/iview/images/iview.jpg";

	char *url = "rtmp://cp53909.edgefcs.net////flash/playback/_definst_/gavinandstacey_01_06.mp4";
	strcpy(tcUrl, "rtmp://cp53909.edgefcs.net/ondemand?auth=");
	strcat(tcUrl, cache->auth->token);

	printf("\ntcUrl: %s", tcUrl);

	// 2816053 531746

	status = RTMP_SetupURL(rtmp, url);

	rtmp->Link.tcUrl.av_val = tcUrl;
	rtmp->Link.tcUrl.av_len = strlen(tcUrl);

	printf("SetupURL: %s %i %i\n", rtmp->Link.tcUrl.av_val, rtmp->Link.tcUrl.av_len, status);

	status = RTMP_Connect(rtmp, NULL);

	printf("Connect: %i\n", status);

	status = RTMP_ConnectStream(rtmp, 0);

	printf("ConnectStream: %i\n", status);

	char *buffer = malloc(IVIEW_RTMP_BUFFER_SIZE);
	int total = 0;

	FILE *file = fopen("output", "w");

	while (total < IVIEW_RTMP_BUFFER_SIZE)
	{
		int num = RTMP_Read(rtmp, buffer, IVIEW_RTMP_BUFFER_SIZE);
		total += num;

		printf("Read: %i", num);

		fwrite(buffer, num, 1, file);
	}

	free(buffer);

	buffer = NULL;

	fclose(file);
}

bool iview_cache_index_needs_refresh(const struct Cache *cache)
{
	if (!cache->index || !cache->lastRefresh)
		return TRUE;

	time_t refreshTs = mktime(cache->lastRefresh);
	time_t currentTs = time(NULL);

	return (currentTs - refreshTs) > FUSE_IVIEW_REFRESH_INTERVAL;
}

void iview_cache_index_refresh(struct Cache *cache)
{
	index_fetch(cache);
	categories_fetch(cache);
	time_fetch(cache);
}

void *fuse_iview_init(struct fuse_conn_info *conn)
{
	struct Cache *cache = IVIEW_DATA;

	srand(time(NULL));

	cache->rootDir = fuse_iview_generate_root_dir();

	return cache;
}

int fuse_iview_getattr(const char *path, struct stat *attrStat)
{
	syslog(LOG_INFO, "getattr: %s", path);

	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);
	char *programName = fuse_get_iview_program_name_from_path(path);
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	int status = 0;
	struct Cache *cache = IVIEW_DATA;


	if (programName)
	{
		struct IviewSeries *series = cache->index;

		while (series)
		{
			if (!strcmp(series->name, seriesName))
			{
				struct IviewProgram *program = series->program;

				while (program)
				{
					if (!strcmp(program->name, programName))
					{
						attrStat->st_mode = S_IFDIR | 0754;
						attrStat->st_nlink = 3;
						attrStat->st_uid = getuid();
						attrStat->st_gid = getgid();
						attrStat->st_size = FUSE_IVIEW_BLOCK_SIZE;
						attrStat->st_blksize = FUSE_IVIEW_BLOCK_SIZE;
						attrStat->st_blocks = FUSE_IVIEW_BLOCK_COUNT;
						attrStat->st_atime = time(NULL);
						attrStat->st_mtime = mktime(cache->lastRefresh);

						break;
					}

					program = program->next;

					if (!program)
					{
						status = -ENOENT;

						break;
					}
				}
			}

			series = series->next;

			if (!series)
				status = -ENOENT;
		}
	}

	else if (seriesName)
	{
		struct IviewSeries *series = cache->index;

		while (series)
		{
			if (!strcmp(series->name, seriesName))
			{
				attrStat->st_mode = S_IFDIR | 0754;
				attrStat->st_nlink = 3;
				attrStat->st_uid = getuid();
				attrStat->st_gid = getgid();
				attrStat->st_size = FUSE_IVIEW_BLOCK_SIZE;
				attrStat->st_blksize = FUSE_IVIEW_BLOCK_SIZE;
				attrStat->st_blocks = FUSE_IVIEW_BLOCK_COUNT;
				attrStat->st_atime = time(NULL);
				attrStat->st_mtime = mktime(cache->lastRefresh);

				break;
			}

			series = series->next;

			if (!series)
				status = -ENOENT;
		}
	}

	else
	{
		status = lstat(fullPath, attrStat);
		syslog(LOG_INFO, "getattr status %i %s", status, strerror(status));

		if (status)
			return -ENOENT;
	}

	return status;
}

int fuse_iview_readlink(const char *path, char *buffer, size_t size)
{
	syslog(LOG_INFO, "readlink");

	return 0;
}

int fuse_iview_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "readdir %s", path);

	//DIR *dir = (DIR *)info->fh;

	//struct dirent *dirEnt = readdir(dir);

	//if (!dirEnt)
	//	return -errno;

	struct Cache *cache = IVIEW_DATA;

	if (iview_cache_index_needs_refresh(cache))
		iview_cache_index_refresh(cache);

	char *seriesName = fuse_get_iview_series_name_from_path(path);

	if (!strcmp(path, FUSE_SERIES_ROOT))
	{
		struct IviewSeries *series = cache->index;

		while (series)
		{
			char *seriesName = strcreplace(series->name, '/', '-');

			filler(buffer, seriesName, NULL, 0);

			series = series->next;
		}
	}

	else if (seriesName)
	{
		struct IviewSeries *series = cache->index;

		while (series)
		{
			if (!strcmp(series->name, seriesName))
			{
				if (!series->program)
					get_programs(cache, series);

				struct IviewProgram *program = series->program;

				while (program)
				{
					filler(buffer, program->name, NULL, 0);

					program = program->next;
				}

				break;
			}

			series = series->next;
		}
	}

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	return 0;
}

int fuse_iview_opendir(const char *path, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "opendir %s", path);

	// Directory only valid if root, or series.
	if (strcmp(path, "/") && !fuse_get_iview_series_name_from_path(path))
		return -ENOENT;

	//char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);
	//DIR *dir = opendir(fullPath);
	//int status = 0;

	//info->fh = (intptr_t) dir;

	//if (!dir)
	//	return -errno;

	return 0;
}

int fuse_iview_mknod(const char *path, mode_t mode, dev_t dev)
{
	syslog(LOG_INFO, "mknod");

	return 0;
}

int fuse_iview_open(const char *path, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "open");

	return 0;	
}

int fuse_iview_unlink(const char *path)
{
	syslog(LOG_INFO, "unlink");

	return 0;
}

int fuse_iview_rmdir(const char *path)
{
	syslog(LOG_INFO, "rmdir %s", path);

	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);
	int status = rmdir(fullPath);

	if (status)
		return -errno;

	return status;
}

int fuse_iview_symlink(const char *path, const char *link)
{
	syslog(LOG_INFO, "symlink");

	return 0;
}

int fuse_iview_rename(const char *fromPath, const char *toPath)
{
	syslog(LOG_INFO, "rename");

	return 0;
}

int fuse_iview_link(const char *path, const char *link)
{
	syslog(LOG_INFO, "link");

	return 0;
}

int fuse_iview_chmod(const char *path, mode_t mode)
{
	syslog(LOG_INFO, "chmod %s", path);

	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);

	int status = chmod(fullPath, mode);

	if (status)
		return -errno;

	return status;
}

int fuse_iview_chown(const char *path, uid_t uid, gid_t gid)
{
	syslog(LOG_INFO, "chown: %s", path);

	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);

	int status = chown(fullPath, uid, gid);

	if (status)
		return -errno;

	return status;
}

int fuse_iview_create(const char *path, mode_t mode, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "create %s", path);

	return 0;
}

int fuse_iview_truncate(const char *path, off_t offset)
{
	syslog(LOG_INFO, "truncate %s", path);
	
	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);
	int status = truncate(fullPath, offset);

	if (status)
		return -errno;

	return status;
}

int fuse_iview_utime(const char *path, struct utimbuf *buffer)
{
	syslog(LOG_INFO, "utime");

	return 0;
}

int fuse_iview_mkdir(const char *path, mode_t mode)
{
	syslog(LOG_INFO, "mkdir: %s", path);

	char *fullPath = strjoin(((struct Cache *)IVIEW_DATA)->rootDir, path);

	int status = mkdir(fullPath, mode);

	syslog(LOG_INFO, "mkdir fullpath: %s", fullPath);

	syslog(LOG_INFO, "mkdir: status %i %s", status, strerror(errno));

	if (status)
		return -errno;

	return 0;
}


int fuse_iview_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "read");

	return 0;
}

int fuse_iview_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "write");

	return 0;
}

int fuse_iview_statfs(const char *path, struct statvfs *stat)
{
	syslog(LOG_INFO, "statfs");

	return 0;
}

int fuse_iview_flush(const char *path, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "flush");

	return 0;
}

int fuse_iview_release(const char *path, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "release");

	return 0;
}

int fuse_iview_fsync(const char *path, int x, struct fuse_file_info *info)
{
	syslog(LOG_INFO, "fsync");

	return 0;
}

int fuse_iview_setxattr(const char *path, const char *attr, const char *value, size_t size, int x)
{
	syslog(LOG_INFO, "setxattr");

	return 0;
}

int fuse_iview_getxattr(const char *path, const char *attr, char *value, size_t size)
{
	syslog(LOG_INFO, "getxattr");

	return 0;
}

int fuse_iview_listxattr(const char *path, char *attr, size_t size)
{
	syslog(LOG_INFO, "listxattr");

	return 0;
}

int fuse_iview_removexattr(const char *path, const char *attr)
{
	syslog(LOG_INFO, "removexattr");

	return 0;
}

void fuse_iview_destroy(void *privateData)
{
	struct Cache *cache = (struct Cache *)privateData;

	nftw(((struct Cache *)IVIEW_DATA)->rootDir, ftw_remove, 64, FTW_DEPTH | FTW_PHYS);

	free(cache);

	cache = NULL;
}

int ftw_remove(const char *path, const struct stat *pathStat, int flags, struct FTW *buffer)
{
	return remove(path);
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

char *fuse_iview_generate_root_dir()
{
	int status = mkdir(FUSE_IVIEW_ROOT_PREFIX, FUSE_ROOT_DIR_MODE);

	if (!status)
		return (char *)FUSE_IVIEW_ROOT_PREFIX;

	char *path = malloc(strlen(FUSE_IVIEW_ROOT_PREFIX) + sizeof(int) + 2);

	strcpy(path, FUSE_IVIEW_ROOT_PREFIX);
	strcat(path, "_");

	path[strlen(path)] = '\0';

	// TODO: Check for permissions error or we will spin around forever
	while (status)
	{
		if (errno != EEXIST)
			path = NULL;

			break;

		for (unsigned int i = strlen(FUSE_IVIEW_ROOT_PREFIX) + 1; i < strlen(FUSE_IVIEW_ROOT_PREFIX) + sizeof(int) + 1; i++)
			path[i] = rand() % 26 + 65;

		status = mkdir(path, FUSE_ROOT_DIR_MODE);
	}

	return path;
}

char *fuse_get_iview_program_name_from_path(const char *path)
{
	if (strlen(path) <= strlen(FUSE_SERIES_ROOT))
		return NULL;

	char *programName = NULL;

	for (unsigned int i = strlen(FUSE_SERIES_ROOT); i < strlen(path); i++)
	{
		if (path[i] == '/')
		{
			programName = (char *)(path + i + 1);

			break;
		}
	}

	if (!programName)
		return NULL;

	for (unsigned int i = 0; i < strlen(programName); i++)
		if (programName[i] == '/')
			return NULL;

	return programName;
}

char *fuse_get_iview_series_name_from_path(const char *path)
{
	if (strlen(path) <= strlen(FUSE_SERIES_ROOT))
		return NULL;

	for (unsigned int i = strlen(FUSE_SERIES_ROOT); i < strlen(path); i++)
		if (path[i] == '/')
			return NULL;

	return (char *)(path + strlen(FUSE_SERIES_ROOT));
}

int main(int argc, char *argv[])
{
	struct Cache *cache = iview_cache_new();

	config_fetch(cache);

	return fuse_main(argc, argv, &fuse_iview_operations, cache);

	/*struct IviewSeries *series;

	signal(SIGINT, handle_sigint);

	openlog("auntie", 0, 0);

	syslog(LOG_INFO, "Opened log.");

	while (TRUE)
	{

	}

	syslog(LOG_INFO, "Closed log.");

	closelog();

	return 0;

	config_fetch(&cache);
	index_fetch(&cache);
	categories_fetch(&cache);

	while (TRUE)
	{
		series = get_series(&cache);

		if (!series)
			break;

		// Check if programs have been loaded into this series yet.
		if (!series->program)
		{
			series->program = get_programs(&cache, series);

			struct IviewProgram *program = select_program(series);

			download_program(&cache, program);
		}

		break;
	}*/

	return 0;
}
