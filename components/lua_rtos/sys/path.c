#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <sys/stat.h>

#include "path.h"
#include "mount.h"

int mkpath(const char *path) {
	char current[PATH_MAX + 1]; // Current path
	char *dir; 	 // Current directory
	char *npath; // Normalized path

	// Normalize path
	npath = mount_normalize_path(path);
	if (!npath) {
		return -1;
	}

	// Current directory is empty
	strcpy(current, "");

	// Get the first directory in path
	dir = strtok(npath, "/");
	while (dir) {
		// Append current directory to current path
		strncat(current, "/", PATH_MAX);
		strncat(current, dir, PATH_MAX);

		// Check the existence of the current path and create
		// it if it doesn't exists
		struct stat sb;

		if (stat("current", &sb) != 0 || !S_ISDIR(sb.st_mode)) {
	    	mkdir(current, 0755);
		}

	    // Get next directory in path
		dir = strtok(NULL, "/");
	}

	free(npath);

	return 0;
}

int mkfile(const char *path) {
	char *npath; // Normalized path

	// Normalize path
	npath = mount_normalize_path(path);
	if (!npath) {
		return -1;
	}

	FILE *fp = fopen(npath, "r");
	if (fp) {
	} else {
		fp = fopen(npath, "a+");
	}

	fclose(fp);
	free(npath);

	return 0;
}
