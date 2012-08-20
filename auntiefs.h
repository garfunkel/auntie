#ifndef AUNTIEFS_H
#define AUNTIEFS_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <fuse/fuse.h>

extern const char FUSE_SERIES_ROOT[];
extern const int FUSE_ROOT_DIR_MODE;
extern const int FUSE_SERIES_DIR_MODE;
extern const blksize_t FUSE_IVIEW_BLOCK_SIZE;
extern const blkcnt_t FUSE_IVIEW_BLOCK_COUNT;

extern struct fuse_operations fuse_iview_operations;

typedef struct RtmpSession
{
	pthread_mutex_t rtmp_read_lock;
	RTMP *rtmp;
} RtmpSession;

RtmpSession *rtmp_session_new();
void rtmp_session_free(RtmpSession *session);

void *fuse_iview_init(struct fuse_conn_info *conn);
int fuse_iview_getattr(const char *path, struct stat *attrStat);
int fuse_iview_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info);
int fuse_iview_opendir(const char *path, struct fuse_file_info *info);
int fuse_iview_open(const char *path, struct fuse_file_info *info);
int fuse_iview_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info);
int fuse_iview_statfs(const char *path, struct statvfs *stat);
int fuse_iview_release(const char *path, struct fuse_file_info *info);
void fuse_iview_destroy(void *privateData);

char *fuse_get_iview_series_name_from_path(const char *path);
char *fuse_get_iview_program_name_from_path(const char *path);

#define IVIEW_DATA (IviewCache *)fuse_get_context()->private_data

#endif // AUNTIEFS_H
