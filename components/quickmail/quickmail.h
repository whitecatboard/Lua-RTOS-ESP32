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

/*! \file      quickmail.h
 *  \brief     header file for libquickmail
 *  \author    Brecht Sanders
 *  \date      2012-2016
 *  \copyright GPL
 */

#ifndef __INCLUDED_QUICKMAIL_H
#define __INCLUDED_QUICKMAIL_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUICKMAIL_PROT_SMTP  1
#define QUICKMAIL_PROT_SMTPS 2


/*! \brief quickmail object type */
typedef struct email_info_struct* quickmail;



/*! \brief type of pointer to function for opening attachment data
 * \param  filedata    custom data as passed to quickmail_add_body_custom/quickmail_add_attachment_custom
 * \return data structure to be used in calls to quickmail_attachment_read_fn and quickmail_attachment_close_fn functions
 * \sa     quickmail_add_body_custom()
 * \sa     quickmail_add_attachment_custom()
 */
typedef void* (*quickmail_attachment_open_fn)(void* filedata);

/*! \brief type of pointer to function for reading attachment data
 * \param  handle      data structure obtained via the corresponding quickmail_attachment_open_fn function
 * \param  buf         buffer for receiving data
 * \param  len         size in bytes of buffer for receiving data
 * \return number of bytes read (zero on end of file)
 * \sa     quickmail_add_body_custom()
 * \sa     quickmail_add_attachment_custom()
 */
typedef size_t (*quickmail_attachment_read_fn)(void* handle, void* buf, size_t len);

/*! \brief type of pointer to function for closing attachment data
 * \param  handle      data structure obtained via the corresponding quickmail_attachment_open_fn function
 * \sa     quickmail_add_body_custom()
 * \sa     quickmail_add_attachment_custom()
 */
typedef void (*quickmail_attachment_close_fn)(void* handle);

/*! \brief type of pointer to function for cleaning up custom data in quickmail_destroy
 * \param  filedata    custom data as passed to quickmail_add_body_custom/quickmail_add_attachment_custom
 * \sa     quickmail_add_body_custom()
 * \sa     quickmail_add_attachment_custom()
 */
typedef void (*quickmail_attachment_free_filedata_fn)(void* filedata);

/*! \brief type of pointer to function for cleaning up custom data in quickmail_destroy
 * \param  mailobj                        quickmail object
 * \param  filename                       attachment filename (same value as mimetype for mail body)
 * \param  mimetype                       MIME type
 * \param  attachment_data_open           function for opening attachment data
 * \param  attachment_data_read           function for reading attachment data
 * \param  attachment_data_close          function for closing attachment data (optional, free() will be used if NULL)
 * \param  callbackdata                   custom data passed to quickmail_list_attachments
 * \sa     quickmail_list_bodies()
 * \sa     quickmail_list_attachments()
 */
typedef void (*quickmail_list_attachment_callback_fn)(quickmail mailobj, const char* filename, const char* mimetype, quickmail_attachment_open_fn email_info_attachment_open, quickmail_attachment_read_fn email_info_attachment_read, quickmail_attachment_close_fn email_info_attachment_close, void* callbackdata);



/*! \brief get version quickmail library
 * \return library version
 */
const char* quickmail_get_version ();

/*! \brief clean up quickmail library, call once at the end of the main thread of the application
 * \return zero on success
 */
int quickmail_cleanup ();

/*! \brief create a new quickmail object
 * \param  from        sender e-mail address
 * \param  subject     e-mail subject
 * \return quickmail object or NULL on error
 */
quickmail quickmail_create (const char* from, const char* subject);

/*! \brief clean up a quickmail object
 * \param  mailobj     quickmail object
 */
void quickmail_destroy (quickmail mailobj);

/*! \brief set the sender (from) e-mail address of a quickmail object
 * \param  mailobj     quickmail object
 * \param  from        sender e-mail address
 */
void quickmail_set_from (quickmail mailobj, const char* from);

/*! \brief get the sender (from) e-mail address of a quickmail object
 * \param  mailobj     quickmail object
 * \return sender e-mail address
 */
const char* quickmail_get_from (quickmail mailobj);

/*! \brief add a recipient (to) e-mail address to a quickmail object
 * \param  mailobj     quickmail object
 * \param  e-mail      recipient e-mail address
 */
void quickmail_add_to (quickmail mailobj, const char* email);

/*! \brief add a carbon copy recipient (cc) e-mail address to a quickmail object
 * \param  mailobj     quickmail object
 * \param  e-mail      recipient e-mail address
 */
