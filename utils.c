#include "utils.h"

void free_null(void *ptr)
{
	free(ptr);

	ptr = NULL;
}
