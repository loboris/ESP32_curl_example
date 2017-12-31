

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "espcurl.h"


// Set default values for configuration variables
uint8_t curl_verbose = 1;        // show detailed info of what curl functions are doing
uint8_t curl_progress = 1;       // show progress during transfers
uint16_t curl_timeout = 60;      // curl operations timeout in seconds
uint32_t curl_maxbytes = 300000; // limit download length
uint8_t curl_initialized = 0;

static uint8_t curl_sim_fs = 0;


struct curl_Transfer {
	char *ptr;
	uint32_t len;
	uint32_t size;
	int status;
	uint8_t tofile;
	uint16_t maxlen;
	double lastruntime;
	FILE *file;
	CURL *curl;
};

struct curl_httppost *formpost = NULL;
struct curl_httppost *lastptr = NULL;


// Initialize the structure used in curlWrite callback
//-----------------------------------------------------------------------------------------------------------
static void init_curl_Transfer(CURL *curl, struct curl_Transfer *s, char *buf, uint16_t maxlen, FILE* file) {
    s->len = 0;
    s->size = 0;
    s->status = 0;
    s->maxlen = maxlen;
    s->lastruntime = 0;
    s->tofile = 0;
    s->file = file;
    s->ptr = buf;
    s->curl = curl;
    if (s->ptr) s->ptr[0] = '\0';
}

// Callback: Get response header or body to buffer or file
//--------------------------------------------------------------------------------
static size_t curlWrite(void *buffer, size_t size, size_t nmemb, void *userdata) {
	struct curl_Transfer *s = (struct curl_Transfer *)userdata;
	CURL *curl = s->curl;
	double curtime = 0;
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

	if (!s->tofile) {
		// === Downloading to buffer ===
		char *buf = (char *)buffer;
		if (s->ptr) {
			if ((s->status == 0) && ((size*nmemb) > 0)) {
				for (int i=0; i<(size*nmemb); i++) {
					if (s->len < (s->maxlen-2)) {
						if (((buf[i] == 0x0a) || (buf[i] == 0x0d)) || ((buf[i] >= 0x20) && (buf[i] >= 0x20))) s->ptr[s->len] = buf[i];
						else s->ptr[s->len] = '.';
						s->len++;
						s->ptr[s->len] = '\0';
					}
					else {
						s->status = 1;
						break;
					}
				}
				if ((curl_progress) && ((curtime - s->lastruntime) > curl_progress)) {
					s->lastruntime = curtime;
					printf("* Download: received %d\r\n", s->len);
				}
			}
		}
		return size*nmemb;
	}
	else {
		// === Downloading to file ===
		size_t nwrite;

		if (curl_sim_fs) nwrite = size*nmemb;
		else {
			nwrite = fwrite(buffer, 1, size*nmemb, s->file);
			if (nwrite <= 0) {
				printf("* Download: Error writing to file %d\r\n", nwrite);
				return 0;
			}
		}

		s->len += nwrite;
		if ((curl_progress) && ((curtime - s->lastruntime) > curl_progress)) {
			s->lastruntime = curtime;
			printf("* Download: received %d\r\n", s->len);
		}

		return nwrite;
	}
}

// Callback: ftp PUT file
//--------------------------------------------------------------------------
static size_t curlRead(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct curl_Transfer *s = (struct curl_Transfer *)userdata;
	CURL *curl = s->curl;
	double curtime = 0;
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
	size_t nread;

	if (curl_sim_fs) {
		if (s->len < 1024) {
			size_t reqsize = size*nmemb;
			nread = 0;
			while ((nread+23) < reqsize) {
				sprintf((char *)(ptr+nread), "%s", "Simulate upload data\r\n");
				nread += 22;
			}
		}
		else nread = 0;
	}
	else {
		nread = fread(ptr, 1, size*nmemb, s->file);
	}

	s->len += nread;
	if ((curl_progress) && ((curtime - s->lastruntime) > curl_progress)) {
		s->lastruntime = curtime;
		printf("* Upload: sent %d\r\n", s->len);
	}

	//printf("**Upload, read %d (%d,%d)\r\n", nread,size,nmemb);
	if (nread <= 0) return 0;

	return nread;
}

//------------------------------------------------------------------
static int closesocket_callback(void *clientp, curl_socket_t item) {
    int ret = close(item);
    if (ret) printf("== CLOSE socket %d, ret=%d\r\n", item, ret);
    return ret;
}

//------------------------------------------------------------------------------------------------------------
static curl_socket_t opensocket_callback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address) {
    int s = socket(address->family, address->socktype, address->protocol);
    if (s < 0) printf("== OPEN socket error %d\r\n", s);
    return s;
}


