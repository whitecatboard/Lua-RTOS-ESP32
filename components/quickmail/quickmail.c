/*
    This file is part of libquickmail.

    libquickmail is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libquickmail is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libquickmail.  If not, see <http://www.gnu.org/licenses/>.

    Adapted for LUA-RTOS_ESP32 by Boris Lovosevic (loboris@gmail.com; https://github.com/loboris)
*/

#include "quickmail.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <curl/curl.h>

#define LIBQUICKMAIL_VERSION_MAJOR 0
#define LIBQUICKMAIL_VERSION_MINOR 1
#define LIBQUICKMAIL_VERSION_MICRO 25

#define VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define VERSION_STRINGIZE(major, minor, micro) VERSION_STRINGIZE_(major, minor, micro)

#define LIBQUICKMAIL_VERSION VERSION_STRINGIZE(LIBQUICKMAIL_VERSION_MAJOR,LIBQUICKMAIL_VERSION_MINOR,LIBQUICKMAIL_VERSION_MICRO)

#define NEWLINE "\r\n"
#define NEWLINELENGTH 2
//#define NEWLINE "\n"
//#define NEWLINELENGTH 1

#define MIME_LINE_WIDTH 992
#define BODY_BUFFER_SIZE 256

//definitions of the different stages of generating the message data
#define MAILPART_INITIALIZE 0
#define MAILPART_HEADER     1
#define MAILPART_BODY       2
#define MAILPART_BODY_DONE  3
#define MAILPART_ATTACHMENT 4
#define MAILPART_END        5
#define MAILPART_DONE       6

static const char* default_mime_type = "text/plain";

struct progress {
  double lastruntime;
  CURL *curl;
};

int quickmail_progress = 2;
int quickmail_verbose = 1;
int quickmail_timeout = 120;

////////////////////////////////////////////////////////////////////////

#define DEBUG_ERROR(errmsg)
static const char* ERRMSG_MEMORY_ALLOCATION_ERROR = "Memory allocation error";

////////////////////////////////////////////////////////////////////////

char* randomize_zeros (char* data)
{
  //replace all 0s with random digits
  char* p = data;
  while (*p) {
    if (*p == '0')
      *p = '0' + rand() % 10;
    p++;
  }
  return data;
}

