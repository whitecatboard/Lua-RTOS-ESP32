/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, a tool for make a ROM file system image
 *
 */

#include "romfs.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>

static romfs_config_t cfg;
static romfs_t fs;
static uint8_t *data;

static void create_dir(char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

		if ((ret = romfs_mkdir(&fs, path)) < 0) {
			fprintf(stderr,"can't create directory %s: error=%d\r\n", path, ret);
			exit(1);
		}
	}
}

static void create_file(char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

        // Open source file
        FILE *srcf = fopen(src,"r");
        if (!srcf) {
            fprintf(stderr,"can't open source file %s: errno=%d (%s)\r\n", src, errno, strerror(errno));
            exit(1);
        }

        // Open destination file
        romfs_file_t dstf;
        if ((ret = romfs_file_open(&fs, &dstf, path, ROMFS_O_WRONLY | ROMFS_O_CREAT)) < 0) {
            fprintf(stderr,"can't open destination file %s: error=%d\r\n", path, ret);
            exit(1);
        }

        char c = fgetc(srcf);
		while (!feof(srcf)) {
			ret = romfs_file_write(&fs, &dstf, &c, 1);
			if (ret < 0) {
				fprintf(stderr,"can't write to destination file %s: error=%d\r\n", path, ret);
	            exit(1);
			}
			c = fgetc(srcf);
		}

		// Close destination file
		ret = romfs_file_close(&fs, &dstf);
		if (ret < 0) {
			fprintf(stderr,"can't close destination file %s: error=%d\r\n", path, ret);
			exit(1);
		}

        // Close source file
        fclose(srcf);
    }
}

static void compact(char *src) {
    DIR *dir;
    struct dirent *ent;
    char curr_path[PATH_MAX];

    dir = opendir(src);
    if (dir) {
        while ((ent = readdir(dir))) {
            // Skip . and .. directories
            if ((strcmp(ent->d_name,".") != 0) && (strcmp(ent->d_name,"..") != 0)) {
                // Update the current path
                strcpy(curr_path, src);
                strcat(curr_path, "/");
                strcat(curr_path, ent->d_name);

                if (ent->d_type == DT_DIR) {
                    create_dir(curr_path);
                    compact(curr_path);
                } else if (ent->d_type == DT_REG) {
                    create_file(curr_path);
                }
            }
        }

        closedir(dir);
    }
}

void usage() {
	fprintf(stdout, "usage: mkromfs -c <pack-dir> -i <image-file-path>\r\n");
}

int main(int argc, char **argv) {
    char *src = NULL;   // Source directory
    char *dst = NULL;   // Destination image
    int c;              // Current option
    int fs_size;        // File system size
    int err;

    fs_size = 4 * 1024 * 1024;

	while ((c = getopt(argc, argv, "c:i:s:")) != -1) {
		switch (c) {
        case 'c':
            src = optarg;
            break;

        case 'i':
			dst = optarg;
			break;
		}
	}

    if ((src == NULL) || (dst == NULL)) {
    		usage();
        exit(1);
    }

    // Mount the file system
    cfg.size = fs_size;

	data = calloc(1, fs_size);
	if (!data) {
		fprintf(stderr, "no memory for mount\r\n");
		return -1;
	}

	cfg.base = data;

	err = romfs_mount(&fs, &cfg);
	if (err < 0) {
		fprintf(stderr, "mount error: error=%d\r\n", err);
		return -1;
	}

	chdir(src);
	compact(".");

	FILE *img;

	img = fopen(dst, "w+");
	if (!img) {
		fprintf(stderr, "can't create image file: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	int total_size = fs.current_size;

	err = romfs_umount(&fs);
	if (err < 0) {
		fprintf(stderr, "unmount error: error=%d\r\n", err);
		return -1;
	}

	fwrite(data, 1, total_size, img);

	fprintf(stdout,"ROMFS size: %d bytes\r\n", total_size);


	fclose(img);

	return 0;
}
