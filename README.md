### Full example of using **libcurl** with ESP3232

---

Includes examples of using **http**/**https** GET & POST, **ftp** LIST, GET, PUT and sending **EMAIL**

The example uses **wear leveling FAT file system** and the latest **esp-idf** commit has to be used (5. May 2017 or later)

---

Included **libGSM** to test libCurl with GSM modems

For more info on using **GSM modules** see my [PPPoE Example repository](https://github.com/loboris/ESP32-PPPOS-EXAMPLE)

If using **GSM/PPPoS** with latest esp-idf (e6afe28bafe5db5ab79fae213f2e8e1ccd9f937c or later) a patch for **components/lwip/api/pppapi.c** is need.

Use included **pppapi.c.patch** to patch the file or copy **pppapi.c.patched** to **components/lwip/api/** as **pppapi.c**

---

#### How to build

Configure your esp32 build environment as for other **esp-idf examples**

Clone the repository

`git clone https://github.com/loboris/ESP32_curl_example.git`

Execute menuconfig and configure your Serial flash config and other settings. Included *sdkconfig.defaults* sets some defaults to be used. **Custom partitions table is used!**

Navigate to **Curl Example Configuration** and set your WiFi SSID and password, mail account user name and password, email recipient's mail address. If using **GSM module**, configure GSM parameters.

`make menuconfig`

Edit *testCurl()* function in **testCurl.c** if you want to change some operating parameters.

Make and flash the example.

`make all && make flash`

**All servers used in the example are real working servers.** You can change them if you want to test the responses from your own server(s).

---

#### Example functions

* print info about used **libcurl** version and supported features
* test **http** GET method; sends some parameters and gets response
* test http GET method; requests file and saves the file to file system
* test http GET method; requests large file (~2.4 MB), simulates saving the file to file system
* test **https** GET method; connects to https server using SSL, get the response
* test http POST method; sends 4 parameters including 1 file; gets the response
* test **ftp** LIST directory
* test ftp GET text file; saves the file to the filesystem
* test ftp GET binary file (JPG) to file system
* test ftp PUT binary file from file system (uploads previously downloaded file under different name)
* test **sftp** DOWNLOAD text file from **ssh** server; saves the file to the filesystem
* test sftp UPLOAD jpg file to **ssh** server
* test **SMTP**; sends email with attachment (attaches the jpg file downloaded from ftp server)

The sequence of operation is repeated after 60 second waiting.

If more than 5 consecutive errors are detected, the system is reseted.

While downloading, the files are stored on file system only if they don't already exists. If the file exists, writing to file is simulated. On restart all files on the file system are deleted.


---

**Example output:**

```

I (1760) cpu_start: Pro cpu start user code
I (1814) cpu_start: Starting scheduler on PRO CPU.
I (198) cpu_start: Starting scheduler on APP CPU.
I (212) [cURL Example]: Mounting FAT filesystem
I (215) [cURL Example]: ==============
I (215) [cURL Example]: FATFS mounted.
I (215) [cURL Example]: ==============

I (1219) wifi: wifi firmware version: 384bc4e
I (1219) wifi: config NVS flash: enabled
I (1219) wifi: config nano formating: disabled
I (1227) wifi: Init dynamic tx buffer num: 32
I (1227) wifi: Init dynamic rx buffer num: 32
I (1227) wifi: wifi driver task: 3ffc537c, prio:23, stack:3584
I (1231) wifi: Init static rx buffer num: 16
I (1235) wifi: Init dynamic rx buffer num: 32
I (1239) wifi: Init rx ampdu len mblock:7
I (1242) wifi: Init lldesc rx ampdu entry mblock:4
I (1247) wifi: wifi power manager task: 0x3ffccd24 prio: 21 stack: 2560
I (1253) wifi: wifi timer task: 3ffcddbc, prio:22, stack:3584
I (1275) phy: phy_version: 350, Mar 22 2017, 15:02:06, 1, 0
I (1276) wifi: mode : sta (24:0a:c4:00:97:c0)
I (1277) [cURL Example]: SYSTEM_EVENT_STA_START
I (1280) wifi: sleep disable
I (1400) wifi: n:1 0, o:1 0, ap:255 255, sta:1 0, prof:1
I (2058) wifi: state: init -> auth (b0)
I (2060) wifi: state: auth -> assoc (0)
I (2063) wifi: state: assoc -> run (10)
I (2098) wifi: connected with LoBoInternet, channel 1
I (2099) [cURL Example]: SYSTEM_EVENT_STA_CONNECTED
I (4212) event: ip: 192.168.0.16, mask: 255.255.255.0, gw: 192.168.0.1
I (4212) [cURL Example]: SYSTEM_EVENT_STA_GOT_IP
I (4214) [cURL Example]: got ip:192.168.0.16 ... ready to go!

Removing files on file system...
* removed "/spiflash/postpar.txt"
* removed "/spiflash/tiger.bmp"
* removed "/spiflash/tiger.jpg"

=========
cURL INFO
=========

Curl version info
  version: 7.54.1-DEV - 472577
Host: LUA-RTOS-ESP32
- IP V6 supported
- SSL supported
- LIBZ supported
- NTLM supported
- DEBUG NOT supported
- UNIX sockets NOT supported
Protocols:
- dict
- file
- ftp
- ftps
- gopher
- http
- https
- imap
- imaps
- pop3
- pop3s
- rtsp
- scp
- sftp
- smtp
- smtps
- telnet
- tftp

I (12064) wifi: pm start, type:0

Starting in 1 seconds...   

--------------------------------------------------------
I (16906) [cURL Example]: === PASS 1; restart_cnt=0 ===
--------------------------------------------------------


#### HTTP GET
     Send GET request with parameters

____________ Response BODY: ____________

============
Method: GET
============

--------------------------
Number of uploaded files: 0
    Number of GET params: 2
   Number of POST params: 0
--------------------------

===========
Debug info:
===========

-----------
POST DATA: 
-----------
Array
(
)
-----------
 GET DATA:  
-----------
Array
(
    [par1] => 765
    [par2] => test
)
-----------
FILE DATA: 
-----------
Array
(
)
-----------

=====================================
LoBo test server, 2017/05/07 15:42:19
=====================================

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### HTTP GET FILE
     Get file (~225 KB) from http server and save it to file system
* Download: received 150142
* Download: received 230456 B; time=8.0 s; speed=28.1 KB/sec

____________ Response BODY: ____________
Saved to file /spiflash/tiger.bmp, size=230456
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### HTTPS (SSL) GET
     Send GET request to secure (SSL) server

____________ Response BODY: ____________
This is a test

The quick brown fox jumped over the lazy dogs.
1234567890!@#$%^&*()[]{}-=_+;:'",.<>/?`~\|

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### HTTP POST
     Send POST request with 3 parameters

____________ Response BODY: ____________

============
Method: POST
============

--------------------------
Number of uploaded files: 0
    Number of GET params: 0
   Number of POST params: 3
--------------------------

===========
Debug info:
===========

-----------
POST DATA: 
-----------
Array
(
    [param1] => 1234
    [param2] => esp32
    [param3] => Test from ESP32
)
-----------
 GET DATA:  
-----------
Array
(
)
-----------
FILE DATA: 
-----------
Array
(
)
-----------

=====================================
LoBo test server, 2017/05/07 15:42:46
=====================================

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### FTP LIST TEST
     List files on FTP server

____________ Response BODY: ____________
-rw-r--r--   1 esp32    esp32group  2427146 May  2 17:29 bigimg.jpg
-rw-r--r--   1 esp32    esp32group  2427146 May  1 16:12 big.jpg
-rw-r--r--   1 root     root          609 May  5 17:50 ftptest.txt
-rw-r--r--   1 esp32    esp32group    97543 May  7 15:40 put_test.jpg
-rw-r--r--   1 esp32    esp32group     4092 May  7 12:43 put_test.txt
drwxr-xr-x   1 esp32    esp32group        0 Apr 26 00:35 test
-rw-r--r--   1 esp32    esp32group      583 May  1 14:54 test.txt
-rw-r--r--   1 esp32    esp32group    97543 Jan 30 19:26 tiger240.jpg
-rw-r--r--   1 esp32    esp32group   230456 Apr 30 10:29 tiger.bmp

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### FTP GET TEXT FILE
     Get small text file from FTP server

____________ Response BODY: ____________
=== Do you speak Latin ?
Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Nam rhoncus odio id venenatis volutpat.
Vestibulum dapibus bibendum ullamcorper.
Maecenas finibus elit augue, vel condimentum odio maximus nec.
In hac habitasse platea dictumst.
Vestibulum vel dolor et turpis rutrum finibus ac at nulla.
Vivamus nec neque ac elit blandit pretium vitae maximus ipsum.
Quisque sodales magna vel erat auctor, sed pellentesque nisi rhoncus.
Donec vehicula maximus pretium.
Aliquam eu tincidunt lorem.
Ut placerat, sem eu pharetra mattis, ante lacus fringilla diam, a consequat quam eros eget erat.

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^
     Received file saved to '/spiflash/postpar.txt' file


#### FTP GET JPG FILE
     Get JPG file (~95 KB) from FTP server and save it to file system
* Download: received 97543 B; time=4.5 s; speed=21.0 KB/sec

____________ Response BODY: ____________
Downloaded to file /spiflash/tiger.jpg, size=97543
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### FTP PUT JPG FILE
     Upload received JPG file back to FTP server
* Upload: sent 97543 B; time=2.8 s; speed=34.2 KB/sec

____________ Response BODY: ____________
Uploaded file /spiflash/tiger.jpg, size=97543
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### SFTP DOWNLOAD TEXT FILE
     Download small text file from SFTP (SSH) server

____________ Response BODY: ____________
Welcome,

you are connected to an FTP or SFTP server used for testing purposes by Rebex FTP/SSL or Rebex SFTP sample code.
Only read access is allowed and the FTP download speed is limited to 16KBps.

For infomation about Rebex FTP/SSL, Rebex SFTP and other Rebex .NET components, please visit our website at http://www.rebex.net/

For feedback and support, contact support@rebex.net

Thanks!

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^
     Received file saved to '/spiflash/sftptest.txt' file


#### SFTP UPLOAD JPG FILE
     Upload JPG file to SFTP (SSH) server
* Upload: sent 4096
* Upload: sent 86016
* Upload: sent 97543 B; time=11.9 s; speed=8.0 KB/sec

____________ Response BODY: ____________
Uploaded file /spiflash/tiger.jpg, size=97543
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### SMTP: SEND MAIL
     Send email using 'smtp.gmail.com' mail server

TIME: 10.0 UP: 129001
     OK.

Waiting 30 seconds...    

```

---

**Partial Example output with *verbose* mode ON:**

```
#### HTTPS (SSL) GET
     Send GET request to secure (SSL) server
* timeout on name lookup is not supported
*   Trying 198.128.229.135...
* TCP_NODELAY set
* Connected to www-eng-x.llnl.gov (198.128.229.135) port 443 (#0)
* Error reading ca cert file /certs/ca-certificates.crt - mbedTLS: (-0x3E00) PK - Read/write of file failed
* mbedTLS: Connecting to www-eng-x.llnl.gov:443
* mbedTLS: Set min SSL version to TLS 1.0
* mbedTLS: Handshake complete, cipher is TLS-ECDHE-RSA-WITH-AES-256-GCM-SHA384
* Dumping cert info:
* cert. version     : 3
* serial number     : 03:67:A7:12:0F:F5:61:60:7E:8D:13:EC:55:4F:49:69:1D:61
* issuer name       : C=US, O=Let's Encrypt, CN=Let's Encrypt Authority X3
* subject name      : CN=www-br.llnl.gov
* issued  on        : 2017-04-01 06:44:00
* expires on        : 2017-06-30 06:44:00
* signed using      : RSA with SHA-256
* RSA key size      : 2048 bits
* basic constraints : CA=false
* subject alt name  : www-br.llnl.gov, www-br.ucllnl.org, www-eng-x.llnl.gov
* key usage         : Digital Signature, Key Encipherment
* ext key usage     : TLS Web Server Authentication, TLS Web Client Authentication

* SSL connected
> GET /documents/a_document.txt HTTP/1.1
Host: www-eng-x.llnl.gov
Accept: */*
Accept-Encoding: deflate, gzip