// Set some common curl options
//----------------------------------------------
static void _set_default_options(CURL *handle) {

    //curl_easy_setopt(handle, CURLOPT_TCP_NODELAY, 0L);

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
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);

    //curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1024L);	//bytes/sec
	//curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 4);		//seconds

    // Open&Close socket callbacks can be set if special handling is needed

    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, closesocket_callback);
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, NULL);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, NULL);
}

//==================================================================================
int Curl_GET(char *url, char *fname, char *hdr, char *body, int hdrlen, int bodylen)
{
	CURL *curl = NULL;
	CURLcode res = 0;
	FILE* file = NULL;
    int err = 0;

    if ((hdr) && (hdrlen < MIN_HDR_BODY_BUF_LEN)) {
        err = -1;
        goto exit;
    }
    if ((body) && (bodylen < MIN_HDR_BODY_BUF_LEN)) {
        err = -2;
        goto exit;
    }

    struct curl_Transfer get_data;
    struct curl_Transfer get_header;

	if (!url) {
        err = -3;
        goto exit;
    }

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) {
            err = -4;
            goto exit;
        }
		curl_initialized = 1;
	}

	// Create a curl curl
	curl = curl_easy_init();
	if (curl == NULL) {
        err = -5;
        goto exit;
	}

    init_curl_Transfer(curl, &get_data, body, bodylen, NULL);
    init_curl_Transfer(curl, &get_header, hdr, hdrlen, NULL);

	if (fname) {
		if (strcmp(fname, "simulate") == 0) {
			get_data.tofile = 1;
			curl_sim_fs = 1;
		}
		else {
			file = fopen(fname, "wb");
			if (file == NULL) {
				err = -6;
				goto exit;
			}
			get_data.file = file;
			get_data.tofile = 1;
			curl_sim_fs = 0;
		}
	}

    curl_easy_setopt(curl, CURLOPT_URL, url);

    _set_default_options(curl);

    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate, gzip");

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &get_header);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &get_data);

	// Perform the request, res will get the return code
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
		if (body) snprintf(body, bodylen, "%s", curl_easy_strerror(res));
        err = -7;
        goto exit;
    }

    if (get_data.tofile) {
    	if (curl_progress) {
			double curtime = 0;
			curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
			printf("* Download: received %d B; time=%0.1f s; speed=%0.1f KB/sec\r\n", get_data.len, curtime, (float)(((get_data.len*10)/curtime) / 10240.0));
    	}
		if (body) {
			if (strcmp(fname, "simulate") == 0) snprintf(body, bodylen, "SIMULATED save to file; size=%d", get_data.len);
			else snprintf(body, bodylen, "Saved to file %s, size=%d", fname, get_data.len);
		}
    }

exit:
	// Cleanup
    if (file) fclose(file);
    if (curl) curl_easy_cleanup(curl);

    return err;
}


//=======================================================================
int Curl_POST(char *url , char *hdr, char *body, int hdrlen, int bodylen)
{
	CURL *curl = NULL;
	CURLcode res = 0;
    int err = 0;
	struct curl_slist *headerlist=NULL;
	const char hl_buf[] = "Expect:";

    if ((hdr) && (hdrlen < MIN_HDR_BODY_BUF_LEN)) {
        err = -1;
        goto exit;
    }
    if ((body) && (bodylen < MIN_HDR_BODY_BUF_LEN)) {
        err = -2;
        goto exit;
    }

    struct curl_Transfer get_data;
    struct curl_Transfer get_header;

	if (!url) {
        err = -3;
        goto exit;
    }

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) {
            err = -4;
            goto exit;
        }
		curl_initialized = 1;
	}

	// Create a curl curl
	curl = curl_easy_init();
	if (curl == NULL) {
        err = -5;
        goto exit;
	}

    init_curl_Transfer(curl, &get_data, body, bodylen, NULL);
    init_curl_Transfer(curl, &get_header, hdr, hdrlen, NULL);

    // initialize custom header list (stating that Expect: 100-continue is not wanted
	headerlist = curl_slist_append(headerlist, hl_buf);

	// set URL that receives this POST
	curl_easy_setopt(curl, CURLOPT_URL, url);

    _set_default_options(curl);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &get_header);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &get_data);

	if (formpost) curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

	// Perform the request, res will get the return code
	res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
		if (body) snprintf(body, bodylen, "%s", curl_easy_strerror(res));
        err = -7;
        goto exit;
    }

exit:
	// Cleanup
    if (curl) curl_easy_cleanup(curl);
	if (formpost) {
		curl_formfree(formpost);
		formpost = NULL;
	}
	curl_slist_free_all(headerlist);

    return err;
}


