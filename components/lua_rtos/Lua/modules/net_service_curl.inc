/*
 * Lua RTOS, Lua curl module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 *
 * Author: Boris Lovosevic (loboris@gmail.com; https://github.com/loboris)
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

/* -------------------------------------------------------------------------------------
 * This implementation uses unmodified libcurl sources from https://github.com/curl/curl
 * and can be easily updated to newer libcurl version
 *
 * Only directories include & lib are used
 *
 * The only modified files are:
 * ----------------------------
 * curl/include/curl/curl.h		some buffer sizes
 * curl/lib/curl_config.h		some options
 *
 * -------------------------------------------------------------------------------------
 * zlib library from https://github.com/madler/zlib is used
 * -------------------------------------------------------------------------------------
 * modified libquickmail from https://github.com/cdevelop/libquickmail is used
 * -------------------------------------------------------------------------------------
 */
/*
 * ============================================================================
 *  It is recommended to set CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN to 8192 or 4096
 *  in mbedTLS section to avoid 'out of memory'  when using SSL/TLS transfers
 *
 *  CONFIG_TCPIP_TASK_STACK_SIZE has to be at least 4096
 * ============================================================================
 */

/* ----------------------------------------------------------------------------------------------
 * When performing SSL transfer curl can use server certificates saved in a file
 * Default location of the certificate file is /certs/ca-certificates.crt
 * This location can be changed in curl_config.h or set as curl option in _set_default_options().
 * Function to select this location at run time could be added.
 *
 * This implementation is configured to silently accept self-signed server certificates.
 * This behavior can be changed in _set_default_options() or a function could be added
 * to change it at run time.
 * ----------------------------------------------------------------------------------------------
 */

/* ##########################################################################################
 * There is a problem when using NET_VFS
 *   (USE_NET_VFS defined - true if CONFIG_WIFI_ENABLED || CONFIG_ETHERNET)
 *
 * Socket fd returned by socket() function is not the fd returned by lwip_socket(),
 * it is the file descriptor returned by fopen("/dev/socket/n","a+") in __wrap_lwip_socket(),
 * which causes closesocket() (lwip_close()) not to close the socket properly.
 * As a temporary  workaround, variable '_curl_socket' is added to lua_rtos/lwip/socket.c
 * which tells __wrap_lwip_socket() to return the original lwip_socket() fd if set to 1
 *
 * ------------------------------------------------------------------------------------------
 * --- This problem affects all functions which uses closesocket() function (ping(), ...) ---
 * --- All of those functions do not close the socket properly                            ---
 * ------------------------------------------------------------------------------------------
 * ##########################################################################################
 */

/* ====================================================================================================
 * To add curl module to Lua system it is necessary to make two additions to lua_rtos/Lua/modules/net.c
 *   - Include this file:
 *     #include "net_service_curl.inc"
 *
 *   - Add to 'static const LUA_REG_TYPE service_map[] = {'
 *     { LSTRKEY( "curl"    ),	 LROVAL   ( curl_map  ) },
 *
 * ====================================================================================================
 */

#include "luartos.h"

#if LUA_USE_HTTP

#include "curl/curl.h"
#include "quickmail/quickmail.h"
#include <esp_system.h>

// Some configuration variables
static uint8_t curl_verbose = 1;
static uint8_t curl_progress = 1;
static uint16_t curl_timeout = 60;
static uint32_t curl_maxbytes = 300000;
static uint16_t curl_max_headlen = 512;
static uint16_t curl_max_bodylen = 2048;
static uint8_t curl_initialized = 0;

static char mail_server[64] = "smtp.gmail.com";
static uint32_t mail_port = 465;

struct upload_status {
int size;
int ptr;
FILE *fattach;
int fsize;
int b64fsize;
uint8_t finished;
};

struct curl_string {
  char *ptr;
  size_t len;
  int status;
  uint16_t maxlen;
};

struct progress {
  double lastruntime;
  double transfertime;
  uint8_t curl_progress;
  curl_off_t dlnow;
  curl_off_t ulnow;
  CURL *curl;
};

struct curl_httppost *formpost = NULL;
struct curl_httppost *lastptr = NULL;


// Initialize the structure used in curlWrite callback
//-------------------------------------------------------------------
static int init_curl_string(struct curl_string *s, uint16_t maxlen) {
	s->len = 0;
	s->status = 0;
	s->maxlen = maxlen;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) return -1;

	s->ptr[0] = '\0';
	return 0;
}

// Callback: Get response header or body to buffer
//--------------------------------------------------------------------------------
static size_t curlWrite(void *buffer, size_t size, size_t nmemb, void *userdata) {
	struct curl_string *s = (struct curl_string *)userdata;
	size_t new_len = s->len;
	if ((s->status == 0) && (s->len < s->maxlen) && ((size*nmemb) > 0)) {
		new_len = s->len + size*nmemb;
		if (new_len > s->maxlen) new_len = s->maxlen;
		char *tmps = realloc(s->ptr, new_len+1);
		if (tmps != NULL) {
			s->ptr = tmps;
			memcpy(s->ptr + s->len, buffer, size*nmemb);
			s->ptr[new_len] = '\0';
			s->len = new_len;
		}
		else s->status = -1;
	}

	return size*nmemb;
}

