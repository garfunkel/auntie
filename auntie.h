#ifndef AUNTIE_H
#define AUNTIE_H

#define _GNU_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/statfs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include <time.h>

#include <librtmp/rtmp.h>
#include <jansson.h>
#include <libxml/globals.h>
#include <libxml/xmlreader.h>

#include "http.h"

#define TRUE 1
#define FALSE 0

typedef char bool;

extern const unsigned short IVIEW_PORT;
extern const char *ABC_MAIN_URL;
extern const char *IVIEW_CONFIG_URL;
extern const char *HTTP_HEADER_TRANSFER_ENCODING_CHUNKED;
extern const char *IVIEW_CONFIG_API_SERIES_INDEX;
extern const char *IVIEW_CONFIG_API_SERIES;
extern const char *IVIEW_CONFIG_TIME_FORMAT;
extern const char *IVIEW_CATEGORIES_URL;
extern const size_t IVIEW_SERIES_ID_LENGTH;
extern const size_t IVIEW_PROGRAM_ID_LENGTH;
extern const ssize_t IVIEW_RTMP_BUFFER_SIZE;
extern const int IVIEW_REFRESH_INTERVAL;
extern const char *IVIEW_DOWNLOAD_EXT;
extern const char *IVIEW_SWF_URL;
extern const uint32_t IVIEW_SWF_SIZE;
extern const uint8_t IVIEW_SWF_HASH[];
extern const uint16_t IVIEW_RTMP_PORT;
extern const unsigned int BYTES_IN_MEGABYTE;
extern const char *IVIEW_RTMP_AKAMAI_PROTOCOL;
extern const char *IVIEW_RTMP_AKAMAI_HOST;
extern const char *IVIEW_RTMP_AKAMAI_APP_PREFIX;
extern const char *IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX;

struct IviewConfig
{
	unsigned char *api;
	unsigned char *auth;
	unsigned char *tray;
	unsigned char *categories;
	unsigned char *classifications;
	unsigned char *captions;
	short captionsOffset;
	short captionsLiveOffset;
	bool liveStreaming;
	unsigned char *serverStreaming;
	unsigned char *serverFallback;
	unsigned char *highlights;
	unsigned char *home;
	unsigned char *geo;
	unsigned char *time;
	unsigned char *feedbackUrl;
};

struct IviewAuth
{
	char *ip;
	char *isp;
	char *desc;
	char *host;
	char *server;
	char *bwTest;
	char *token;
	char *text;
	bool free;
};

struct IviewKeyword
{
	char *text;
	struct IviewKeyword *next;
};

struct IviewProgram
{
	unsigned int id;
	char *name;
	char *desc;
	char *uri;
	char *transmissionTime;
	char *iviewExpiry;
	size_t size;
	struct IviewProgram *next;
};

struct IviewSeries
{
	unsigned int id;
	char *name;
	char *desc;
	char *image;
	struct IviewKeyword *keyword;
	struct IviewProgram *program;
	struct IviewSeries *next;
};

struct IviewCategories
{

};

struct Cache
{
	struct IviewConfig *config;
	struct IviewAuth *auth;
	struct IviewSeries *index;
	struct IviewCategories *categories;
	struct tm *lastRefresh;
};

struct Cache *iview_cache_new();
void config_fetch(struct Cache *cache);
char *strjoin(const char *first, const char *second);
char *strcreplace(const char *str, const char from, const char to);
bool iview_cache_index_needs_refresh(const struct Cache *cache);
void iview_cache_index_refresh(struct Cache *cache);
void get_programs(const struct Cache *cache, struct IviewSeries *series);
unsigned char *iview_filename_encode(const unsigned char *fileName);
unsigned char *iview_filename_decode(const unsigned char *fileName);
RTMP *download_program_open(struct Cache *cache, const struct IviewProgram *program);
int download_program_read(RTMP *rtmp, char *buffer, size_t size, off_t offset);
int download_program_close(RTMP *rtmp);

void iview_cache_config_free(struct IviewConfig *config);
void iview_cache_auth_free(struct IviewAuth *auth);
void iview_cache_index_free(struct IviewSeries *series);
void iview_cache_program_free(struct IviewProgram *program);
void iview_cache_keyword_free(struct IviewKeyword *keyword);
void iview_cache_categories_free(struct IviewCategories *categories);

void free_null(void *ptr);

#endif // AUNTIE_H