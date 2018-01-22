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
 * Lua RTOS,  http lua page preprocessor
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_HTTP_SERVER

#include <stdio.h>

#include <sys/syslog.h>

int http_preprocess_lua_page(const char *ipath, const char *opath) {
    FILE *ifp; // Input file
    FILE *ofp; // Output file

    int c;
    int nested = 0;
    int print = 0;
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

		if (c == *cbt) {
			nested++;

			cbt++;
			if (!*cbt) {
				lua = 1;
				add_cr = 1;
				cbuff = buff;

				if (nested > 1) {
					if (print) {
						fprintf(ofp, "\")\n");
						io_write = 1;
					}
				}
			} else {
				*cbuff++ = c;
			}

			*cbuff = '\0';
			continue;
		} else if (c == *cet) {
			nested--;

			cet++;
			if (!*cet) {
				lua = 0;
				add_cr = 1;

				if (nested > 0) {
					if (print) {
						fprintf(ofp, "\nprint(\"");
						io_write = 1;
					}
				}

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
						print = 0;
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
						print = 1;
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
						print = 0;
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
						print = 1;
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

#endif