// Callback: ftp PUT file
//-----------------------------------------------------------------------------
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t nread = fread(ptr, 1, size*nmemb, (FILE *)stream);
  //printf("**FTP Upload, read %d (%d,%d)\r\n", nread,size,nmemb);
  if (nread <= 0) return 0;

  return nread;
}

// Callback: Print transfer info
//--------------------------------------------------------------------------------------------------------
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
  struct progress *myp = (struct progress *)p;
  CURL *curl = myp->curl;
  double curtime = 0;

  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */
  if ((myp->curl_progress) && ((curtime - myp->lastruntime) >= myp->curl_progress) && ((ulnow > 0) || (dlnow >0))) {
	if (myp->lastruntime == 0) printf("\r\n");
    myp->lastruntime = curtime;
    if (((ultotal > 1000000) || (dltotal > 1000000)) && (myp->curl_progress < 5)) myp->curl_progress = 5;

    printf("* TIME: %0.1f", curtime);
    if ((ulnow > 0) & (ulnow != myp->ulnow)) {
    	myp->ulnow = ulnow;
        printf(" UP: %" CURL_FORMAT_CURL_OFF_T, ulnow);
        if (ultotal > 0) {
            printf(" of %" CURL_FORMAT_CURL_OFF_T, ultotal);
        }
    }
    if ((dlnow > 0) & (dlnow != myp->dlnow)) {
    	myp->dlnow = dlnow;
        printf("  DOWN: %" CURL_FORMAT_CURL_OFF_T, dlnow);
        if (dltotal > 0) {
            printf(" of %" CURL_FORMAT_CURL_OFF_T, dltotal);
        }
    }
    printf("\r\n");
    if ((ulnow > 0) && (ulnow == ultotal)) myp->curl_progress = 0;
    if ((dlnow > 0) && (dlnow == dltotal)) myp->curl_progress = 0;
  }
  vTaskDelay(100 / portTICK_RATE_MS);

  // Check if maximum download size exceeded
  if (dlnow > curl_maxbytes) return 1;

  return 0;
}

// ==== Socket open & close callbacks =====================================================

#if 0
extern int _curl_socket;
#endif

//------------------------------------------------------------------
static int closesocket_callback(void *clientp, curl_socket_t item) {
	int ret = closesocket(item);
	if (curl_verbose) printf("== CLOSE socket %d, ret=%d\r\n", item, ret);
	return ret;
}

//------------------------------------------------------------------------------------------------------------
static curl_socket_t opensocket_callback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address) {
#if 0
	_curl_socket = 1;  // workaround for NET_VFS problem
#endif
	int s = socket(address->family, address->socktype, address->protocol);
#if 0
	_curl_socket = 0;  // workaround for NET_VFS problem
#endif
	if (curl_verbose) printf("== OPEN socket %d: %d,%d,%d\r\n", s, address->family, address->socktype, address->protocol);
	return s;
}

// ========================================================================================

// Set some common curl options
//----------------------------------------------
static void _set_default_options(CURL *handle) {
	curl_easy_setopt(handle, CURLOPT_VERBOSE, curl_verbose);

	// ** Set SSL Options
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(handle, CURLOPT_PROXY_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(handle, CURLOPT_PROXY_SSL_VERIFYHOST, 0L);

	// ==== Provide CA Certs from different file than default ====
	//curl_easy_setopt(handle, CURLOPT_CAINFO, "ca-bundle.crt");
	// ===========================================================

	/*
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER , 1L);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST , 1L);
	*/

	/* If the server is redirected, we tell libcurl if we want to follow redirection
	 * There are some problems when redirecting ssl requests, so for now we disable redirection
	 */
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L);  // set to 1 to enable redirection
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 3L);

    curl_easy_setopt(handle, CURLOPT_TIMEOUT, (long)curl_timeout);

    curl_easy_setopt(handle, CURLOPT_MAXFILESIZE, (long)curl_maxbytes);
    curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 1L);

    //curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1024L);	//bytes/sec
	//curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 4);		//seconds

    // Open&Close socket callbacks are needed because of NET_VFS problem
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, closesocket_callback);
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, NULL);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, NULL);
}


/*
 * Populate POST request based on provided data in Lua table
 */
