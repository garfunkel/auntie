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

#include <librtmp/rtmp.h>
#include <jansson.h>
#include <libxml/globals.h>
#include <libxml/xmlreader.h>
#include <fuse/fuse.h>

#define HTTP_BUFFER_SIZE (const int)sysconf(_SC_PAGESIZE)
#define TRUE 1
#define FALSE 0

typedef char bool;

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
const char *FUSE_SERIES_ROOT = "/";
const int FUSE_ROOT_DIR_MODE = 0754;
const int FUSE_SERIES_DIR_MODE = 0754;
const int FUSE_IVIEW_REFRESH_INTERVAL = 3600;
const blksize_t FUSE_IVIEW_BLOCK_SIZE = 4096;
const blkcnt_t FUSE_IVIEW_BLOCK_COUNT = 1;
const ssize_t IVIEW_RTMP_BUFFER_SIZE = 8192;

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

void *fuse_iview_init(struct fuse_conn_info *conn);
int fuse_iview_getattr(const char *path, struct stat *attrStat);
int fuse_iview_readlink(const char *path, char *buffer, size_t size);
int fuse_iview_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info);
int fuse_iview_opendir(const char *path, struct fuse_file_info *info);
int fuse_iview_mknod(const char *path, mode_t mode, dev_t dev);
int fuse_iview_mkdir(const char *path, mode_t mode);
int fuse_iview_unlink(const char *path);
int fuse_iview_rmdir(const char *path);
int fuse_iview_symlink(const char *path, const char *link);
int fuse_iview_rename(const char *fromPath, const char *toPath);
int fuse_iview_link(const char *path, const char *link);
int fuse_iview_chmod(const char *path, mode_t mode);
int fuse_iview_chown(const char *path, uid_t uid, gid_t gid);
int fuse_iview_create(const char *path, mode_t mode, struct fuse_file_info *info);
int fuse_iview_truncate(const char *path, off_t offset);
int fuse_iview_utime(const char *path, struct utimbuf *buffer);
int fuse_iview_open(const char *path, struct fuse_file_info *info);
int fuse_iview_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info);
int fuse_iview_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info);
int fuse_iview_statfs(const char *path, struct statvfs *stat);
int fuse_iview_flush(const char *path, struct fuse_file_info *info);
int fuse_iview_release(const char *path, struct fuse_file_info *info);
int fuse_iview_fsync(const char *path, int x, struct fuse_file_info *info);
int fuse_iview_setxattr(const char *path, const char *attr, const char *value, size_t size, int x);
int fuse_iview_getxattr(const char *path, const char *attr, char *value, size_t size);
int fuse_iview_listxattr(const char *path, char *attr, size_t size);
int fuse_iview_removexattr(const char *path, const char *attr);
void fuse_iview_destroy(void *privateData);

struct fuse_operations fuse_iview_operations =
{
	.init = fuse_iview_init,
	.getattr = fuse_iview_getattr,
	.readlink = fuse_iview_readlink,
	.readdir = fuse_iview_readdir,
	.opendir = fuse_iview_opendir,
	.mknod = fuse_iview_mknod,
	.mkdir = fuse_iview_mkdir,
	.unlink = fuse_iview_unlink,
	.rmdir = fuse_iview_rmdir,
	.symlink = fuse_iview_symlink,
	.rename = fuse_iview_rename,
	.link = fuse_iview_link,
	.chmod = fuse_iview_chmod,
	.chown = fuse_iview_chown,
	.create = fuse_iview_create,
	.truncate = fuse_iview_truncate,
	.utime = fuse_iview_utime,
	.open = fuse_iview_open,
	.read = fuse_iview_read,
	.write = fuse_iview_write,
	.statfs = fuse_iview_statfs,
	.flush = fuse_iview_flush,
	.release = fuse_iview_release,
	.fsync = fuse_iview_fsync,
	.setxattr = fuse_iview_setxattr,
	.getxattr = fuse_iview_getxattr,
	.listxattr = fuse_iview_listxattr,
	.removexattr = fuse_iview_removexattr,
	.destroy = fuse_iview_destroy,
};

struct Cache *iview_cache_new();
char *strjoin(const char *first, const char *second);
char *strcreplace(const char *str, const char from, const char to);
bool iview_cache_index_needs_refresh(const struct Cache *cache);
void iview_cache_index_refresh(struct Cache *cache);
char *fuse_get_iview_series_name_from_path(const char *path);
char *fuse_get_iview_program_name_from_path(const char *path);

#define IVIEW_DATA (struct Cache *)fuse_get_context()->private_data

#endif // AUNTIE_H