< HTTP/1.1 200 OK
* Download: received 17
< Date: Sun, 07 May 2017 16:38:04 GMT
< Server: Apache
< Strict-Transport-Security: max-age=31536000; includeSubdomains; preload
< Last-Modified: Thu, 20 Apr 1995 07:00:00 GMT
< ETag: "6e-2d61af3ba7c00"
< Accept-Ranges: bytes
< Content-Length: 110
< Connection: close
< Content-Type: text/plain; charset=UTF-8
< 
* Download: received 110
* Closing connection 0

____________ Response BODY: ____________
This is a test

The quick brown fox jumped over the lazy dogs.
1234567890!@#$%^&*()[]{}-=_+;:'",.<>/?`~\|

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^

#### FTP GET JPG FILE
     Get JPG file (~95 KB) from FTP server and save it to file system
* timeout on name lookup is not supported
*   Trying 82.196.4.208...
* TCP_NODELAY set
* Connected to loboris.eu (82.196.4.208) port 21 (#0)
< 220 LoBo FTP Server
> AUTH SSL
< 500 AUTH not understood
> AUTH TLS
< 500 AUTH not understood
> USER esp32
< 331 Password required for esp32
> PASS esp32user
< 230 Anonymous access granted, restrictions apply
> PWD
< 257 "/" is the current directory
* Entry path is '/'
> PASV
* Connect data stream passively
* ftp_perform ends with SECONDARY: 0
< 227 Entering Passive Mode (82,196,4,208,192,21).
*   Trying 82.196.4.208...
* TCP_NODELAY set
* Connecting to 82.196.4.208 (82.196.4.208) port 49173
* Connected to loboris.eu (82.196.4.208) port 21 (#0)
> TYPE I
< 200 Type set to I
> SIZE tiger240.jpg
< 213 97543
> RETR tiger240.jpg
< 150 Opening BINARY mode data connection for tiger240.jpg (97543 bytes)
* Maxdownload = -1
* Getting file with size: 97543
* Remembering we are in dir ""
< 226 Transfer complete
> QUIT
< 221 Goodbye.
* Closing connection 0
* Download: received 97543 B; time=3.8 s; speed=24.8 KB/sec

____________ Response BODY: ____________
Downloaded to file /spiflash/tiger.jpg, size=97543
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^

#### SFTP DOWNLOAD TEXT FILE
     Download small text file from SFTP (SSH) server
* timeout on name lookup is not supported
*   Trying 195.144.107.198...
* TCP_NODELAY set
* Connected to test.rebex.net (195.144.107.198) port 22 (#0)
* SSH MD5 fingerprint: 0361c498f1ff7d239751071388b8c555
* SSH authentication methods available: password,keyboard-interactive,publickey
* Initialized password authentication
* Authentication complete
* Closing connection 0

____________ Response BODY: ____________
Welcome,

you are connected to an FTP or SFTP server used for testing purposes by Rebex FTP/SSL or Rebex SFTP sample code.
Only read access is allowed and the FTP download speed is limited to 16KBps.

For infomation about Rebex FTP/SSL, Rebex SFTP and other Rebex .NET components, please visit our website at http://www.rebex.net/

For feedback and support, contact support@rebex.net

Thanks!

^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^
     Received file saved to '/spiflash/sftptest.txt' file


#### SFTP UPLOAD JPG FILE
     Upload JPG file to SFTP (SSH) server
* timeout on name lookup is not supported
*   Trying 199.71.215.197...
* TCP_NODELAY set
* Connected to demo.wftpserver.com (199.71.215.197) port 2222 (#0)
* SSH MD5 fingerprint: 8c8cbe606d6a2a97008c203e15d36f74
* SSH authentication methods available: publickey,password
* Initialized password authentication
* Authentication complete
* Upload: sent 36864
* We are completely uploaded and fine
* Closing connection 0
* Upload: sent 97543 B; time=11.2 s; speed=8.5 KB/sec

____________ Response BODY: ____________
Uploaded file /spiflash/tiger.jpg, size=97543
^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^


#### SMTP: SEND MAIL
     Send email using 'smtp.gmail.com' mail server
* Rebuilt URL to: smtps://smtp.gmail.com:465/
* timeout on name lookup is not supported
*   Trying 74.125.133.109...
* TCP_NODELAY set
* Connected to smtp.gmail.com (74.125.133.109) port 465 (#0)
* Error reading ca cert file /certs/ca-certificates.crt - mbedTLS: (-0x3E00) PK - Read/write of file failed
* mbedTLS: Connecting to smtp.gmail.com:465
* mbedTLS: Set min SSL version to TLS 1.0
* mbedTLS: Handshake complete, cipher is TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256
* Dumping cert info:
* cert. version     : 3
* serial number     : 14:8E:07:6E:DC:42:D9:21
* issuer name       : C=US, O=Google Inc, CN=Google Internet Authority G2
* subject name      : C=US, ST=California, L=Mountain View, O=Google Inc, CN=smtp.gmail.com
* issued  on        : 2017-04-27 09:09:38
* expires on        : 2017-07-20 08:31:00
* signed using      : RSA with SHA-256
* RSA key size      : 2048 bits
* basic constraints : CA=false
* subject alt name  : smtp.gmail.com
* ext key usage     : TLS Web Server Authentication, TLS Web Client Authentication

* SSL connected
< 220 smtp.gmail.com ESMTP t85sm10281633wmt.23 - gsmtp
> EHLO localhost
< 250-smtp.gmail.com at your service, [109.60.11.156]
< 250-SIZE 35882577
< 250-8BITMIME
< 250-AUTH LOGIN PLAIN XOAUTH2 PLAIN-CLIENTTOKEN OAUTHBEARER XOAUTH
< 250-ENHANCEDSTATUSCODES
< 250-PIPELINING
< 250-CHUNKING
< 250 SMTPUTF8
> AUTH LOGIN
< 334 VXNlcm5hbWU6
> <!!hidden - user name!!>
< 334 UGFzc3dvcmQ6
> <!!hidden - password!!>
< 235 2.7.0 Accepted
> MAIL FROM:<loboris@gmail.com>
< 250 2.1.0 OK t85sm10281633wmt.23 - gsmtp
> RCPT TO:<loboris@gmail.com>
< 250 2.1.5 OK t85sm10281633wmt.23 - gsmtp
> DATA
< 354  Go ahead t85sm10281633wmt.23 - gsmtp

< 250 2.0.0 OK 1494175105 t85sm10281633wmt.23 - gsmtp
> QUIT
< 221 2.0.0 closing connection t85sm10281633wmt.23 - gsmtp
* Closing connection 0
TIME: 5.0 UP: 120055
     OK.

```