//--------------------------------------------
static int _preparePost(lua_State* L, int pos)
{
	if ((!lua_istable(L, pos)) && (!lua_isstring(L, pos))) return -1;

	if (lua_istable(L, pos)) {
		// === We have table parameter ===
		// table is in the stack at index 'pos', scan it's content
		lua_pushnil(L);  // first key
		while (lua_next(L, pos) != 0) {
			// uses 'key' (at index -2) and 'value' (at index -1)
			if ((lua_isstring(L, -1)) && (lua_isstring(L, -2))) {
				// only use key-value pairs that are or can be converted to string
				size_t klen = 0;
				size_t vlen = 0;
				const char* key = lua_tolstring(L, -2, &klen);
				const char* value = lua_tolstring(L, -1, &vlen);

				if ((klen > 0) && (vlen > 0)) {
					if (strstr(key, "_FILE_") == key) {
						struct stat sb;
						if ((stat(value, &sb) == 0) && (sb.st_size > 0)) {
							if (klen > 6) {
								char pname[klen-5];
								sprintf(pname, "%s", key+6);
								curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, pname, CURLFORM_FILE, value, CURLFORM_END);
							}
							else curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, value, CURLFORM_END);
						}
					}
					else {
						if (strstr(key, "_JSON_") == key) {
							if (klen > 6) {
								char pname[klen-5];
								sprintf(pname, "%s", key+6);
								curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, pname, CURLFORM_COPYCONTENTS, value, CURLFORM_CONTENTTYPE, "application/json", CURLFORM_END);
							}
							else curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "json", CURLFORM_COPYCONTENTS, value, CURLFORM_CONTENTTYPE, "application/json", CURLFORM_END);
						}
						else {
							curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, key, CURLFORM_COPYCONTENTS, value, CURLFORM_END);
						}
					}
				}
			}
			// removes 'value'; keeps 'key' for next iteration
			lua_pop(L, 1);
		}
	}

	return 0;
}


// ==== Main Lua curl functions ==============================================================

/*
 * ---------------------------------------------------
 * res, hdr,body = net.curl.post(url, tparams)
 * ---------------------------------------------------
 *
 * POST data to http or https server
 *
 * Params:
 * 		    url:	string; server url, if starting with 'https://' SSL will be used
 * 		tparams:	Lua table containing post data in form 'key=value'
 * 					special 'key' values are accepted:
 * 						_FILE_key:	value is the name of the file to be uploaded; if only '_FILE_' is given, 'file' will be used as key
 * 						_JSON_key:	value is json string; if only '_JSON_' is given, 'json' will be used as key
 * Returns:
 * 		res:	boolean, true on success, false on error
 * 		hdr:	received response header or error message on error
 * 	   body:	received response body or error message on error
 *
 * Example:
 * --------
 *
 * 	res, hdr, bdy = net.curl.post("http://loboris.eu/ESP32/test.php", {temp=24, user="esp32"})
 *
 * 	postreq = {
 * 				temp=24,
 * 				_FILE_file="test.jpg",
 * 				_FILE_file2="data.txt",
 * 				_JSON_jsondata="{\"name\":\"John\", \"age\":31}"
 * 	          }
 * 	res, hdr, bdy = net.curl.post("http://loboris.eu/ESP32/test.php", postreq)
 *
 * Note:
 * 	- More than one file can be uploaded
 */

//======================================
static int lnet_curlpost(lua_State* L) {
	CURL *curl;
	CURLcode res;

	const char* url = luaL_checkstring( L, 1 );

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) return luaL_error(L, "Curl global init failed: ", curl_easy_strerror(res));
		curl_initialized = 1;
	}

	struct progress prog;
	struct curl_string get_data;
	struct curl_string get_header;
	if (init_curl_string(&get_data, curl_max_bodylen)) return luaL_error(L, "Failed to initialize curl data");
	if (init_curl_string(&get_header, curl_max_headlen)) {
		free(get_data.ptr);
		return luaL_error(L, "Failed to initialize curl data");
	}

	formpost=NULL;
	lastptr=NULL;
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";


	_preparePost(L, 2);

	curl = curl_easy_init();
	if (curl == NULL) {
		free(get_data.ptr);
		free(get_header.ptr);
		curl_formfree(formpost);
		curl_slist_free_all(headerlist);
		return luaL_error(L, "Failed to create curl handle");
	}

    prog.lastruntime = 0;
    prog.curl_progress = curl_progress;
    prog.ulnow = 0;
    prog.dlnow = 0;
    prog.curl = curl;
	// initialize custom header list (stating that Expect: 100-continue is not wanted
	headerlist = curl_slist_append(headerlist, buf);

	// set URL that receives this POST
	curl_easy_setopt(curl, CURLOPT_URL, url);
	_set_default_options(curl);

	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    // pass the struct pointer into the xferinfo function, note that this is an alias to CURLOPT_PROGRESSDATA
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
    if (curl_progress) curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &get_header);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &get_data);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

	// Perform the request, res will get the return code
	res = curl_easy_perform(curl);
    if (curl_progress) printf("\r\n");
	// Check for errors
    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
    	lua_pushboolean(L, false);
    	lua_pushstring(L, curl_easy_strerror(res));
    	lua_pushstring(L, "error");
    }
    else {
    	if ((get_header.status) || (get_data.status)) {
    		if (curl_verbose) printf("Error: hdr_stat=%d, data_stat=%d\r\n", get_header.status, get_data.status);
        	lua_pushboolean(L, false);
        	if (get_header.status) lua_pushstring(L, "hdr error");
        	else lua_pushstring(L, get_header.ptr);
        	if (get_data.status) lua_pushstring(L, "body error");
        	else lua_pushstring(L, get_data.ptr);
    	}
    	else {
        	lua_pushboolean(L, true);
            // Push header
            lua_pushstring(L, get_header.ptr);
            // Push body, or inform that it is received to file
            lua_pushstring(L, get_data.ptr);
    	}
    }

	free(get_data.ptr);
	free(get_header.ptr);

	// Cleanup
	curl_easy_cleanup(curl);
	curl_formfree(formpost);
	curl_slist_free_all(headerlist);

	return 3;
}

