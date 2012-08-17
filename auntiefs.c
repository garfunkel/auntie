#include "auntie.h"
#include "auntiefs.h"

const char *FUSE_SERIES_ROOT = "/";
const int FUSE_ROOT_DIR_MODE = 0754;
const int FUSE_SERIES_DIR_MODE = 0754;
const blksize_t FUSE_IVIEW_BLOCK_SIZE = 4096;
const blkcnt_t FUSE_IVIEW_BLOCK_COUNT = 1;

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

void *fuse_iview_init(struct fuse_conn_info *conn)
{
	struct Cache *cache = IVIEW_DATA;

	return cache;
}

int fuse_iview_getattr(const char *path, struct stat *attrStat)
{
	syslog(LOG_INFO, "getattr: %s", path);

	char *programName = fuse_get_iview_program_name_from_path(path);
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	char *decodedProgramName = (char *)iview_filename_decode((unsigned char *)programName);
	char *decodedSeriesName = (char *)iview_filename_decode((unsigned char *)seriesName);
	int status = 0;
	struct Cache *cache = IVIEW_DATA;

	if (decodedProgramName)
	{
		struct IviewSeries *series = cache->index;
		bool found = FALSE;

		while (series)
		{
			if (!strcmp(series->name, decodedSeriesName))
			{
				if (!series->program)
					get_programs(cache, series);

				struct IviewProgram *program = series->program;

				while (program)
				{
					if (!strncmp(program->name, decodedProgramName, strlen(program->name)))
					{
						attrStat->st_mode = S_IFREG | 0754;
						attrStat->st_nlink = 1;
						attrStat->st_uid = getuid();
						attrStat->st_gid = getgid();
						attrStat->st_size = program->size;
						attrStat->st_atime = time(NULL);
						attrStat->st_mtime = mktime(cache->lastRefresh);

						found = TRUE;

						break;
					}

					program = program->next;

					if (!program)
					{
						status = -ENOENT;

						break;
					}
				}

				if (found)
					break;
			}

			series = series->next;

			if (!series)
				status = -ENOENT;
		}
	}

	else if (decodedSeriesName)
	{
		struct IviewSeries *series = cache->index;

		while (series)
		{
			if (!strcmp(series->name, decodedSeriesName))
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
		status = lstat(path, attrStat);
		syslog(LOG_INFO, "getattr status %i %s", status, strerror(status));

		if (status)
			return -ENOENT;
	}

	//free_null(programName);
	//free_null(decodedProgramName);
	//free_null(seriesName);
	//free_null(decodedSeriesName);

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
			char *encodedSeriesName = (char *)iview_filename_encode((unsigned char *)seriesName);

			filler(buffer, encodedSeriesName, NULL, 0);

			free_null(seriesName);
			free_null(encodedSeriesName);

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
					char *programName = strjoin(program->name, IVIEW_DOWNLOAD_EXT);
					char *encodedProgramName = (char *)iview_filename_encode((unsigned char *)programName);

					filler(buffer, encodedProgramName, NULL, 0);

					free_null(programName);
					free_null(encodedProgramName);

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
	syslog(LOG_INFO, "open %s", path);

	char *programName = fuse_get_iview_program_name_from_path(path);
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	char *decodedProgramName = (char *)iview_filename_decode((unsigned char *)programName);
	char *decodedSeriesName = (char *)iview_filename_decode((unsigned char *)seriesName);
	int status = 0;
	struct Cache *cache = IVIEW_DATA;

	if (decodedProgramName)
	{
		struct IviewSeries *series = cache->index;
		bool found = FALSE;

		while (series)
		{
			if (!strcmp(series->name, decodedSeriesName))
			{
				if (!series->program)
					get_programs(cache, series);

				struct IviewProgram *program = series->program;

				while (program)
				{
					if (!strncmp(program->name, decodedProgramName, strlen(program->name)))
					{
						info->fh = (uint64_t)download_program_open(cache, program);

						found = TRUE;

						break;
					}

					program = program->next;

					if (!program)
					{
						status = -ENOENT;

						break;
					}
				}

				if (found)
					break;
			}

			series = series->next;

			if (!series)
				status = -ENOENT;
		}
	}

	return status;
}

int fuse_iview_unlink(const char *path)
{
	syslog(LOG_INFO, "unlink");

	return 0;
}

int fuse_iview_rmdir(const char *path)
{
	syslog(LOG_INFO, "rmdir %s", path);

	int status = rmdir(path);

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

	int status = chmod(path, mode);

	if (status)
		return -errno;

	return status;
}

int fuse_iview_chown(const char *path, uid_t uid, gid_t gid)
{
	syslog(LOG_INFO, "chown: %s", path);

	int status = chown(path, uid, gid);

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
	
	int status = truncate(path, offset);

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

	int status = mkdir(path, mode);

	syslog(LOG_INFO, "mkdir fullpath: %s", path);

	syslog(LOG_INFO, "mkdir: status %i %s", status, strerror(errno));

	if (status)
		return -errno;

	return 0;
}

int fuse_iview_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	sleep(1);

	syslog(LOG_INFO, "read %s %i %i", path, (unsigned int)size, (unsigned int)offset);

	return download_program_read((RTMP *)info->fh, buffer, size, offset);
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

	download_program_close((RTMP *)info->fh);

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

	iview_cache_config_free(cache->config);
	iview_cache_auth_free(cache->auth);
	iview_cache_index_free(cache->index);
	iview_cache_categories_free(cache->categories);

	free_null(cache->lastRefresh);
	free_null(cache);

	cache = NULL;
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

	unsigned int seriesEnd = 0;

	for (unsigned int i = strlen(FUSE_SERIES_ROOT); i < strlen(path); i++)
	{
		if (path[i] == '/')
		{
			seriesEnd = i - 1;

			break;
		}
	}

	if (seriesEnd == 0)
		return (char *)(path + strlen(FUSE_SERIES_ROOT));

	return strndup((char *)(path + strlen(FUSE_SERIES_ROOT)), seriesEnd);
}
