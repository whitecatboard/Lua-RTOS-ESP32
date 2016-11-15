#include <syscalls/syscalls.h>

#include <spiffs.h>
#include <esp_spiffs.h>
#include <spiffs_nucleus.h>

#include <sys/dirent.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

extern spiffs fs;
extern int spiffs_result(int res);

int _getdents_spiffs(struct file *fp, void *buff, int size) {
	int res = 0;

	// Open directory, if needed
	if (!fp->f_dir) {
	    fp->f_dir = malloc(sizeof(spiffs_DIR));
	    if (!fp->f_dir) {
	        return ENOMEM;
	    }

	    if (!SPIFFS_opendir(&fs, fp->f_path, fp->f_dir)) {
	        free(fp->f_dir);

	        res = spiffs_result(fs.err_code);
	        if (res < 0) {
	        	errno = res;
	        	return -1;
	        }
	    }
	}

	struct dirent ent;
	struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;
    char *fn;
    int len = 0;
    int entries = 0;

    ent.d_name[0] = '\0';
    for(;;) {
        // Read directory
        pe = SPIFFS_readdir((spiffs_DIR *)fp->f_dir, pe);
        if (!pe) {
            res = spiffs_result(fs.err_code);
            break;
        }

        // Break condition
        if (pe->name[0] == 0) break;

        // Get name and length
        fn = (char *)pe->name;
        len = strlen(fn);

        // Get entry type and size
        ent.d_type = DT_REG;
        ent.d_reclen = sizeof(struct dirent);
        ent.d_ino = 1;

        if (len >= 2) {
            if (fn[len - 1] == '.') {
                if (fn[len - 2] == '/') {
                    ent.d_type = DT_DIR;

                    fn[len - 2] = '\0';

                    len = strlen(fn);

                    // Skip root dir
                    if (len == 0) {
                        continue;
                    }
                }
            }
        }

        // Skip entries not belonged to path
        if (strncmp(fn, fp->f_path, strlen(fp->f_path)) != 0) {
            continue;
        }

        if (strlen(fp->f_path) > 1) {
            if (*(fn + strlen(fp->f_path)) != '/') {
                continue;
            }
        }

        // Skip root directory
        fn = fn + strlen(fp->f_path);
        len = strlen(fn);
        if (len == 0) {
            continue;
        }

        // Skip initial /
        if (len > 1) {
            if (*fn == '/') {
                fn = fn + 1;
                len--;
            }
        }

        // Skip subdirectories
        if (strchr(fn,'/')) {
            continue;
        }

        ent.d_namlen = len;

        strncpy(ent.d_name, fn, 255);

        entries++;

        break;
    }

    if (entries) {
    	memcpy(buff, &ent, sizeof(struct dirent));
    	return entries * sizeof(struct dirent);
    } else {
    	return 0;
    }
}

int _getdents_fat(struct file *fp, void *buff, int size) {
	return 0;
}

int getdents (int fd, void *buff, int size) {
	struct file *fp;

	// Get file from file descriptor
	if (!(fp = get_file(fd))) {
		errno = EBADF;
		return -1;
	}

	if (fp->f_fs_type == FS_SPIFFS) {
		return _getdents_spiffs(fp, buff, size);
	} else if (fp->f_fs_type == FS_FAT) {
		return _getdents_fat(fp, buff, size);
	} else {
		return -1;
	}
}