/*
 * ---------------------------------------------------
 * res, hdr,body = net.curl.get(url [, fname])
 * ---------------------------------------------------
 *
 * GET data from http or https server
 *
 * Params:
 * 		   url:	string; server url, if starting with 'https://' SSL will be used
 * 		 fname:	optional; if given response body will be written to the file of that name
 *
 * Returns:
 * 		 res:	boolean, true on success, false on error
 * 		 hdr:	received response header or error message on error
 * 		body:	received response body or error message on error
 *
 * Example:
 * --------
 * 	res, hdr, bdy = net.curl.get("http://loboris.eu/ESP32/test.php?par1=765&par2=test")
 * 	res, hdr, bdy = net.curl.get("http://loboris.eu/ESP32/test.php?name=lobo", "response.txt")
 *
 */

//=====================================
static int lnet_curlget(lua_State* L) {
	CURL *handle;
	CURLcode res;
	uint8_t tofile = 0;
	const char* fname;
	FILE* file = NULL;

	const char* url = luaL_checkstring( L, 1 );
	if (lua_gettop(L) > 1) {
		fname = luaL_checkstring( L, 2 );
		tofile = 1;
	}

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) return luaL_error(L, "Curl global init failed: ", curl_easy_strerror(res));
		curl_initialized = 1;
	}

	struct progress prog;
	struct curl_string get_data;
	struct curl_string get_header;

	if (init_curl_string(&get_data, curl_max_bodylen)) return luaL_error(L, "Failed to initialize curl data");
	if (init_curl_string(&get_header, curl_max_headlen)) {
		free(get_data.ptr);
		return luaL_error(L, "Failed to initialize curl data");
	}

	// Create a curl handle
	handle = curl_easy_init();
	if (handle == NULL) {
		free(get_data.ptr);
		free(get_header.ptr);
		return luaL_error(L, "Failed to create curl handle");
	}

	if (tofile) {
		file = fopen(fname, "wb");
		if (file == NULL) {
			free(get_data.ptr);
			free(get_header.ptr);
			return luaL_error(L, "Error opening file");
		}
	}

    prog.lastruntime = 0;
    prog.curl_progress = curl_progress;
    prog.ulnow = 0;
    prog.dlnow = 0;
    prog.curl = handle;
	curl_easy_setopt(handle, CURLOPT_URL, (char *)url);
	_set_default_options(handle);

	curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
    // pass the struct pointer into the xferinfo function, note that this is an alias to CURLOPT_PROGRESSDATA
    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &prog);
    if (curl_progress) curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "deflate, gzip");

    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &get_header);
	if (tofile) {
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
	}
	else {
	    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWrite);
	    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &get_data);
	}
    // Perform the request, res will get the return code
    res = curl_easy_perform(handle);

    if (curl_progress) printf("\r\n");
    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
    	lua_pushboolean(L, false);
    	lua_pushstring(L, curl_easy_strerror(res));
    	lua_pushstring(L, "error");
    }
    else {
    	if ((get_header.status) || (get_data.status)) {
    		if (curl_verbose) printf("Error: hdr_stat=%d, data_stat=%d\r\n", get_header.status, get_data.status);
        	lua_pushboolean(L, false);
        	if (get_header.status) lua_pushstring(L, "hdr error");
        	else lua_pushstring(L, get_header.ptr);
        	if (get_data.status) lua_pushstring(L, "body error");
        	else {
                if (tofile) lua_pushstring(L, "Received to file.");
                else lua_pushstring(L, get_data.ptr);
        	}
    	}
    	else {
        	lua_pushboolean(L, true);
            // Push header
            lua_pushstring(L, get_header.ptr);
            // Push body, or inform that it is received to file
            if (tofile) lua_pushstring(L, "Received to file.");
            else lua_pushstring(L, get_data.ptr);
    	}
    }

    if (tofile) fclose(file);

    free(get_data.ptr);
	free(get_header.ptr);

	// Cleanup
    curl_easy_cleanup(handle);

    return 3;
}

/*
 * ------------------------------------------------------------------------
 * res, hdr, resp = net.curl.ftp(upload, url, user, pass [, fname])
 * ------------------------------------------------------------------------
 *
 * FTP operations; LIST, GET file, PUT file
 *   if the server supports SSL/TLS, secure transport will be used for login and data transfer
 *
 * Params:
 *		upload: if 0, LIST or GET operations are performed; if 1 file upload is performed
 * 		   url:	string; ftp server url, must strat with 'ftp://' or 'ftps://'
 * 	      user: string; user name to login to ftp server
 * 	      pass: string; user passvord to login to ftp server
 * 		 fname:	optional; string:
 * 		 		when upload=0	if given, LIST or file will be written to the file of that name
 * 		 		when upload=1	if given, file of that name will be PUT on the server
 *
 * Returns:
 * 		 res:	boolean, true on success, false on error
 * 		 hdr:	received response header or error message on error
 * 		resp:	received LIST response or file
 *
 * Example:
 * --------
 *  NOTE: 'ftp://speedtest.tele2.net' is real ftp server which can be used for testing
 *
 *  -- list files in root directory
 * 	res, hdr, resp = net.curl.ftp(0,"ftp://speedtest.tele2.net","anonymous","lua.esp32@gmail.com")
 *
 * 	-- send file to upload directory (file name MUST be given in URL)
 * 	res, hdr, resp = net.curl.ftp(1,"ftp://speedtest.tele2.net/upload/image77.jpg","anonymous","lua.esp32@gmail.com","images/myimage.jpg")
 *
 * 	-- list files in upload directory to check if file is uploaded (trailing slash MUST be given)
 * 	res, hdr, resp = net.curl.ftp(0,"ftp://speedtest.tele2.net/upload","anonymous","lua.esp32@gmail.com")
 *
 *  -- get file back under new name
 * 	res, hdr, resp = net.curl.ftp(0,"ftp://speedtest.tele2.net/upload/image77.jpg","anonymous","lua.esp32@gmail.com","images/newimage.jpg")
 */