void quickmail_add_cc (quickmail mailobj, const char* email);

/*! \brief add a blind carbon copy recipient (bcc) e-mail address to a quickmail object
 * \param  mailobj     quickmail object
 * \param  e-mail      recipient e-mail address
 */
void quickmail_add_bcc (quickmail mailobj, const char* email);

/*! \brief set the subject of a quickmail object
 * \param  mailobj     quickmail object
 * \param  subject     e-mail subject
 */
void quickmail_set_subject (quickmail mailobj, const char* subject);

/*! \brief set the subject of a quickmail object
 * \param  mailobj     quickmail object
 * \return e-mail subject
 */
const char* quickmail_get_subject (quickmail mailobj);

/*! \brief add an e-mail header to a quickmail object
 * \param  mailobj     quickmail object
 * \param  headerline  header line to add
 */
void quickmail_add_header (quickmail mailobj, const char* headerline);

/*! \brief set the body of a quickmail object
 * \param  mailobj     quickmail object
 * \param  body        e-mail body
 */
void quickmail_set_body (quickmail mailobj, const char* body);

/*! \brief set the body of a quickmail object
 * any existing bodies will be removed and a single plain text body will be added
 * \param  mailobj     quickmail object
 * \return e-mail body or NULL on error (caller must free result)
 */
char* quickmail_get_body (quickmail mailobj);

/*! \brief add a body file to a quickmail object (deprecated)
 * \param  mailobj     quickmail object
 * \param  mimetype    MIME type (text/plain will be used if set to NULL)
 * \param  path        path of file with body data
 */
void quickmail_add_body_file (quickmail mailobj, const char* mimetype, const char* path);

/*! \brief add a body from memory to a quickmail object
 * \param  mailobj     quickmail object
 * \param  mimetype    MIME type (text/plain will be used if set to NULL)
 * \param  data        body content
 * \param  datalen     size of data in bytes
 * \param  mustfree    non-zero if data must be freed by quickmail_destroy
 */
void quickmail_add_body_memory (quickmail mailobj, const char* mimetype, char* data, size_t datalen, int mustfree);

/*! \brief add a body with custom read functions to a quickmail object
 * \param  mailobj                        quickmail object
 * \param  mimetype                       MIME type (text/plain will be used if set to NULL)
 * \param  data                           custom data passed to attachment_data_open and attachment_data_filedata_free functions
 * \param  attachment_data_open           function for opening attachment data
 * \param  attachment_data_read           function for reading attachment data
 * \param  attachment_data_close          function for closing attachment data (optional, free() will be used if NULL)
 * \param  attachment_data_filedata_free  function for cleaning up custom data in quickmail_destroy (optional, free() will be used if NULL)
 */
void quickmail_add_body_custom (quickmail mailobj, const char* mimetype, char* data, quickmail_attachment_open_fn attachment_data_open, quickmail_attachment_read_fn attachment_data_read, quickmail_attachment_close_fn attachment_data_close, quickmail_attachment_free_filedata_fn attachment_data_filedata_free);

/*! \brief remove body from quickmail object
 * \param  mailobj     quickmail object
 * \param  mimetype    MIME type (text/plain will be used if set to NULL)
 * \return zero on success
 */
int quickmail_remove_body (quickmail mailobj, const char* mimetype);

/*! \brief list bodies by calling a callback function for each body
 * \param  mailobj                        quickmail object
 * \param  callback                       function to call for each attachment
 * \param  callbackdata                   custom data to pass to the callback function
 * \sa     quickmail_list_attachment_callback_fn
 */
void quickmail_list_bodies (quickmail mailobj, quickmail_list_attachment_callback_fn callback, void* callbackdata);

/*! \brief add a file attachment to a quickmail object
 * \param  mailobj     quickmail object
 * \param  path        path of file to attach
 * \param  mimetype    MIME type of file to attach (application/octet-stream will be used if set to NULL)
 */
void quickmail_add_attachment_file (quickmail mailobj, const char* path, const char* mimetype);

/*! \brief add an attachment from memory to a quickmail object
 * \param  mailobj     quickmail object
 * \param  filename    name of file to attach (must not include full path)
 * \param  mimetype    MIME type of file to attach (set to NULL for default binary file)
 * \param  data        data content
 * \param  datalen     size of data in bytes
 * \param  mustfree    non-zero if data must be freed by quickmail_destroy
 */
