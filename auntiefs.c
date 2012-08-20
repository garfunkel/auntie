#include "auntie.h"
#include "auntiefs.h"
#include "utils.h"

#include <errno.h>
#include <pthread.h>
#include <fuse/fuse.h>

const char FUSE_SERIES_ROOT[] = "/";
const int FUSE_ROOT_DIR_MODE = 0754;
const int FUSE_SERIES_DIR_MODE = 0754;
const blksize_t FUSE_IVIEW_BLOCK_SIZE = 4096;
const blkcnt_t FUSE_IVIEW_BLOCK_COUNT = 1;

struct fuse_operations fuse_iview_operations =
{
	.init = fuse_iview_init,
	.getattr = fuse_iview_getattr,
	.readdir = fuse_iview_readdir,
	.opendir = fuse_iview_opendir,
	.open = fuse_iview_open,
	.read = fuse_iview_read,
	.statfs = fuse_iview_statfs,
	.release = fuse_iview_release,
	.destroy = fuse_iview_destroy,
};

RtmpSession *rtmp_session_new()
{
	RtmpSession *session = malloc(sizeof(RtmpSession));

	pthread_mutex_init(&session->rtmp_read_lock, NULL);
	
	session->rtmp = NULL;

	return session;
}

void rtmp_session_free(RtmpSession *session)
{
	pthread_mutex_destroy(&session->rtmp_read_lock);

	session->rtmp = NULL;

	free_null(session);
}

void *fuse_iview_init(struct fuse_conn_info *conn)
{
	IviewCache *cache = iview_cache_new();

	config_fetch(cache);

	return cache;
}

int fuse_iview_getattr(const char *path, struct stat *attrStat)
{
	char *programName = fuse_get_iview_program_name_from_path(path);
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	char *decodedProgramName = (char *)iview_filename_decode((unsigned char *)programName);
	char *decodedSeriesName = (char *)iview_filename_decode((unsigned char *)seriesName);
	int status = 0;
	IviewCache *cache = IVIEW_DATA;

	if (decodedProgramName)
	{
		IviewSeries *series = iview_get_series(cache, decodedSeriesName);
		
		if (series)
		{
			if (!series->program)
				get_programs(cache, series);

			IviewProgram *program = iview_get_program(series, decodedProgramName);

			if (program)
			{
				attrStat->st_mode = S_IFREG | 0754;
				attrStat->st_nlink = 1;
				attrStat->st_uid = getuid();
				attrStat->st_gid = getgid();
				attrStat->st_size = program->size;
				attrStat->st_atime = time(NULL);
				attrStat->st_mtime = mktime(cache->lastRefresh);
			}

			else
				status = -ENOENT;
		}

		else
			status = -ENOENT;
	}

	else if (decodedSeriesName)
	{
		IviewSeries *series = iview_get_series(cache, decodedSeriesName);

		if (series)
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
		}

		else
			status = -ENOENT;
	}

	else
	{
		status = lstat(path, attrStat);

		if (status)
			return -ENOENT;
	}

	free_null(programName);
	free_null(decodedProgramName);
	free_null(seriesName);
	free_null(decodedSeriesName);

	return status;
}

int fuse_iview_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
{
	IviewCache *cache = IVIEW_DATA;

	if (iview_cache_index_needs_refresh(cache))
		iview_cache_index_refresh(cache);

	char *seriesName = fuse_get_iview_series_name_from_path(path);

	if (!strcmp(path, FUSE_SERIES_ROOT))
	{
		IviewSeries *series = cache->index;
		char *seriesName = NULL;
		char *encodedSeriesName = NULL;

		while (series)
		{
			seriesName = strcreplace(series->name, '/', '-');
			encodedSeriesName = (char *)iview_filename_encode((unsigned char *)seriesName);

			filler(buffer, encodedSeriesName, NULL, 0);

			free_null(seriesName);
			free_null(encodedSeriesName);

			series = series->next;
		}
	}

	else if (seriesName)
	{
		IviewSeries *series = iview_get_series(cache, seriesName);

		if (series)
		{
			if (!series->program)
				get_programs(cache, series);

			IviewProgram *program = series->program;
			char *programName = NULL;
			char *encodedProgramName = NULL;

			while (program)
			{
				programName = strjoin(program->name, IVIEW_DOWNLOAD_EXT);
				encodedProgramName = (char *)iview_filename_encode((unsigned char *)programName);

				filler(buffer, encodedProgramName, NULL, 0);

				free_null(programName);
				free_null(encodedProgramName);

				program = program->next;
			}
		}
	}

	free_null(seriesName);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	return 0;
}

int fuse_iview_opendir(const char *path, struct fuse_file_info *info)
{
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	int status = 0;

	// Directory only valid if root, or series.
	if (strcmp(path, "/") && !seriesName)
		status = -ENOENT;

	free_null(seriesName);

	return status;
}

int fuse_iview_open(const char *path, struct fuse_file_info *info)
{
	char *programName = fuse_get_iview_program_name_from_path(path);
	char *seriesName = fuse_get_iview_series_name_from_path(path);
	char *decodedProgramName = (char *)iview_filename_decode((unsigned char *)programName);
	char *decodedSeriesName = (char *)iview_filename_decode((unsigned char *)seriesName);
	int status = 0;
	IviewCache *cache = IVIEW_DATA;

	if (decodedProgramName)
	{
		IviewSeries *series = iview_get_series(cache, decodedSeriesName);

		if (series)
		{
			if (!series->program)
				get_programs(cache, series);

			IviewProgram *program = iview_get_program(series, decodedProgramName);

			if (program)
			{
				RtmpSession *rtmpSession = rtmp_session_new();
				
				rtmpSession->rtmp = download_program_open(cache, program);
						
				info->fh = (uint64_t)rtmpSession;
				info->nonseekable = TRUE;
			}

			else
				status = -ENOENT;
		}

		else
			status = -ENOENT;
	}

	free_null(programName);
	free_null(seriesName);
	free_null(decodedProgramName);
	free_null(decodedSeriesName);

	return status;
}

int fuse_iview_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
	RtmpSession *rtmpSession = (RtmpSession *)info->fh;

	pthread_mutex_lock(&rtmpSession->rtmp_read_lock);
	
	int numRead = download_program_read(rtmpSession->rtmp, buffer, size, offset);
	
	pthread_mutex_unlock(&rtmpSession->rtmp_read_lock);

	return numRead;
}

int fuse_iview_statfs(const char *path, struct statvfs *stat)
{
	return 0;
}

int fuse_iview_release(const char *path, struct fuse_file_info *info)
{
	RtmpSession *session = (RtmpSession *)info->fh;

	download_program_close(session->rtmp);

	rtmp_session_free(session);

	return 0;
}

void fuse_iview_destroy(void *privateData)
{
	iview_cache_free((IviewCache *)privateData);
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

	if (!strcmp(programName + strlen(programName) - strlen(IVIEW_DOWNLOAD_EXT), IVIEW_DOWNLOAD_EXT))
		return strndup(programName, strlen(programName) - strlen(IVIEW_DOWNLOAD_EXT));

	return NULL;
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
		return strdup(path + strlen(FUSE_SERIES_ROOT));

	return strndup((char *)(path + strlen(FUSE_SERIES_ROOT)), seriesEnd);
}