//=====================================
static int lnet_curlftp(lua_State* L) {
	CURL *handle;
	CURLcode res;
	uint8_t tofile = 0;
	const char* fname = NULL;
	FILE* file = NULL;
	int fsize = 0;

	// Get parameters
	uint8_t upload = luaL_checkinteger(L, 1) & 1;
	const char* url = luaL_checkstring( L, 2 );
	const char* user = luaL_checkstring( L, 3 );
	const char* pass = luaL_checkstring( L, 4 );
	if (upload) {
		fname = luaL_checkstring( L, 5 );
		tofile = 1;
	}
	else {
		if (lua_gettop(L) > 4) {
			fname = luaL_checkstring( L, 5 );
			tofile = 1;
		}
	}

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) return luaL_error(L, "Curl global init failed: ", curl_easy_strerror(res));
		curl_initialized = 1;
	}

	struct progress prog;
	struct curl_string get_data;
	struct curl_string get_header;
	//struct curl_slist *headerlist = NULL;

	if (init_curl_string(&get_data, curl_max_bodylen)) return luaL_error(L, "Failed to initialize curl data");
	if (init_curl_string(&get_header, curl_max_headlen)) {
		free(get_data.ptr);
		return luaL_error(L, "Failed to initialize curl data");
	}

	// Create a curl handle
	handle = curl_easy_init();
	if (handle == NULL) {
		free(get_data.ptr);
		free(get_header.ptr);
		curl_global_cleanup();
		return luaL_error(L, "Failed to create curl handle");
	}

	if (tofile) {
		if (upload) {
			// Uploading the file
			struct stat sb;
			if ((stat(fname, &sb) == 0) && (sb.st_size > 0)) {
				fsize = sb.st_size;
				file = fopen(fname, "rb");
			}
		}
		else {
			// Downloading to file (LIST or Get file)
			file = fopen(fname, "wb");
		}
		if (file == NULL) {
			free(get_data.ptr);
			free(get_header.ptr);
			curl_easy_cleanup(handle);
			return luaL_error(L, "Error opening file");
		}
	}

    prog.lastruntime = 0;
    prog.curl_progress = curl_progress;
    prog.ulnow = 0;
    prog.dlnow = 0;
    prog.curl = handle;
    /// build a list of commands to pass to libcurl
    //headerlist = curl_slist_append(headerlist, "QUIT");

    curl_easy_setopt(handle, CURLOPT_URL, (char *)url);
	_set_default_options(handle);

	// set authentication credentials
    curl_easy_setopt(handle, CURLOPT_USERNAME, user);
    curl_easy_setopt(handle, CURLOPT_PASSWORD, pass);

    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
    // pass the struct pointer into the xferinfo function, note that this is an alias to CURLOPT_PROGRESSDATA
    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &prog);
    if (curl_progress) curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &get_header);
    if (upload) {
        // we want to use our own read function
        curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_callback);
        // specify which file to upload
        curl_easy_setopt(handle, CURLOPT_READDATA, (void *)file);
        // enable uploading
        curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L);

	    curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, (long)fsize);
    }
    else {
		if (tofile) {
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
		}
		else {
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWrite);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, &get_data);
		}
    }
	//curl_easy_setopt(handle, CURLOPT_POSTQUOTE, headerlist);
	curl_easy_setopt(handle, CURLOPT_FTP_USE_EPSV, 0L);
	curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_TRY);

    curl_easy_setopt(handle, CURLOPT_TIMEOUT, (long)curl_timeout);

	// Perform the request, res will get the return code
    res = curl_easy_perform(handle);

    if (curl_progress) printf("\r\n");
    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
    	lua_pushboolean(L, false);
    	lua_pushstring(L, curl_easy_strerror(res));
    	lua_pushstring(L, "error");
    }
    else {
    	if ((get_header.status) || (get_data.status)) {
    		if (curl_verbose) printf("Error: hdr_stat=%d, data_stat=%d\r\n", get_header.status, get_data.status);
        	lua_pushboolean(L, false);
        	if (get_header.status) lua_pushstring(L, "hdr error");
        	else lua_pushstring(L, get_header.ptr);
        	if (get_data.status) lua_pushstring(L, "body error");
        	else {
                if ((upload == 0) && (tofile)) lua_pushstring(L, "Received to file.");
                else lua_pushstring(L, get_data.ptr);
        	}
    	}
    	else {
        	lua_pushboolean(L, true);
            // Push header
            lua_pushstring(L, get_header.ptr);
            // Push body, or inform that it is received to file
            if ((upload == 0) && (tofile)) lua_pushstring(L, "Received to file.");
            else lua_pushstring(L, get_data.ptr);
    	}
    }

    if (tofile) fclose(file);

    free(get_data.ptr);
	free(get_header.ptr);

	// clean up the FTP commands list
	//curl_slist_free_all(headerlist);

	curl_easy_cleanup(handle);

    return 3;
}