//=========================================================================================================================================
static int _Curl_FTP(uint8_t upload, char *url, char *user_pass, char *fname, char *hdr, char *body, int hdrlen, int bodylen, uint8_t sftp)
{
	CURL *curl = NULL;
	CURLcode res = 0;
    int err = 0;
	FILE* file = NULL;
	int fsize = 0;

    if ((hdr) && (hdrlen < MIN_HDR_BODY_BUF_LEN)) {
        err = -1;
        goto exit;
    }
    if ((body) && (bodylen < MIN_HDR_BODY_BUF_LEN)) {
        err = -2;
        goto exit;
    }

    struct curl_Transfer get_data;
    struct curl_Transfer get_header;

	if ((!url) || (!user_pass)) {
        err = -3;
        goto exit;
    }

	if (!curl_initialized) {
		res = curl_global_init(CURL_GLOBAL_DEFAULT);
		if (res) {
            err = -4;
            goto exit;
        }
		curl_initialized = 1;
	}

	// Create a curl curl
	curl = curl_easy_init();
	if (curl == NULL) {
        err = -5;
        goto exit;
	}

    init_curl_Transfer(curl, &get_data, body, bodylen, NULL);
    init_curl_Transfer(curl, &get_header, hdr, hdrlen, NULL);

	if (fname) {
		if (strcmp(fname, "simulate") == 0) {
			get_data.tofile = 1;
			curl_sim_fs = 1;
		}
		else {
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
	            err = -6;
	            goto exit;
			}
			get_data.file = file;
			get_data.tofile = 1;
			get_data.size = fsize;
			curl_sim_fs = 0;
		}
	}

    curl_easy_setopt(curl, CURLOPT_URL, url);

    _set_default_options(curl);

	// set authentication credentials
    if (sftp) {
    	curl_easy_setopt(curl, CURLOPT_PASSWORD, user_pass);
        curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
    }
    else {
        /// build a list of commands to pass to libcurl
        //headerlist = curl_slist_append(headerlist, "QUIT");
    	curl_easy_setopt(curl, CURLOPT_USERPWD, user_pass);

    	//curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
    	curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0L);
    	curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    }

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &get_header);
    if (upload) {
        // we want to use our own read function
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, curlRead);
        // specify which file to upload
        curl_easy_setopt(curl, CURLOPT_READDATA, &get_data);
        // enable uploading
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	    if (fsize > 0) curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)fsize);
    }
    else {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &get_data);
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)curl_timeout);

	// Perform the request, res will get the return code
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
    	if (curl_verbose) printf("curl_easy_perform failed: %s\r\n", curl_easy_strerror(res));
		if (body) snprintf(body, bodylen, "%s", curl_easy_strerror(res));
        err = -7;
        goto exit;
    }

    if (get_data.tofile) {
    	if (curl_progress) {
			double curtime = 0;
			curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
			if (upload) printf("* Upload: sent");
			else printf("* Download: received");
			printf(" %d B; time=%0.1f s; speed=%0.1f KB/sec\r\n", get_data.len, curtime, (float)(((get_data.len*10)/curtime) / 10240.0));
    	}

		if (body) {
	        if (upload) {
				if (strcmp(fname, "simulate") == 0) snprintf(body, bodylen, "SIMULATED upload from file; size=%d", get_data.len);
				else snprintf(body, bodylen, "Uploaded file %s, size=%d", fname, fsize);
	        }
	        else {
				if (strcmp(fname, "simulate") == 0) snprintf(body, bodylen, "SIMULATED download to file; size=%d", get_data.len);
				else snprintf(body, bodylen, "Downloaded to file %s, size=%d", fname, get_data.len);
	        }
		}
    }

exit:
	// Cleanup
    if (file) fclose(file);
    if (curl) curl_easy_cleanup(curl);

    return err;
}

//-------------------------------------------------------------------------------------------------------------------
int Curl_FTP(uint8_t upload, char *url, char *user_pass, char *fname, char *hdr, char *body, int hdrlen, int bodylen)
{
	return _Curl_FTP(upload, url, user_pass, fname, hdr, body, hdrlen, bodylen, 0);
}

//---------------------------------------------------------------------------------------------------------------
int Curl_SFTP(uint8_t upload, char *url, char *pass, char *fname, char *hdr, char *body, int hdrlen, int bodylen)
{
	return _Curl_FTP(upload, url, pass, fname, hdr, body, hdrlen, bodylen, 1);
}

//-------------------
void Curl_cleanup() {
	if (curl_initialized) {
		curl_global_cleanup();
		curl_initialized = 0;
	}
}
