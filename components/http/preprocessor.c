/*
 * Lua RTOS, http lua page preprocessor
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include <stdio.h>

int http_process_lua_page(const char *ipath, const char *opath) {
    FILE *ifp; // Input file
    FILE *ofp; // Output file

    int c;
    char string;
    char lua = 0;
    char delim;
	char io_write = 0;
	char add_cr = 0;
    char buff[6];

    const char *bt = "<?lua";
    const char *et = "?>";

    const char *cbt = bt;
    const char *cet = et;
    char *cbuff = buff;

    // Open input file
    ifp = fopen(ipath,"r");
    if (!ifp) {
        return -1;
    }

    // Open output file
    ofp = fopen(opath,"w+");
    if (!ofp) {
		fclose(ifp);

        return -1;
    }

	string = 0;
	*cbuff = '\0';

	fprintf(ofp, "do\n");
	fprintf(ofp, "local print = net.service.http.print_chunk\n");

    while((c = fgetc(ifp)) != EOF) {
    	if (c == '"') {
    		if (!string) {
    			delim = '\"';
    			string = 1;
    		} else {
    			if (c == delim) {
    				string = 0;
    			}
    		}
    	} else if (c == '\'') {
    		if (!string) {
    			delim = '\'';
    			string = 1;
    		} else {
    			if (c == delim) {
    				string = 0;
    			}
    		}
    	}

		if ((c == *cbt) && (!string)) {
			cbt++;
			if (!*cbt) {
				lua = 1;
				add_cr = 1;
				cbuff = buff;
			} else {
				*cbuff++ = c;
			}

			*cbuff = '\0';
			continue;
		} else if ((c == *cet) && (!string)) {
			cet++;
			if (!*cet) {
				lua = 0;
				add_cr = 1;
				cbuff = buff;
			} else {
				*cbuff++ = c;
			}

			*cbuff = '\0';
			continue;
		} else {
			cbt = bt;
			cet = et;

			cbuff = buff;

			while (*cbuff) {
				if (c == '\n') {
					if (io_write) {
						fprintf(ofp, "\")\n");
						io_write = 0;
					}
				} else if (c == '\r') {
					continue;
				} else if (c == '\"') {
					fprintf(ofp, "\\%c",c);
				} else {
					if (!io_write) {
						if (add_cr) {
							fprintf(ofp, "\n");
							add_cr = 0;
						}
						fprintf(ofp, "print(\"");
						io_write = 1;
					}
					fprintf(ofp, "%c",*cbuff);
				}

				cbuff++;
			}

			if (!lua) {
				if (c == '\n') {
					if (io_write) {
						fprintf(ofp, "\")\n");
						io_write = 0;
					}
				} else if (c == '\r') {
					continue;
				} else if (c == '\"') {
					fprintf(ofp, "\\%c",c);
				} else {
					if (!io_write) {
						if (add_cr) {
							fprintf(ofp, "\n");
							add_cr = 0;
						}
						fprintf(ofp, "print(\"");
						io_write = 1;
					}
					fprintf(ofp, "%c",c);
				}
			} else {
				fprintf(ofp, "%c",c);
			}

			cbuff = buff;
			*cbuff = '\0';
		}
    }

    if (io_write) {
		fprintf(ofp, "\")\n");
    }

	fprintf(ofp, "end");

    fclose(ifp);
    fclose(ofp);

    return 0;
}