/*
 * Send email
 * Default SMTP server is 'smtp.gmail.com', it can be changed using net.curl.mailserver() function
 *
 * ret,msg = net.s.curl.sendmail(options)
 *
 * Params:
 * 		options:	Lua table containing email options in the following format:
 * 					user=<user_name>		string; login name for SMPT server
 * 					pass=<password>			string; user password for SMTP server
 * 					to=<recipient>			string; recipient's email address
 * 					subj=<subject>			OPTIONAL; string; email subject; DEFAULT: "LUA_RTOS_ESP32"
 * 					msg=<subject>			OPTIONAL; string; email message; DEFAULT: "Message from Lua_RTOS_ESP32"
 * 					secure=<true|false>		OPTIONAL; boolean; if true send using SSL/TLS (DEFAULT: true)
 * 					smtp=<server>			OPTIONAL; string; SMTP server used only for this mail; DEFAULT: use global
 * 					port=<port>				OPTIONAL; integer; SMTP server's port used only for this mail; DEFAULT: use global
 * 					cc=<CC recipient(s)>	OPTIONAL; string or Lua table; CC recipient's email addresses
 * 					attach=<attachment(s)>	OPTIONAL; string or Lua table; file name(s) of the attachment(s)
 *
 * 	Returns:
 * 		ret:	boolean; true on success, false on error
 * 		stat:	string; error message on error, "OK" on success
 *
 *  Example:
 *  	opt = {user="luaesp32@gmail.com", pass="luaESP32password", to="myfriend@gmail.com"}
 *  	ret,stat = net.curl.sendmail(opt)
 *
 *  	opt = {
 *  			user = "luaesp32@gmail.com",
 *  			pass = "luaESP32password",
 *  			to = {"myfriend@gmail.com","otherfriend@gmail.com"},
 *  			subj = "Test",
 *  			msg = "Testing send mail from Lua_RTOS_ESP32",
 *  			secure = true,
 *  			attach = {"picture.jpg", "sd/images/image7.bmp"}
 *  		  }
 *  	ret,stat = net.curl.sendmail(opt)
 *
 *		-- reuse 'opt' for another mail
 *  	opt.cc = "forgotten@gmail.com"
 *  	opt.subj = "2nd test"
 *  	ret,stat = net.curl.sendmail(opt)
 */
