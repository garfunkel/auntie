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

#define TRUE 1
#define FALSE 0

typedef char bool;
typedef struct RTMP RTMP;

extern const uint16_t IVIEW_PORT;
extern const char ABC_MAIN_URL[];
extern const char IVIEW_CONFIG_URL[];
extern const char HTTP_HEADER_TRANSFER_ENCODING_CHUNKED[];
extern const char IVIEW_CONFIG_API_SERIES_INDEX[];
extern const char IVIEW_CONFIG_API_SERIES[];
extern const char IVIEW_CONFIG_TIME_FORMAT[];
extern const char IVIEW_PROGRAM_TIME_FORMAT[];
extern const char IVIEW_CATEGORIES_URL[];
extern const size_t IVIEW_SERIES_ID_LENGTH;
extern const size_t IVIEW_PROGRAM_ID_LENGTH;
extern const ssize_t IVIEW_RTMP_BUFFER_SIZE;
extern const int IVIEW_REFRESH_INTERVAL;
extern const char IVIEW_DOWNLOAD_EXT[];
extern const char IVIEW_SWF_URL[];
extern const uint32_t IVIEW_SWF_SIZE;
extern const uint8_t IVIEW_SWF_HASH[];
extern const uint16_t IVIEW_RTMP_PORT;
extern const unsigned int BYTES_IN_MEGABYTE;
extern const char IVIEW_RTMP_AKAMAI_PROTOCOL[];
extern const char IVIEW_RTMP_AKAMAI_HOST[];
extern const char IVIEW_RTMP_AKAMAI_APP_PREFIX[];
extern const char IVIEW_RTMP_AKAMAI_PLAYPATH_PREFIX[];

typedef struct IviewConfig
{
	char *api;
	char *auth;
	char *tray;
	char *categories;
	char *classifications;
	char *captions;
	short captionsOffset;
	short captionsLiveOffset;
	bool liveStreaming;
	char *serverStreaming;
	char *serverFallback;
	char *highlights;
	char *home;
	char *geo;
	char *time;
	char *feedbackUrl;
} IviewConfig;

typedef struct IviewAuth
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
} IviewAuth;

typedef struct IviewKeyword
{
	char *text;
	struct IviewKeyword *next;
} IviewKeyword;

typedef struct IviewProgram
{
	unsigned int id;
	char *name;
	char *desc;
	char *uri;
	struct tm *transmissionTime;
	struct tm *iviewExpiry;
	struct tm *unknownHValue;
	size_t size;
	struct IviewProgram *next;
} IviewProgram;

typedef struct IviewSeries
{
	unsigned int id;
	char *name;
	char *desc;
	char *image;
	struct IviewKeyword *keyword;
	struct IviewProgram *program;
	struct IviewSeries *next;
} IviewSeries;

typedef struct IviewCategories
{

} IviewCategories;

typedef struct IviewCache
{
	struct IviewConfig *config;
	struct IviewAuth *auth;
	struct IviewSeries *index;
	struct IviewCategories *categories;
	struct tm *lastRefresh;
} IviewCache;

void config_fetch(IviewCache *cache);
IviewProgram *series_parse(IviewSeries *series, const char *json);
char *strjoin(const char *first, const char *second);
char *strcreplace(const char *str, const char from, const char to);
void get_programs(const IviewCache *cache, IviewSeries *series);
unsigned char *iview_filename_encode(const unsigned char *fileName);
unsigned char *iview_filename_decode(const unsigned char *fileName);
RTMP *download_program_open(IviewCache *cache, const IviewProgram *program);
int download_program_read(RTMP *rtmp, char *buffer, size_t size, off_t offset);
int download_program_close(RTMP *rtmp);

IviewCache *iview_cache_new();
IviewConfig *iview_config_new();
IviewAuth *iview_auth_new();
IviewSeries *iview_series_new();
IviewProgram *iview_program_new();
IviewKeyword *iview_keyword_new();
bool iview_cache_index_needs_refresh(const IviewCache *cache);
void iview_cache_index_refresh(IviewCache *cache);
void iview_cache_free(IviewCache *cache);
void iview_cache_config_free(IviewConfig *config);
void iview_cache_auth_free(IviewAuth *auth);
void iview_cache_index_free(IviewSeries *series);
void iview_cache_program_free(IviewProgram *program);
void iview_cache_keyword_free(IviewKeyword *keyword);
void iview_cache_categories_free(IviewCategories *categories);

IviewSeries *iview_get_series(const IviewCache *cache, const char *seriesName);
IviewProgram *iview_get_program(const IviewSeries *series, const char *programName);

#endif // AUNTIE_H