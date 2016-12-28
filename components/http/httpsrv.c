/*******************************************************************************
 * Copyright (c) 2015, http://www.jbox.dk/
 * All rights reserved. Released under the BSD license.
 * httpsrv.c 1.0 01/01/2016 (Simple Http Server)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include "luartos.h"

#if LUA_USE_HTTP

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <pthread/pthread.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syslog.h>

#define PORT           80
#define SERVER         "httpsrv/1.0"
#define PROTOCOL       "HTTP/1.0"
#define RFC1123FMT     "%a, %d %b %Y %H:%M:%S GMT"
#define HTPP_BUFF_SIZE 1024

char *get_mime_type(char *name) {

    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".htm")  == 0) return "text/html";
    if (strcmp(ext, ".txt")  == 0) return "text/html";
    if (strcmp(ext, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif")  == 0) return "image/gif";
    if (strcmp(ext, ".png")  == 0) return "image/png";
    if (strcmp(ext, ".css")  == 0) return "text/css";
    if (strcmp(ext, ".au")   == 0) return "audio/basic";
    if (strcmp(ext, ".wav")  == 0) return "audio/wav";
    if (strcmp(ext, ".avi")  == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mpg")  == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3")  == 0) return "audio/mpeg";
    if (strcmp(ext, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(ext, ".pdf")  == 0) return "application/pdf";
    return NULL;

}

void send_headers(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date) {
    time_t now;
    char timebuf[128];
    fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
    fprintf(f, "Server: %s\r\n", SERVER);
    now = time(NULL);
    strftime(timebuf, sizeof (timebuf), RFC1123FMT, gmtime(&now));
    fprintf(f, "Date: %s\r\n", timebuf);
    if (extra) fprintf(f, "%s\r\n", extra);
    if (mime) fprintf(f, "Content-Type: %s\r\n", mime);
    if (length >= 0) fprintf(f, "Content-Length: %d\r\n", length);
    if (date != -1) {
        strftime(timebuf, sizeof (timebuf), RFC1123FMT, gmtime(&date));
        fprintf(f, "Last-Modified: %s\r\n", timebuf);
    }
    fprintf(f, "Connection: close\r\n");
    fprintf(f, "\r\n");

}

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
    send_headers(f, status, title, extra, "text/html", -1, -1);
    fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
    fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
    fprintf(f, "%s\r\n", text);
    fprintf(f, "</BODY></HTML>\r\n");

}

void send_file(FILE *f, char *path, struct stat *statbuf) {
    int n;
    char data[HTPP_BUFF_SIZE];
    FILE *file = fopen(path, "r");

    if (!file) {
        send_error(f, 403, "Forbidden", NULL, "Access denied.");
    } else {
        int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
        send_headers(f, 200, "OK", NULL, get_mime_type(path), length, statbuf->st_mtime);
        while ((n = fread(data, 1, sizeof (data), file)) > 0) fwrite(data, 1, n, f);
        fclose(file);
    }
}

int process(FILE *f) {
    char buf[HTPP_BUFF_SIZE];
    char *method;
    char *path;
    char *protocol;
    struct stat statbuf;
    char pathbuf[HTPP_BUFF_SIZE];
    int len;

    if (!fgets(buf, sizeof (buf), f)) return -1;

    method = strtok(buf, " ");
    path = strtok(NULL, " ");
    protocol = strtok(NULL, "\r");

    if (!method || !path || !protocol) return -1;

    fseek(f, 0, SEEK_CUR); // Force change of stream direction

    if (strcasecmp(method, "GET") != 0) {
        send_error(f, 501, "Not supported", NULL, "Method is not supported.");
    } else if (stat(path, &statbuf) < 0) {
        send_error(f, 404, "Not Found", NULL, "File not found.");
    } else if (S_ISDIR(statbuf.st_mode)) {
        len = strlen(path);
        if (len == 0 || path[len - 1] != '/') {
            snprintf(pathbuf, sizeof (pathbuf), "Location: %s/", path);
            send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
        } else {
            snprintf(pathbuf, sizeof (pathbuf), "%sindex.html", path);
            if (stat(pathbuf, &statbuf) >= 0) {
                send_file(f, pathbuf, &statbuf);
            } else {
                DIR *dir;
                struct dirent *de;

                send_headers(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
                fprintf(f, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>", path);
                fprintf(f, "<H4>Index of %s</H4>\r\n<PRE>\n", path);
                fprintf(f, "Name                             Last Modified              Size\r\n");
                fprintf(f, "<HR>\r\n");
                if (len > 1) fprintf(f, "<A HREF=\"..\">..</A>\r\n");

                dir = opendir(path);
                while ((de = readdir(dir)) != NULL) {
                    char timebuf[32];
                    struct tm *tm;

                    strcpy(pathbuf, path);
                    strcat(pathbuf, de->d_name);

                    stat(pathbuf, &statbuf);
                    tm = gmtime(&statbuf.st_mtime);
                    strftime(timebuf, sizeof (timebuf), "%d-%b-%Y %H:%M:%S", tm);

                    fprintf(f, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
                    fprintf(f, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
                    if (strlen(de->d_name) < 32) fprintf(f, "%*s", 32 - strlen(de->d_name), "");
                    if (S_ISDIR(statbuf.st_mode)) {
                        fprintf(f, "%s\r\n", timebuf);
                    } else {
                        fprintf(f, "%s %10d\r\n", timebuf, (int)statbuf.st_size);
                    }
                }
                closedir(dir);

                fprintf(f, "</PRE>\r\n<HR>\r\n<ADDRESS>%s</ADDRESS>\r\n</BODY></HTML>\r\n", SERVER);
            }
        }
    } else {
        send_file(f, path, &statbuf);
    }

    return 0;
}

static void *http_thread(void *arg) {
	int sock;
    struct sockaddr_in sin;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port        = htons(PORT);
    bind(sock, (struct sockaddr *) &sin, sizeof (sin));

    listen(sock, 5);
    syslog(LOG_INFO, "http: server listening on port %d\n", PORT);

    while (1) {
        int s;
        FILE *f;
        s = accept(sock, NULL, NULL);
        if (s < 0) break;
        f = fdopen(s, "a+");
        process(f);
        fclose(f);
    }

    close(sock);

    pthread_exit(NULL);
}

void http_start() {
	pthread_attr_t attr;
	struct sched_param sched;
	pthread_t thread;
	int res;

	// Init thread attributes
	pthread_attr_init(&attr);

	// Set stack size
    pthread_attr_setstacksize(&attr, LUA_TASK_STACK);

    // Set priority
    sched.sched_priority = LUA_TASK_PRIORITY;
    pthread_attr_setschedparam(&attr, &sched);

    // Set CPU
    cpu_set_t cpu_set = LUA_TASK_CPU;
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);

    // Create thread
    res = pthread_create(&thread, &attr, http_thread, NULL);
    if (res) {
		panic("Cannot start http_thread");
	}
}

void http_stop() {

}

#endif