//======================================
static int lnet_sendmail(lua_State* L) {
	if (!curl_initialized) {
		CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) return luaL_error(L, "Curl global init failed: ", curl_easy_strerror(res));
		curl_initialized = 1;
	}

	if (!lua_istable(L, 1)) {
		return luaL_error(L, "Table parameter expected!");
	}

	size_t len = 0;
	quickmail mailobj = NULL;
	uint8_t protocol = QUICKMAIL_PROT_SMTPS;

	quickmail_verbose = curl_verbose;
	quickmail_progress = curl_progress;
	quickmail_timeout = curl_timeout;

	const char* user;
	const char* pass;
	const char* tmpstr;
	const char* errmsg = NULL;
	char *smtp_server = mail_server;
	int port = mail_port;
	struct stat sb;

	// ============================
	// === Get email parameters ===

	// Check user name
	lua_getfield(L, 1, "user");
	if ((lua_isnil(L, -1)) || (!lua_isstring(L, -1))) {
		errmsg = "user name not found";
		goto exit;
	}
	user = luaL_checklstring( L, -1, &len );

	// Check User password
	lua_getfield(L, 1, "pass");
	if ((lua_isnil(L, -1)) || (!lua_isstring(L, -1))) {
		errmsg = "user password not found";
		goto exit;
	}
	pass = luaL_checklstring( L, -1, &len );

	// Check email subject (OPTIONAL)
	lua_getfield(L, 1, "subj");
	if ((!lua_isnil(L, -1)) && (lua_isstring(L, -1))) {
		tmpstr = luaL_checklstring( L, -1, &len );
	}
	else tmpstr = "LUA_RTOS_ESP32";

	// Check recipient's email
	lua_getfield(L, 1, "to");
	if ((lua_isnil(L, -1)) || ((!lua_isstring(L, -1)) && (!lua_istable(L, -1)))) {
		errmsg = "recipient's email not found";
		goto exit;
	}

	// Create quickmail object
	mailobj =  quickmail_create(user, tmpstr);
	if (mailobj == NULL) {
		return luaL_error(L, "Error creating mail object!");
	}

	// 'to' field is on top of the stack, add recipient(s)
	if (lua_istable(L, -1)) {
		int argn = lua_rawlen(L, -1);
		for (int i = 0; i < argn; i++) {
			lua_rawgeti(L, -1, i + 1);
			if (lua_type(L, -1) == LUA_TSTRING) {
				tmpstr = luaL_checklstring( L, -1, &len );
				lua_pop(L, 1);
				quickmail_add_to(mailobj, tmpstr);
			}
			else lua_pop(L, 1);
		}
	}
	else if (lua_isstring(L, -1)) {
		tmpstr = luaL_checklstring( L, -1, &len );
		quickmail_add_to(mailobj, tmpstr);
	}

	// Check and add CC recipient(s) (OPTIONAL)
	lua_getfield(L, 1, "cc");
	if ((!lua_isnil(L, -1)) && ((lua_isstring(L, -1)) || (lua_istable(L, -1)))) {
		if (lua_istable(L, -1)) {
			int argn = lua_rawlen(L, -1);
			for (int i = 0; i < argn; i++) {
				lua_rawgeti(L, -1, i + 1);
				if (lua_type(L, -1) == LUA_TSTRING) {
					tmpstr = luaL_checklstring( L, -1, &len );
					lua_pop(L, 1);
					quickmail_add_cc(mailobj, tmpstr);
				}
				else lua_pop(L, 1);
			}
		}
		else if (lua_isstring(L, -1)) {
			tmpstr = luaL_checklstring( L, -1, &len );
			quickmail_add_cc(mailobj, tmpstr);
		}
	}

	// Check and add email body (OPTIONAL)
	lua_getfield(L, 1, "msg");
	if ((!lua_isnil(L, -1)) && (lua_isstring(L, -1))) {
		tmpstr = luaL_checklstring( L, -1, &len );
	}
	else tmpstr = "Message from Lua_RTOS_ESP32";
	quickmail_set_body(mailobj, tmpstr);

	// Check SSL/TLS option (OPTIONAL)
	lua_getfield(L, 1, "secure");
	if ((!lua_isnil(L, -1)) && (lua_isboolean(L, -1))) {
		protocol = lua_toboolean( L, -1 );
		if (protocol) protocol = QUICKMAIL_PROT_SMTPS;
		else protocol = QUICKMAIL_PROT_SMTP;
	}

	// Check if SMTP server is given (OPTIONAL)
	lua_getfield(L, 1, "smtp");
	if ((!lua_isnil(L, -1)) && (lua_isstring(L, -1))) {
		smtp_server = (char *)luaL_checklstring( L, -1, &len );
	}

	// Check if SMTP server port is given (OPTIONAL)
	lua_getfield(L, 1, "port");
	if ((!lua_isnil(L, -1)) && (lua_isinteger(L, -1))) {
		port = luaL_checkinteger( L, -1 );
		if (port < 0) port = mail_port;
	}

	// Check and add attachment file(s) (OPTIONAL)
	lua_getfield(L, 1, "attach");
	if ((!lua_isnil(L, -1)) && ((lua_isstring(L, -1)) || (lua_istable(L, -1)))) {
		if (lua_istable(L, -1)) {
			int argn = lua_rawlen(L, -1);
			for (int i = 0; i < argn; i++) {
				lua_rawgeti(L, -1, i + 1);
				if (lua_type(L, -1) == LUA_TSTRING) {
					tmpstr = luaL_checklstring( L, -1, &len );
					lua_pop(L, 1);
					// check if file exists and it's length is greater than zero
					if ((stat(tmpstr, &sb) == 0) && (sb.st_size > 0)) {
						quickmail_add_attachment_file(mailobj, tmpstr, NULL);
					}
				}
				else lua_pop(L, 1);
			}
		}
		else if (lua_isstring(L, -1)) {
			tmpstr = luaL_checklstring( L, -1, &len );
			// check if file exists and it's length is greater than zero
			if ((stat(tmpstr, &sb) == 0) && (sb.st_size > 0)) {
				quickmail_add_attachment_file(mailobj, tmpstr, NULL);
			}
		}
	}

	// ============================

	quickmail_add_header(mailobj, "Importance: Low");
	quickmail_add_header(mailobj, "X-Priority: 5");
	quickmail_add_header(mailobj, "X-MSMail-Priority: Low");
	quickmail_set_debug_log(mailobj, stderr);

	// ** Send email **
	errmsg = quickmail_protocol_send(mailobj, smtp_server, port, protocol, user, pass);

	// Cleanup
	quickmail_destroy(mailobj);
	quickmail_cleanup();

exit:
	if (errmsg) {
		lua_pushboolean(L, false);
		lua_pushstring(L, errmsg);
	}
	else {
		lua_pushboolean(L, true);
		lua_pushstring(L, "OK");
	}

	return 2;
}


// ==== Various configuration, sutup and info function =======================================


// Set or get verbose level
// When set to 0 no info is shown
// When set to 1, verbose information of curl operations is printed
//=========================================
static int lnet_curlverbose(lua_State* L) {
	if (lua_gettop(L) > 0) {
		uint8_t v = luaL_checkinteger(L, 1) & 1;
		curl_verbose = v;
	}
	lua_pushinteger(L, curl_verbose);
	return 1;
}