void quickmail_add_attachment_memory (quickmail mailobj, const char* filename, const char* mimetype, char* data, size_t datalen, int mustfree);

/*! \brief add an attachment with custom read functions to a quickmail object
 * \param  mailobj                        quickmail object
 * \param  filename                       name of file to attach (must not include full path)
 * \param  mimetype                       MIME type of file to attach (set to NULL for default binary file)
 * \param  data                           custom data passed to attachment_data_open and attachment_data_filedata_free functions
 * \param  attachment_data_open           function for opening attachment data
 * \param  attachment_data_read           function for reading attachment data
 * \param  attachment_data_close          function for closing attachment data (optional, free() will be used if NULL)
 * \param  attachment_data_filedata_free  function for cleaning up custom data in quickmail_destroy (optional, free() will be used if NULL)
 */
void quickmail_add_attachment_custom (quickmail mailobj, const char* filename, const char* mimetype, char* data, quickmail_attachment_open_fn attachment_data_open, quickmail_attachment_read_fn attachment_data_read, quickmail_attachment_close_fn attachment_data_close, quickmail_attachment_free_filedata_fn attachment_data_filedata_free);

/*! \brief remove attachment from quickmail object
 * \param  mailobj     quickmail object
 * \param  filename    name of file to attach (must not include full path)
 * \return zero on success
 */
int quickmail_remove_attachment (quickmail mailobj, const char* filename);

/*! \brief list attachments by calling a callback function for each attachment
 * \param  mailobj                        quickmail object
 * \param  callback                       function to call for each attachment
 * \param  callbackdata                   custom data to pass to the callback function
 * \sa     quickmail_list_attachment_callback_fn
 */
void quickmail_list_attachments (quickmail mailobj, quickmail_list_attachment_callback_fn callback, void* callbackdata);

/*! \brief set the debug logging destination of a quickmail object
 * \param  mailobj     quickmail object
 * \param  filehandle  file handle of logging destination (or NULL for no logging)
 */
void quickmail_set_debug_log (quickmail mailobj, FILE* filehandle);

/*! \brief save the generated e-mail to a file
 * \param  mailobj     quickmail object
 * \param  filehandle  file handle to write the e-mail contents to
 */
void quickmail_fsave (quickmail mailobj, FILE* filehandle);

/*! \brief read data the next data from the e-mail contents (can be used as CURLOPT_READFUNCTION callback function)
 * \param  buffer      buffer to copy data to (bust be able to hold size * nmemb bytes)
 * \param  size        record size
 * \param  nmemb       number of records to copy to \p buffer
 * \param  mailobj     quickmail object
 * \return number of bytes copied to \p buffer or 0 if at end
 */
size_t quickmail_get_data (void* buffer, size_t size, size_t nmemb, void* mailobj);

/*! \brief send the e-mail via SMTP or SMTPS
 * \param  mailobj     quickmail object
 * \param  smtpserver  IP address or hostname of SMTP/SMTPS server
 * \param  smtpport    SMTP/SMTPS port number (normally this is 25)
 * \param  prptocol    protocol type (SMTP or SMTPS)
 * \param  username    username to use for authentication (or NULL if not needed)
 * \param  password    password to use for authentication (or NULL if not needed)
 * \return NULL on success or error message on error
 */
const char* quickmail_protocol_send (quickmail mailobj, const char* smtpserver, unsigned int smtpport, int protocol, const char* username, const char* password);

/*! \brief send the e-mail via SMTP
 * \param  mailobj     quickmail object
 * \param  smtpserver  IP address or hostname of SMTP server
 * \param  smtpport    SMTP port number (normally this is 25)
 * \param  username    username to use for authentication (or NULL if not needed)
 * \param  password    password to use for authentication (or NULL if not needed)
 * \return NULL on success or error message on error
 */
const char* quickmail_send (quickmail mailobj, const char* smtpserver, unsigned int smtpport, const char* username, const char* password);

/*! \brief send the e-mail via SMTPS
 * \param  mailobj     quickmail object
 * \param  smtpserver  IP address or hostname of SMTPS server
 * \param  smtpport    SMTPS port number (normally this is 465)
 * \param  username    username to use for authentication (or NULL if not needed)
 * \param  password    password to use for authentication (or NULL if not needed)
 * \return NULL on success or error message on error
 */
const char* quickmail_send_secure (quickmail mailobj, const char* smtpserver, unsigned int smtpport, const char* username, const char* password);

int quickmail_progress;
int quickmail_verbose;
int quickmail_timeout;

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_QUICKMAIL_H
