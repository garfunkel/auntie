#include "auntie.h"
#include "auntiefs.h"

int main(int argc, char *argv[])
{
	struct Cache *cache = iview_cache_new();

	config_fetch(cache);

	//char b[100000];

	//download_program(cache, NULL, b, 100000, 0);

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