// n = net.curlprogress([n])
// Set or get curl operation progress info in seconds
// Progress is shown every n seconds
// when set to 0, no progress info is shown
//==========================================
static int lnet_curlprogress(lua_State* L) {
	if (lua_gettop(L) > 0) {
		uint8_t p = luaL_checkinteger(L, 1);
		if (p > 20) p = 5;
		curl_progress = p;
	}
	lua_pushinteger(L, curl_progress);
	return 1;
}

// Set or get curl timeout value in seconds
// Default is 60 sec, set to larger value when sending/receiving large files
//=========================================
static int lnet_curltimeout(lua_State* L) {
	if (lua_gettop(L) > 0) {
		uint16_t t = luaL_checkinteger(L, 1);
		if (t > 300) t = 180;
		curl_timeout = t;
	}
	lua_pushinteger(L, curl_timeout);
	return 1;
}

// Set or get receive limit in bytes
//==========================================
static int lnet_curlreclimit(lua_State* L) {
	if (lua_gettop(L) > 0) {
		uint32_t mb = luaL_checkinteger(L, 1);
		if (mb > (3*1024*1024)) mb = 300*1024;
		curl_maxbytes = mb;
	}
	lua_pushinteger(L, curl_maxbytes);
	return 1;
}

// Set default SMTP(S) server and port
//========================================
static int lnet_mailserver(lua_State* L) {
	if (lua_gettop(L) > 1) {
		if ((lua_isstring(L, 1)) && (lua_isinteger(L, 2))) {
			size_t len = 0;
			const char* server = lua_tolstring(L, 1, &len);
			int port = luaL_checkinteger(L, 2);
			if (((len > 3) && (len < 64)) && ((port > 0) && (port < 65536))) {
				sprintf(mail_server, "%s", server);
				mail_port = port;
			}
			else return luaL_error(L, "Wrong parameter given!");
		}
	}
	lua_pushstring(L, mail_server);
	lua_pushinteger(L, mail_port);
	return 2;
}

// Free some memory after using curl
//==========================================
static int lnet_curl_cleanup(lua_State* L) {
	if (curl_initialized) {
		curl_global_cleanup();
		curl_initialized = 0;
	}
	return 0;
}

// Return curl version & print info if reguested
//======================================
static int lnet_curlinfo(lua_State* L) {
	uint8_t info = 0;

	if (lua_gettop(L) > 0) {
		if (lua_isboolean(L, 1)) {
			info = lua_toboolean(L, 1);
		}
	}

	curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
	if (info) {
		printf("Curl version info\r\n");
		printf("  version: %s - %d\r\n", data->version, data->version_num);
		printf("Host: %s\r\n", data->host);
		if (data->features & CURL_VERSION_IPV6) {
			printf("- IP V6 supported\r\n");
		} else {
			printf("- IP V6 NOT supported\r\n");
		}
		if (data->features & CURL_VERSION_SSL) {
			printf("- SSL supported\r\n");
		} else {
			printf("- SSL NOT supported\r\n");
		}
		if (data->features & CURL_VERSION_LIBZ) {
			printf("- LIBZ supported\r\n");
		} else {
			printf("- LIBZ NOT supported\r\n");
		}
		if (data->features & CURL_VERSION_NTLM) {
			printf("- NTLM supported\r\n");
		} else {
			printf("- NTLM NOT supported\r\n");
		}
		if (data->features & CURL_VERSION_DEBUG) {
			printf("- DEBUG supported\r\n");
		} else {
			printf("- DEBUG NOT supported\r\n");
		}
		if (data->features & CURL_VERSION_UNIX_SOCKETS) {
			printf("- UNIX sockets supported\r\n");
		} else {
			printf("- UNIX sockets NOT supported\r\n");
		}
		printf("Protocols:\r\n");
		int i=0;
		while(data->protocols[i] != NULL) {
			printf("- %s\r\n", data->protocols[i]);
			i++;
		}
	}

	lua_pushstring(L, data->version);
	lua_pushinteger(L, data->version_num);

    return 2;
}

//======================================
static const LUA_REG_TYPE curl_map[] = {
	{ LSTRKEY( "get"        ),	 LFUNCVAL( lnet_curlget      ) },
	{ LSTRKEY( "post"       ),	 LFUNCVAL( lnet_curlpost     ) },
	{ LSTRKEY( "ftp"        ),	 LFUNCVAL( lnet_curlftp      ) },
	{ LSTRKEY( "sendmail"   ),	 LFUNCVAL( lnet_sendmail     ) },
	{ LSTRKEY( "mailserver" ),	 LFUNCVAL( lnet_mailserver   ) },
	{ LSTRKEY( "cleanup"    ),	 LFUNCVAL( lnet_curl_cleanup ) },
	{ LSTRKEY( "info"       ),	 LFUNCVAL( lnet_curlinfo     ) },
	{ LSTRKEY( "verbose"    ),	 LFUNCVAL( lnet_curlverbose  ) },
	{ LSTRKEY( "progress"   ),	 LFUNCVAL( lnet_curlprogress ) },
	{ LSTRKEY( "timeout"    ),	 LFUNCVAL( lnet_curltimeout  ) },
	{ LSTRKEY( "reclimit"   ),	 LFUNCVAL( lnet_curlreclimit ) },
	{ LNILKEY, LNILVAL }
};

#endif
