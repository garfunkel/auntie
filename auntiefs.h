#ifndef AUNTIEFS_H
#define AUNTIEFS_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>

#include <fuse/fuse.h>

extern const char *FUSE_SERIES_ROOT;
extern const int FUSE_ROOT_DIR_MODE;
extern const int FUSE_SERIES_DIR_MODE;
extern const blksize_t FUSE_IVIEW_BLOCK_SIZE;
extern const blkcnt_t FUSE_IVIEW_BLOCK_COUNT;

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

extern struct fuse_operations fuse_iview_operations;

char *fuse_get_iview_series_name_from_path(const char *path);
char *fuse_get_iview_program_name_from_path(const char *path);

#define IVIEW_DATA (struct Cache *)fuse_get_context()->private_data

#endif // AUNTIEFS_H