char* str_append (char** data, const char* newdata)
{
  //append a string to the end of an existing string
  char* p;
  int len = (*data ? strlen(*data) : 0);
  if ((p = (char*)realloc(*data, len + strlen(newdata) + 1)) == NULL) {
    free(p);
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  *data = p;
  strcpy(*data + len, newdata);
  return *data;
}

////////////////////////////////////////////////////////////////////////

struct email_info_struct {
  int current;  //must be zet to 0
  time_t timestamp;
  char* from;
  struct email_info_email_list_struct* to;
  struct email_info_email_list_struct* cc;
  struct email_info_email_list_struct* bcc;
  char* subject;
  char* header;
  struct email_info_attachment_list_struct* bodylist;
  struct email_info_attachment_list_struct* attachmentlist;
  char* buf;
  int buflen;
  char* mime_boundary_body;
  char* mime_boundary_part;
  struct email_info_attachment_list_struct* current_attachment;
  FILE* debuglog;
  char dtable[64];
};

////////////////////////////////////////////////////////////////////////

struct email_info_email_list_struct {
  char* data;
  struct email_info_email_list_struct* next;
};

void email_info_string_list_add (struct email_info_email_list_struct** list, const char* data)
{
  struct email_info_email_list_struct** p = list;
  while (*p)
    p = &(*p)->next;
  if ((*p = (struct email_info_email_list_struct*)malloc(sizeof(struct email_info_email_list_struct))) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return;
  }
  (*p)->data = (data ? strdup(data) : NULL);
  (*p)->next = NULL;
}

void email_info_string_list_free (struct email_info_email_list_struct** list)
{
  struct email_info_email_list_struct* p = *list;
  struct email_info_email_list_struct* current;
  while (p) {
    current = p;
    p = current->next;
    free(current->data);
    free(current);
  }
  *list = NULL;
}

char* email_info_string_list_concatenate (struct email_info_email_list_struct* list)
{
  char* result = NULL;
  struct email_info_email_list_struct* listentry = list;
  while (listentry) {
    if (listentry->data && *listentry->data) {
      if (result)
        str_append(&result, "," NEWLINE "\t");
      str_append(&result, "<");
      str_append(&result, listentry->data);
      str_append(&result, ">");
    }
    listentry = listentry->next;
  }
  return result;
}

////////////////////////////////////////////////////////////////////////

struct email_info_attachment_list_struct {
  char* filename;
  char* mimetype;
  void* filedata;
  void* handle;
  quickmail_attachment_open_fn email_info_attachment_open;
  quickmail_attachment_read_fn email_info_attachment_read;
  quickmail_attachment_close_fn email_info_attachment_close;
  quickmail_attachment_free_filedata_fn email_info_attachment_filedata_free;
  struct email_info_attachment_list_struct* next;
};

struct email_info_attachment_list_struct* email_info_attachment_list_add (struct email_info_attachment_list_struct** list, const char* filename, const char* mimetype, void* filedata, quickmail_attachment_open_fn email_info_attachment_open, quickmail_attachment_read_fn email_info_attachment_read, quickmail_attachment_close_fn email_info_attachment_close, quickmail_attachment_free_filedata_fn email_info_attachment_filedata_free)
{
  struct email_info_attachment_list_struct** p = list;
  while (*p)
    p = &(*p)->next;
  if ((*p = (struct email_info_attachment_list_struct*)malloc(sizeof(struct email_info_attachment_list_struct))) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  (*p)->filename = strdup(filename ? filename : "UNNAMED");
  (*p)->mimetype = (mimetype ? strdup(mimetype) : NULL);
  (*p)->filedata = filedata;
  (*p)->handle = NULL;
  (*p)->email_info_attachment_open = email_info_attachment_open;
  (*p)->email_info_attachment_read = email_info_attachment_read;
  (*p)->email_info_attachment_close = email_info_attachment_close;
  (*p)->email_info_attachment_filedata_free = email_info_attachment_filedata_free;
  (*p)->next = NULL;
  return *p;
}

void email_info_attachment_list_free_entry (struct email_info_attachment_list_struct* current)
{
  if (current->handle) {
    if (current->email_info_attachment_close)
      current->email_info_attachment_close(current->handle);
    //else
    //  free(current->handle);
    current->handle = NULL;
  }
  if (current->filedata) {
    if (current->email_info_attachment_filedata_free)
      current->email_info_attachment_filedata_free(current->filedata);
    else
      free(current->filedata);
  }
  if (current->mimetype)
    free(current->mimetype);
  free(current->filename);
  free(current);
}

void email_info_attachment_list_free (struct email_info_attachment_list_struct** list)
{
  struct email_info_attachment_list_struct* p = *list;
  struct email_info_attachment_list_struct* current;
  while (p) {
    current = p;
    p = current->next;
    email_info_attachment_list_free_entry(current);
  }
  *list = NULL;
}

int email_info_attachment_list_delete (struct email_info_attachment_list_struct** list, const char* filename)
{
  struct email_info_attachment_list_struct** p = list;
  while (*p) {
    if (strcmp((*p)->filename, filename) == 0) {
      struct email_info_attachment_list_struct* current = *p;
      *p = current->next;
      email_info_attachment_list_free_entry(current);
      return 0;
    }
    p = &(*p)->next;
  }
  return -1;
}

void email_info_attachment_list_close_handles (struct email_info_attachment_list_struct* list)
{
  struct email_info_attachment_list_struct* p = list;
  while (p) {
    if (p->handle) {
      if (p->email_info_attachment_close)
        p->email_info_attachment_close(p->handle);
      //else
      //  free(p->handle);
      p->handle = NULL;
    }
    p = p->next;
  }
}

//dummy attachment functions

void* email_info_attachment_open_dummy (void* filedata)
{
  return &email_info_attachment_open_dummy;
}

size_t email_info_attachment_read_dummy (void* handle, void* buf, size_t len)
{
  return 0;
}

struct email_info_attachment_list_struct* email_info_attachment_list_add_dummy (struct email_info_attachment_list_struct** list, const char* filename, const char* mimetype)
{
  return email_info_attachment_list_add(list, filename, mimetype, NULL, email_info_attachment_open_dummy, email_info_attachment_read_dummy, NULL, NULL);
}

//file attachment functions

void* email_info_attachment_open_file (void* filedata)
{
  return (void*)fopen((char*)filedata, "rb");
}

size_t email_info_attachment_read_file (void* handle, void* buf, size_t len)
{
  return fread(buf, 1, len, (FILE*)handle);;
}

void email_info_attachment_close_file (void* handle)
{
  if (handle)
    fclose((FILE*)handle);
}

struct email_info_attachment_list_struct* email_info_attachment_list_add_file (struct email_info_attachment_list_struct** list, const char* path, const char* mimetype)
{
  //determine base filename
  const char* basename = path + strlen(path);
  while (basename != path) {
    basename--;
    if (*basename == '/') {
      basename++;
      break;
    }
  }
  return email_info_attachment_list_add(list, basename, mimetype, (void*)strdup(path), email_info_attachment_open_file, email_info_attachment_read_file, email_info_attachment_close_file, NULL);
}

//memory attachment functions

struct email_info_attachment_memory_filedata_struct {
  char* data;
  size_t datalen;
  int mustfree;
};

struct email_info_attachment_memory_handle_struct {
  const char* data;
  size_t datalen;
  size_t pos;
};

void* email_info_attachment_open_memory (void* filedata)
{
  struct email_info_attachment_memory_filedata_struct* data;
  struct email_info_attachment_memory_handle_struct* result;
  data = ((struct email_info_attachment_memory_filedata_struct*)filedata);
  if (!data->data)
    return NULL;
  if ((result = (struct email_info_attachment_memory_handle_struct*)malloc(sizeof(struct email_info_attachment_memory_handle_struct))) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  result->data = data->data;
  result->datalen = data->datalen;
  result->pos = 0;
  return result;
}

size_t email_info_attachment_read_memory (void* handle, void* buf, size_t len)
{
  struct email_info_attachment_memory_handle_struct* h = (struct email_info_attachment_memory_handle_struct*)handle;
  size_t n = (h->pos + len <= h->datalen ? len : h->datalen - h->pos);
  memcpy(buf, h->data + h->pos, n);
  h->pos += n;
  return n;
}

void email_info_attachment_close_memory (void* handle)
{
  if (handle)
    free(handle);
}

void email_info_attachment_filedata_free_memory (void* filedata)
{
  struct email_info_attachment_memory_filedata_struct* data = ((struct email_info_attachment_memory_filedata_struct*)filedata);
  if (data) {
    if (data->mustfree)
      free(data->data);
    free(data);
  }
}

struct email_info_attachment_list_struct* email_info_attachment_list_add_memory (struct email_info_attachment_list_struct** list, const char* filename, const char* mimetype, char* data, size_t datalen, int mustfree)
{
  struct email_info_attachment_memory_filedata_struct* filedata;
  if ((filedata = (struct email_info_attachment_memory_filedata_struct*)malloc(sizeof(struct email_info_attachment_memory_filedata_struct))) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  filedata->data = data;
  filedata->datalen = datalen;
  filedata->mustfree = mustfree;
  return email_info_attachment_list_add(list, filename, mimetype, filedata, email_info_attachment_open_memory, email_info_attachment_read_memory, email_info_attachment_close_memory, email_info_attachment_filedata_free_memory);
}

////////////////////////////////////////////////////////////////////////

const char* quickmail_get_version ()
{
  return VERSION_STRINGIZE(LIBQUICKMAIL_VERSION_MAJOR, LIBQUICKMAIL_VERSION_MINOR, LIBQUICKMAIL_VERSION_MICRO);
}

int quickmail_cleanup ()
{
#ifndef NOCURL
  curl_global_cleanup();
#endif
  return 0;
}

quickmail quickmail_create (const char* from, const char* subject)
{
  int i;
  struct email_info_struct* mailobj;
  if ((mailobj = (struct email_info_struct*)malloc(sizeof(struct email_info_struct))) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  mailobj->current = 0;
  mailobj->timestamp = time(NULL);
  mailobj->from = (from ? strdup(from) : NULL);
  mailobj->to = NULL;
  mailobj->cc = NULL;
  mailobj->bcc = NULL;
  mailobj->subject = (subject ? strdup(subject) : NULL);
  mailobj->header = NULL;
  mailobj->bodylist = NULL;
  mailobj->attachmentlist = NULL;
  mailobj->buf = NULL;
  mailobj->buflen = 0;
  mailobj->mime_boundary_body = NULL;
  mailobj->mime_boundary_part = NULL;
  mailobj->current_attachment = NULL;
  mailobj->debuglog = NULL;
  for (i = 0; i < 26; i++) {
    mailobj->dtable[i] = (char)('A' + i);
    mailobj->dtable[26 + i] = (char)('a' + i);
  }
  for (i = 0; i < 10; i++) {
    mailobj->dtable[52 + i] = (char)('0' + i);
  }
  mailobj->dtable[62] = '+';
  mailobj->dtable[63] = '/';
  srand(time(NULL));
  return mailobj;
}

void quickmail_destroy (quickmail mailobj)
{
  free(mailobj->from);
  email_info_string_list_free(&mailobj->to);
  email_info_string_list_free(&mailobj->cc);
  email_info_string_list_free(&mailobj->bcc);
  free(mailobj->subject);
  free(mailobj->header);
  email_info_attachment_list_free(&mailobj->bodylist);
  email_info_attachment_list_free(&mailobj->attachmentlist);
  free(mailobj->buf);
  free(mailobj->mime_boundary_body);
  free(mailobj->mime_boundary_part);
  free(mailobj);
}

void quickmail_set_from (quickmail mailobj, const char* from)
{
  free(mailobj->from);
  mailobj->from = strdup(from);
}

const char* quickmail_get_from (quickmail mailobj)
{
  return mailobj->from;
}

void quickmail_add_to (quickmail mailobj, const char* email)
{
  email_info_string_list_add(&mailobj->to, email);
}

void quickmail_add_cc (quickmail mailobj, const char* email)
{
  email_info_string_list_add(&mailobj->cc, email);
}

void quickmail_add_bcc (quickmail mailobj, const char* email)
{
  email_info_string_list_add(&mailobj->bcc, email);
}

void quickmail_set_subject (quickmail mailobj, const char* subject)
{
  free(mailobj->subject);
  mailobj->subject = (subject ? strdup(subject) : NULL);
}

const char* quickmail_get_subject (quickmail mailobj)
{
  return mailobj->subject;
}

void quickmail_add_header (quickmail mailobj, const char* headerline)
{
  str_append(&mailobj->header, headerline);
  str_append(&mailobj->header, NEWLINE);
}

void quickmail_set_body (quickmail mailobj, const char* body)
{
  email_info_attachment_list_free(&mailobj->bodylist);
  if (body)
    email_info_attachment_list_add_memory(&mailobj->bodylist, default_mime_type, default_mime_type, strdup(body), strlen(body), 1);
}

char* quickmail_get_body (quickmail mailobj)
{
  size_t n;
  char* p;
  char* result = NULL;
  size_t resultlen = 0;
  if (mailobj->bodylist && (mailobj->bodylist->handle = mailobj->bodylist->email_info_attachment_open(mailobj->bodylist->filedata)) != NULL) {
    do {
      if ((p = (char*)realloc(result, resultlen + BODY_BUFFER_SIZE)) == NULL) {
        free(result);
        result = NULL;
        DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
        break;
      }
      result = p;
      if ((n = mailobj->bodylist->email_info_attachment_read(mailobj->bodylist->handle, result + resultlen, BODY_BUFFER_SIZE)) > 0)
        resultlen += n;
    } while (n > 0);
    if (mailobj->bodylist->email_info_attachment_close)
      mailobj->bodylist->email_info_attachment_close(mailobj->bodylist->handle);
    //else
    //  free(mailobj->bodylist->handle);
    mailobj->bodylist->handle = NULL;
  }
  return result;
}

void quickmail_add_body_file (quickmail mailobj, const char* mimetype, const char* path)
{
  email_info_attachment_list_add(&mailobj->bodylist, NULL, (mimetype ? mimetype : default_mime_type), (void*)strdup(path), email_info_attachment_open_file, email_info_attachment_read_file, email_info_attachment_close_file, NULL);
}

void quickmail_add_body_memory (quickmail mailobj, const char* mimetype, char* data, size_t datalen, int mustfree)
{
  email_info_attachment_list_add_memory(&mailobj->bodylist, NULL, (mimetype ? mimetype : default_mime_type), data, datalen, mustfree);
}

void quickmail_add_body_custom (quickmail mailobj, const char* mimetype, char* data, quickmail_attachment_open_fn attachment_data_open, quickmail_attachment_read_fn attachment_data_read, quickmail_attachment_close_fn attachment_data_close, quickmail_attachment_free_filedata_fn attachment_data_filedata_free)
{
  email_info_attachment_list_add(&mailobj->bodylist, NULL, (mimetype ? mimetype : default_mime_type), data, (attachment_data_open ? attachment_data_open : email_info_attachment_open_dummy), (attachment_data_read ? attachment_data_read : email_info_attachment_read_dummy), attachment_data_close, attachment_data_filedata_free);
}

int quickmail_remove_body (quickmail mailobj, const char* mimetype)
{
  return email_info_attachment_list_delete(&mailobj->bodylist, mimetype);
}

void quickmail_list_bodies (quickmail mailobj, quickmail_list_attachment_callback_fn callback, void* callbackdata)
{
  struct email_info_attachment_list_struct* p = mailobj->bodylist;
  while (p) {
    callback(mailobj, p->filename, p->mimetype, p->email_info_attachment_open, p->email_info_attachment_read, p->email_info_attachment_close, callbackdata);
    p = p->next;
  }
}

void quickmail_add_attachment_file (quickmail mailobj, const char* path, const char* mimetype)
{
  email_info_attachment_list_add_file(&mailobj->attachmentlist, path, mimetype);
}

void quickmail_add_attachment_memory (quickmail mailobj, const char* filename, const char* mimetype, char* data, size_t datalen, int mustfree)
{
  email_info_attachment_list_add_memory(&mailobj->attachmentlist, filename, mimetype, data, datalen, mustfree);
}

void quickmail_add_attachment_custom (quickmail mailobj, const char* filename, const char* mimetype, char* data, quickmail_attachment_open_fn attachment_data_open, quickmail_attachment_read_fn attachment_data_read, quickmail_attachment_close_fn attachment_data_close, quickmail_attachment_free_filedata_fn attachment_data_filedata_free)
{
  email_info_attachment_list_add(&mailobj->attachmentlist, filename, mimetype, data, (attachment_data_open ? attachment_data_open : email_info_attachment_open_dummy), (attachment_data_read ? attachment_data_read : email_info_attachment_read_dummy), attachment_data_close, attachment_data_filedata_free);
}

int quickmail_remove_attachment (quickmail mailobj, const char* filename)
{
  return email_info_attachment_list_delete(&mailobj->attachmentlist, filename);
}

void quickmail_list_attachments (quickmail mailobj, quickmail_list_attachment_callback_fn callback, void* callbackdata)
{
  struct email_info_attachment_list_struct* p = mailobj->attachmentlist;
  while (p) {
    callback(mailobj, p->filename, p->mimetype, p->email_info_attachment_open, p->email_info_attachment_read, p->email_info_attachment_close, callbackdata);
    p = p->next;
  }
}

void quickmail_set_debug_log (quickmail mailobj, FILE* filehandle)
{
  mailobj->debuglog = filehandle;
}

void quickmail_fsave (quickmail mailobj, FILE* filehandle)
{
  int i;
  size_t n;
  char buf[80];
  while ((n = quickmail_get_data(buf, sizeof(buf), 1, mailobj)) > 0) {
    for (i = 0; i < n; i++)
      fprintf(filehandle, "%c", buf[i]);
  }
}

size_t quickmail_get_data (void* ptr, size_t size, size_t nmemb, void* userp)
{
  struct email_info_struct* mailobj = (struct email_info_struct*)userp;

  //abort if no data is requested
  if (size * nmemb == 0)
    return 0;

  //initialize on first run
  if (mailobj->current == MAILPART_INITIALIZE) {
    free(mailobj->buf);
    mailobj->buf = NULL;
    mailobj->buflen = 0;
    free(mailobj->mime_boundary_body);
    mailobj->mime_boundary_body = NULL;
    free(mailobj->mime_boundary_part);
    mailobj->mime_boundary_part = NULL;
    mailobj->current_attachment = mailobj->bodylist;
    mailobj->current++;
  }

  //process current part of mail if no partial data is pending
  while (mailobj->buflen == 0) {
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_HEADER) {
      char* s;
      //generate header part
      char** p = &mailobj->buf;
      str_append(p, "User-Agent: libquickmail v" LIBQUICKMAIL_VERSION NEWLINE);
      if (mailobj->timestamp != 0) {
        char* oldlocale;
        char timestamptext[32];
        //get original locale settings and switch to C locale (so date is in English)
        if ((oldlocale = setlocale(LC_TIME, NULL)) != NULL) {
          oldlocale = strdup(oldlocale);
          setlocale(LC_TIME, "C");
        }
        //format timestamp
        if (strftime(timestamptext, sizeof(timestamptext), "%a, %d %b %Y %H:%M:%S %z", localtime(&mailobj->timestamp))) {
          str_append(p, "Date: ");
          str_append(p, timestamptext);
          str_append(p, NEWLINE);
        }
        //restore original locale
        if (oldlocale) {
          setlocale(LC_TIME, oldlocale);
          free(oldlocale);
        }
      }
      if (mailobj->from && *mailobj->from) {
        str_append(p, "From: <");
        str_append(p, mailobj->from);
        str_append(p, ">" NEWLINE);
      }
      if ((s = email_info_string_list_concatenate(mailobj->to)) != NULL) {
        str_append(p, "To: ");
        str_append(p, s);
        str_append(p, NEWLINE);
        free(s);
      }
      if ((s = email_info_string_list_concatenate(mailobj->cc)) != NULL) {
        str_append(p, "Cc: ");
        str_append(p, s);
        str_append(p, NEWLINE);
        free(s);
      }
      if (mailobj->subject) {
        str_append(p, "Subject: ");
        str_append(p, mailobj->subject);
        str_append(p, NEWLINE);
      }
      if (mailobj->header) {
        str_append(p, mailobj->header);
      }
      if (mailobj->attachmentlist) {
        str_append(p, "MIME-Version: 1.0" NEWLINE);
      }
      if (mailobj->attachmentlist) {
        mailobj->mime_boundary_part = randomize_zeros(strdup("=PART=SEPARATOR=_0000_0000_0000_0000_0000_0000_="));
        str_append(p, "Content-Type: multipart/mixed; boundary=\"");
        str_append(p, mailobj->mime_boundary_part);
        str_append(p, "\"" NEWLINE NEWLINE "This is a multipart message in MIME format." NEWLINE NEWLINE "--");
        str_append(p, mailobj->mime_boundary_part);
        str_append(p, NEWLINE);
      }
      if (mailobj->bodylist && mailobj->bodylist->next) {
        mailobj->mime_boundary_body = randomize_zeros(strdup("=BODY=SEPARATOR=_0000_0000_0000_0000_0000_0000_="));
        str_append(p, "Content-Type: multipart/alternative; boundary=\"");
        str_append(p, mailobj->mime_boundary_body);
        str_append(p, "\"" NEWLINE);
      }
      mailobj->buflen = (mailobj->buf ? strlen(mailobj->buf) : 0);
      mailobj->current++;
    }
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_BODY) {
      if (mailobj->current_attachment) {
        if (!mailobj->current_attachment->handle) {
          //open file with body data
          while (mailobj->current_attachment) {
            if ((mailobj->current_attachment->handle = mailobj->current_attachment->email_info_attachment_open(mailobj->current_attachment->filedata)) != NULL) {
              break;
            }
            /////to do: notify/log the file could not be opened
            mailobj->current_attachment = mailobj->current_attachment->next;
          }
          if (!mailobj->current_attachment) {
            mailobj->current_attachment = mailobj->attachmentlist;
            mailobj->current++;
          }
          //generate attachment header
          if (mailobj->current_attachment && mailobj->current_attachment->handle) {
            mailobj->buf = NULL;
            if (mailobj->mime_boundary_body) {
              mailobj->buf = str_append(&mailobj->buf, NEWLINE "--");
              mailobj->buf = str_append(&mailobj->buf, mailobj->mime_boundary_body);
              mailobj->buf = str_append(&mailobj->buf, NEWLINE);
            }
            mailobj->buf = str_append(&mailobj->buf, "Content-Type: ");
            mailobj->buf = str_append(&mailobj->buf, (mailobj->bodylist && mailobj->current_attachment->mimetype ? mailobj->current_attachment->mimetype : default_mime_type));
            mailobj->buf = str_append(&mailobj->buf, NEWLINE "Content-Transfer-Encoding: 8bit" NEWLINE "Content-Disposition: inline" NEWLINE NEWLINE);
            mailobj->buflen = (mailobj->buf ? strlen(mailobj->buf) : 0);
          }
        }
        if (mailobj->buflen == 0 && mailobj->current_attachment && mailobj->current_attachment->handle) {
          //read body data
          if ((mailobj->buf = malloc(BODY_BUFFER_SIZE)) == NULL) {
            DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
          }
          if (mailobj->buf == NULL || (mailobj->buflen = mailobj->current_attachment->email_info_attachment_read(mailobj->current_attachment->handle, mailobj->buf, BODY_BUFFER_SIZE)) <= 0) {
            //end of file
            free(mailobj->buf);
            mailobj->buflen = 0;
            if (mailobj->current_attachment->email_info_attachment_close)
              mailobj->current_attachment->email_info_attachment_close(mailobj->current_attachment->handle);
            //else
            //  free(mailobj->current_attachment->handle);
            mailobj->current_attachment->handle = NULL;
            mailobj->current_attachment = mailobj->current_attachment->next;
          }
        }
      } else {
        mailobj->current_attachment = mailobj->attachmentlist;
        mailobj->current++;
      }
    }
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_BODY_DONE) {
      mailobj->buf = NULL;
      if (mailobj->mime_boundary_body) {
        mailobj->buf = str_append(&mailobj->buf, NEWLINE "--");
        mailobj->buf = str_append(&mailobj->buf, mailobj->mime_boundary_body);
        mailobj->buf = str_append(&mailobj->buf, "--" NEWLINE);
        mailobj->buflen = strlen(mailobj->buf);
        free(mailobj->mime_boundary_body);
        mailobj->mime_boundary_body = NULL;
      }
      mailobj->current++;
    }
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_ATTACHMENT) {
      if (mailobj->current_attachment) {
        if (!mailobj->current_attachment->handle) {
          //open file to attach
          while (mailobj->current_attachment) {
            if ((mailobj->current_attachment->handle = mailobj->current_attachment->email_info_attachment_open(mailobj->current_attachment->filedata)) != NULL) {
              break;
            }
            mailobj->current_attachment = mailobj->current_attachment->next;
          }
          //generate attachment header
          if (mailobj->current_attachment && mailobj->current_attachment->handle) {
            mailobj->buf = NULL;
            if (mailobj->mime_boundary_part) {
              mailobj->buf = str_append(&mailobj->buf, NEWLINE "--");
              mailobj->buf = str_append(&mailobj->buf, mailobj->mime_boundary_part);
              mailobj->buf = str_append(&mailobj->buf, NEWLINE);
            }
            mailobj->buf = str_append(&mailobj->buf, "Content-Type: ");
            mailobj->buf = str_append(&mailobj->buf, (mailobj->current_attachment->mimetype ? mailobj->current_attachment->mimetype : "application/octet-stream"));
            mailobj->buf = str_append(&mailobj->buf, "; Name=\"");
            mailobj->buf = str_append(&mailobj->buf, (mailobj->current_attachment->filename ? mailobj->current_attachment->filename : "ATTACHMENT"));
            mailobj->buf = str_append(&mailobj->buf, "\"" NEWLINE "Content-Disposition: attachment; filename=\"");
            mailobj->buf = str_append(&mailobj->buf, (mailobj->current_attachment->filename ? mailobj->current_attachment->filename : "ATTACHMENT"));
            mailobj->buf = str_append(&mailobj->buf, "\"" NEWLINE "Content-Transfer-Encoding: base64" NEWLINE NEWLINE);
            mailobj->buflen = strlen(mailobj->buf);
          }
        } else {
          //generate next line of attachment data
            size_t n = 0;
            int mimelinepos = 0;
            unsigned char igroup[3] = {0, 0, 0};
            unsigned char ogroup[4];
            mailobj->buflen = 0;
            if ((mailobj->buf = (char*)malloc(MIME_LINE_WIDTH + NEWLINELENGTH + 1)) == NULL) {
              DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
              n = 0;
            } else {
              while (mimelinepos < MIME_LINE_WIDTH && (n = mailobj->current_attachment->email_info_attachment_read(mailobj->current_attachment->handle, igroup, 3)) > 0) {
                //code data to base64
                ogroup[0] = mailobj->dtable[igroup[0] >> 2];
                ogroup[1] = mailobj->dtable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
                ogroup[2] = mailobj->dtable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
                ogroup[3] = mailobj->dtable[igroup[2] & 0x3F];
                //padd with "=" characters if less than 3 characters were read
                if (n < 3) {
                  ogroup[3] = '=';
                  if (n < 2)
                    ogroup[2] = '=';
                }
                memcpy(mailobj->buf + mimelinepos, ogroup, 4);
                mailobj->buflen += 4;
                mimelinepos += 4;
              }
              if (mimelinepos > 0) {
                memcpy(mailobj->buf + mimelinepos, NEWLINE, NEWLINELENGTH);
                mailobj->buflen += NEWLINELENGTH;
              }
            }
            if (n <= 0) {
              //end of file
              if (mailobj->current_attachment->email_info_attachment_close)
                mailobj->current_attachment->email_info_attachment_close(mailobj->current_attachment->handle);
              else
                free(mailobj->current_attachment->handle);
              mailobj->current_attachment->handle = NULL;
              mailobj->current_attachment = mailobj->current_attachment->next;
            }
        }
      } else {
        mailobj->current++;
      }
    }
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_END) {
      mailobj->buf = NULL;
      mailobj->buflen = 0;
      if (mailobj->mime_boundary_part) {
        mailobj->buf = str_append(&mailobj->buf, NEWLINE "--");
        mailobj->buf = str_append(&mailobj->buf, mailobj->mime_boundary_part);
        mailobj->buf = str_append(&mailobj->buf, "--" NEWLINE);
        mailobj->buflen = strlen(mailobj->buf);
        free(mailobj->mime_boundary_part);
        mailobj->mime_boundary_part = NULL;
      }
      //mailobj->buf = str_append(&mailobj->buf, NEWLINE "." NEWLINE);
      //mailobj->buflen = strlen(mailobj->buf);
      mailobj->current++;
    }
    if (mailobj->buflen == 0 && mailobj->current == MAILPART_DONE) {
      break;
    }
  }

  //flush pending data if any
  if (mailobj->buflen > 0) {
    int len = (mailobj->buflen > size * nmemb ? size * nmemb : mailobj->buflen);
    memcpy(ptr, mailobj->buf, len);
    if (len < mailobj->buflen) {
      mailobj->buf = memmove(mailobj->buf, mailobj->buf + len, mailobj->buflen - len);
      mailobj->buflen -= len;
    } else {
      free(mailobj->buf);
      mailobj->buf = NULL;
      mailobj->buflen = 0;
    }
    return len;
  }

  //if (mailobj->current != MAILPART_DONE)
  //  ;//this should never be reached
  mailobj->current = 0;
  return 0;
}

