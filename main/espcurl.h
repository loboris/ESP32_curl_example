

#ifndef _ESPCURL_H_
#define _ESPCURL_H_

#include <stdint.h>
#include "curl/curl.h"
//#include "quickmail/quickmail.h"

#define MIN_HDR_BODY_BUF_LEN  1024
#define GMAIL_SMTP  "smtp.gmail.com";
#define GMAIL_PORT  465;


// Some configuration variables
uint8_t curl_verbose;   // show detailed info of what curl functions are doing
uint8_t curl_progress;  // show progress during transfers
uint16_t curl_timeout;  // curl operations timeout in seconds
uint32_t curl_maxbytes; // limit download length
uint8_t curl_initialized;


struct curl_httppost *formpost;
struct curl_httppost *lastptr;


// ================
// Public functions
// ================

/*
 * ----------------------------------------------------------
 * int res = Curl_GET(url, fname, hdr, body, hdrlen, bodylen)
 * ----------------------------------------------------------
 *
 * GET data from http or https server
 *
 * Params:
 * 		   url:	pointer to server url, if starting with 'https://' SSL will be used
 * 		 fname:	pointer to file name; if not NULL response body will be written to the file of that name
 * 		   hdr:	pointer to char buffer to which the received response header or error message will be written
 * 		  body:	pointer to char buffer to which the received response body or error message will be written
 *      hdrlen: length of the hdr buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *     bodylen: length of the body buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *
 * Returns:
 * 		 res:	0 success, error code on error
 *
 */
//===================================================================================
int Curl_GET(char *url, char *fname, char *hdr, char *body, int hdrlen, int bodylen);


/*
 * -----------------------------------------------------
 * int res = Curl_POST(url, hdr, body, hdrlen, bodylen);
 * -----------------------------------------------------
 *
 * POST data to http or https server
 *
 * Params:
 * 		   url:	pointer to server url, if starting with 'https://' SSL will be used
 * 		   hdr:	pointer to char buffer to which the received response header or error message will be written
 * 		  body:	pointer to char buffer to which the received response body or error message will be written
 *      hdrlen: length of the hdr buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *     bodylen: length of the body buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *
 * Returns:
 * 		 res:	0 success, error code on error
 * 
 * NOTE:
 *      Before calling this function, POST data has to be set using curl_formadd() function
 *      If adding the text parameter use the format:
 *          curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "key_string", CURLFORM_COPYCONTENTS, "value_string", CURLFORM_END);
 *      If adding the file, use the format:
 *          curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file_key_string", CURLFORM_FILE, "file_name", CURLFORM_END);
 * 
 */
//=======================================================================
int Curl_POST(char *url, char *hdr, char *body, int hdrlen, int bodylen);


/*
 * ------------------------------------------------------------------------------
 * int res = Curl_FTP(upload, url, user_pass, fname, hdr, body, hdrlen, bodylen);
 * ------------------------------------------------------------------------------
 *
 * FTP operations; LIST, GET file, PUT file
 *   if the server supports SSL/TLS, secure transport will be used for login and data transfer
 *
 * Params:
 * 		   url:	pointer to server url, if starting with 'https://' SSL will be used
 * 	 user_pass:	pointer to user name and password in the format "user:password"
 * 		   hdr:	pointer to char buffer to which the received response header or error message will be written
 * 		  body:	pointer to char buffer to which the received response body or error message will be written
 *      hdrlen: length of the hdr buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *     bodylen: length of the body buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 * 		 fname: pointer to the file name
 *              IF NOT NULL && upload=0, LIST or file will be written to the file of that name
 *              IF upload=1	file of that name will be PUT on the server
 *
 * Returns:
 * 		 res:	0 success, error code on error
 *
 */
//====================================================================================================================
int Curl_FTP(uint8_t upload, char *url, char *user_pass, char *fname, char *hdr, char *body, int hdrlen, int bodylen);


/*
 * --------------------------------------------------------------------------
 * int res = Curl_SFTP(upload, url, pass, fname, hdr, body, hdrlen, bodylen);
 * --------------------------------------------------------------------------
 *
 * SFTP operations; DOWNLOAD or UPLOAD file from/to sftp server
 *
 * Params:
 * 		   url:	pointer to server url in format 'sftp:user@<server_name>:port'
 * 	 	  pass:	pointer to user password in the for the user specified in URL
 * 		   hdr:	pointer to char buffer to which the received response header or error message will be written
 * 		  body:	pointer to char buffer to which the received response body or error message will be written
 *      hdrlen: length of the hdr buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 *     bodylen: length of the body buffer, must be greather than MIN_HDR_BODY_BUF_LEN
 * 		 fname: pointer to the file name
 *              IF NOT NULL && upload=0, LIST or file will be written to the file of that name
 *              IF upload=1	file of that name will be PUT on the server
 *
 * Returns:
 * 		 res:	0 success, error code on error
 *
 */
//================================================================================================================
int Curl_SFTP(uint8_t upload, char *url, char *pass, char *fname, char *hdr, char *body, int hdrlen, int bodylen);


void Curl_cleanup();


#endif
