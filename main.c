#include "auntie.h"
#include "auntiefs.h"

#include <fuse/fuse.h>

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &fuse_iview_operations, NULL);
}