#ifndef NOCURL
char* add_angle_brackets (const char* data)
{
  size_t datalen = strlen(data);
  char* result;
  if ((result = (char*)malloc(datalen + 3)) == NULL) {
    DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
    return NULL;
  }
  result[0] = '<';
  memcpy(result + 1, data, datalen);
  result[datalen + 1] = '>';
  result[datalen + 2] = 0;
  return result;
}
#endif

static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
  struct progress *myp = (struct progress *)p;
  CURL *curl = myp->curl;
  double curtime = 0;

  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */
  if ((curtime - myp->lastruntime) >= quickmail_progress) {
	if (myp->lastruntime == 0) printf("\r\n");
    myp->lastruntime = curtime;
    printf("TIME: %0.1f UP: %" CURL_FORMAT_CURL_OFF_T"\r", curtime, ulnow);
  }
  return 0;
}

const char* quickmail_protocol_send (quickmail mailobj, const char* smtpserver, unsigned int smtpport, int protocol, const char* username, const char* password)
{
  //libcurl based sending
  CURL *curl;
  CURLcode result = CURLE_FAILED_INIT;
  struct progress prog;

  if ((curl = curl_easy_init()) != NULL) {
   prog.lastruntime = 0;
   prog.curl = curl;
   struct curl_slist *recipients = NULL;
    struct email_info_email_list_struct* listentry;
    //set destination URL
    char* addr;
    size_t len = strlen(smtpserver) + 14;
    if ((addr = (char*)malloc(len)) == NULL) {
      DEBUG_ERROR(ERRMSG_MEMORY_ALLOCATION_ERROR)
      return ERRMSG_MEMORY_ALLOCATION_ERROR;
    }
    snprintf(addr, len, "%s://%s:%u", (protocol == QUICKMAIL_PROT_SMTPS ? "smtps" : "smtp"), smtpserver, smtpport);
    curl_easy_setopt(curl, CURLOPT_URL, addr);
    free(addr);
    //try Transport Layer Security (TLS), but continue anyway if it fails
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);
    //don't fail if the TLS/SSL a certificate could not be verified
    //alternative: add the issuer certificate (or the host certificate if
    //the certificate is self-signed) to the set of certificates that are
    //known to libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    //set authentication credentials if provided
    if (username && *username)
      curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    if (password)
      curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    //set from value for envelope reverse-path
    if (mailobj->from && *mailobj->from) {
      addr = add_angle_brackets(mailobj->from);
      curl_easy_setopt(curl, CURLOPT_MAIL_FROM, addr);
      free(addr);
    }
    //set recipients
    listentry = mailobj->to;
    while (listentry) {
      if (listentry->data && *listentry->data) {
        addr = add_angle_brackets(listentry->data);
        recipients = curl_slist_append(recipients, addr);
        free(addr);
      }
      listentry = listentry->next;
    }
    listentry = mailobj->cc;
    while (listentry) {
      if (listentry->data && *listentry->data) {
        addr = add_angle_brackets(listentry->data);
        recipients = curl_slist_append(recipients, addr);
        free(addr);
      }
      listentry = listentry->next;
    }
    listentry = mailobj->bcc;
    while (listentry) {
      if (listentry->data && *listentry->data) {
        addr = add_angle_brackets(listentry->data);
        recipients = curl_slist_append(recipients, addr);
        free(addr);
      }
      listentry = listentry->next;
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    //set callback function for getting message body
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, quickmail_get_data);
    curl_easy_setopt(curl, CURLOPT_READDATA, mailobj);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); //set CURLOPT_UPLOAD to 1 to not use VRFY and other unneeded commands
    //enable debugging if requested
    if (mailobj->debuglog) {
      curl_easy_setopt(curl, CURLOPT_VERBOSE, quickmail_verbose);
      curl_easy_setopt(curl, CURLOPT_STDERR, mailobj->debuglog);
    }
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, quickmail_timeout);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    // pass the struct pointer into the xferinfo function, note that this is an alias to CURLOPT_PROGRESSDATA
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
    if (quickmail_progress) curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

    //send the message
    result = curl_easy_perform(curl);
    printf("\r\n");
    //free the list of recipients and clean up
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
  }
  return (result == CURLE_OK ? NULL : curl_easy_strerror(result));
}

const char* quickmail_send (quickmail mailobj, const char* smtpserver, unsigned int smtpport, const char* username, const char* password)
{
  return quickmail_protocol_send(mailobj, smtpserver, smtpport, QUICKMAIL_PROT_SMTP, username, password);
}
const char* quickmail_send_secure (quickmail mailobj, const char* smtpserver, unsigned int smtpport, const char* username, const char* password)
{
  return quickmail_protocol_send(mailobj, smtpserver, smtpport, QUICKMAIL_PROT_SMTPS, username, password);
}
