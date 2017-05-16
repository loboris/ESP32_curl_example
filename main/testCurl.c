


#include "freertos/FreeRTOS.h"

#include "espcurl.h"
#include "quickmail.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "sdkconfig.h"

#undef DISABLE_SSH_AGENT

// =====================================
// === Set your WiFi SSID & password
#define SSID CONFIG_WIFI_SSID
#define PASSWORD CONFIG_WIFI_PASSWORD
// =====================================

// ========================================================================================================================
// === Mail server, set different if needed
static char mail_server[] = CONFIG_MAIL_SERVER;
static uint32_t mail_port = CONFIG_MAIL_SERVER_PORT;
// secure (SSL/TLS):  QUICKMAIL_PROT_SMTPS
//       non secure:  QUICKMAIL_PROT_SMTP
static uint8_t mail_protocol = QUICKMAIL_PROT_SMTPS;

// Mail server user
static char mail_user[] = CONFIG_MAIL_USER;
// Mail server user password
static char mail_pass[] = CONFIG_MAIL_PASSWORD;

// Recipients email addressaddress
static char mail_to[] = CONFIG_MAIL_RECIPIENT;
// email subject
static char mail_subj[] = "Mail from ESP32";
// email message
static char mail_msg[] = "Hi from my ESP32\r\nThis is mail message with some attachments\r\n";
// email attachment sent if attachment file cannot be opened
static char mail_attach[] = "This is a <b>test</b> e-mail.<br/>\nThis mail was sent using <u>libquickmail</u> from ESP32.";
// ========================================================================================================================

// ===================================================================================================
// Those are real URL's which can be used for testing
static char Get_testURL[] = "http://loboris.eu/ESP32/test.php?par1=765&par2=test";
static char Get_file_testURL[] = "http://loboris.eu/ESP32/tiger.bmp";
static char Get_bigfile_testURL[] = "http://loboris.eu/ESP32/bigimg.jpg";

static char Get_testURL_SSL[] = "https://www-eng-x.llnl.gov/documents/a_document.txt";
//static char Get_testURL_SSL[] = "https://google.com";

static char Post_testURL[] = "http://loboris.eu/ESP32/test.php";

static char Ftp_testURL[] = "ftp://loboris.eu";
static char Ftp_user_pass[] = "esp32:esp32user";
static char Ftp_gettext_testURL[] = "ftp://loboris.eu/ftptest.txt";
static char Ftp_getfile_testURL[] = "ftp://loboris.eu/tiger240.jpg";
static char Ftp_putfile_testURL[] = "ftp://loboris.eu/put_test.jpg";
static char Ftp_puttextfile_testURL[] = "ftp://loboris.eu/put_test.txt";

static char SFtp_pass[] = "password";
static char SFtp_pass2[] = "demo-user";
static char SFtp_getfile_testURL[] = "sftp://demo@test.rebex.net:22/readme.txt";
static char SFtp_putfile_testURL[] = "sftp://demo-user@demo.wftpserver.com:2222/upload/esp32test.jpg";
// ===================================================================================================


static uint8_t print_header = 0;
static uint8_t print_body = 0;

#define SIMULATE_FS "simulate"

static char tag[] = "[cURL Example]";
static uint8_t has_fs = 0;
static uint8_t thread_started = 0;
static int last_error = 0;
static int num_errors = 0;
static uint8_t _restarting = 0;

static esp_err_t nvs_ok;
static uint32_t restart_cnt;

// Handle of the wear leveling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/spiflash";



// Print some info about curl environment
//---------------------
static void curl_info()
{
	curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);

    printf("\r\n=========\r\n");
    printf("cURL INFO\r\n");
    printf("=========\r\n\n");

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

//----------------------------------
static int check_file(char *fname) {
	if (!has_fs) return -1;

	struct stat sb;
	if ((stat(fname, &sb) == 0) && (sb.st_size > 0)) return 1;
	return 0;
}

//---------------------------------------------------------
static void print_response(char *hd, char * bdy, int res) {
	if (res) {
		printf("     ERROR: %d [%s]\r\n", res, bdy);
	}
	else {
		if (print_header) {
			printf("\r\n____________ Response HEADER: ____________\r\n%s\r\n^^^^^^^^^^^^ Response HEADER: ^^^^^^^^^^^^\r\n", hd);
		}
		if (print_body) {
			printf("\r\n____________ Response BODY: ____________\r\n%s\r\n^^^^^^^^^^^^ Response BODY: ^^^^^^^^^^^^\r\n", bdy);
		}
		if ((!print_header) && (!print_body)) printf("     OK.");
	}
	last_error = res;
	if (res) num_errors++;
	else num_errors = 0;
}

//-------------------
static void testGET()
{
	int res = 0;
    char *hdrbuf = calloc(1024, 1);
    assert(hdrbuf);
    char *bodybuf = calloc(4096, 1);
    assert(bodybuf);

    printf("\r\n\r\n#### HTTP GET\r\n");
    printf("     Send GET request with parameters\r\n");

    res = Curl_GET(Get_testURL, NULL, hdrbuf, bodybuf, 1024, 4096);
    print_response(hdrbuf, bodybuf,res);
	if (res) goto exit;
    vTaskDelay(2000 / portTICK_RATE_MS);

	printf("\r\n\r\n#### HTTP GET FILE\r\n");
	printf("     Get file (~225 KB) from http server");

	int exists = check_file("/spiflash/tiger.bmp");
	if (exists == 0) {
		printf(" and save it to file system\r\n");
		res = Curl_GET(Get_file_testURL, "/spiflash/tiger.bmp", hdrbuf, bodybuf, 1024, 4096);
	}
	else {
		printf(", simulate saving to file system\r\n");
		res = Curl_GET(Get_file_testURL, SIMULATE_FS, hdrbuf, bodybuf, 1024, 4096);
	}

	print_response(hdrbuf, bodybuf,res);
	if (res) goto exit;
    vTaskDelay(1000 / portTICK_RATE_MS);

	printf("\r\n\r\n#### HTTP GET BIG FILE\r\n");
	printf("     Get big file (~2.4 MB), simulate saving to file system\r\n");

	uint16_t tmo = curl_timeout;
	uint32_t maxb = curl_maxbytes;

	curl_timeout = 90;
	curl_maxbytes = 3000000;

	res = Curl_GET(Get_bigfile_testURL, SIMULATE_FS, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);

	curl_timeout = tmo;
	curl_maxbytes = maxb;

exit:
    free(bodybuf);
    free(hdrbuf);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

//-----------------------
static void testGET_SSL()
{
    char *hdrbuf = calloc(1024, 1);
    assert(hdrbuf);
    char *bodybuf = calloc(4096, 1);
    assert(bodybuf);

    printf("\r\n\r\n#### HTTPS (SSL) GET\r\n");
    printf("     Send GET request to secure (SSL) server\r\n");

    int res = Curl_GET(Get_testURL_SSL, NULL, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);

	free(bodybuf);
    free(hdrbuf);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

//-------------------
static void testPOST()
{
    char *hdrbuf = calloc(1024, 1);
    assert(hdrbuf);
    char *bodybuf = calloc(4096, 1);
    assert(bodybuf);

    printf("\r\n\r\n#### HTTP POST\r\n");
    printf("     Send POST request with 3 parameters\r\n");

	if (formpost) curl_formfree(formpost);
	formpost=NULL;
	lastptr=NULL;

    // Add parameters
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "param1", CURLFORM_COPYCONTENTS, "1234", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "param2", CURLFORM_COPYCONTENTS, "esp32", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "param3", CURLFORM_COPYCONTENTS, "Test from ESP32", CURLFORM_END);
	// add file parameter
    int exists = check_file("/spiflash/postpar.txt");
	if (exists == 1) {
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, "/spiflash/postpar.txt", CURLFORM_END);
	}
    
    int res = Curl_POST(Post_testURL, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);

	free(bodybuf);
    free(hdrbuf);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

//-------------------
static void testFTP()
{
	int exists, res;

    char *hdrbuf = calloc(1024, 1);
    assert(hdrbuf);
    char *bodybuf = calloc(4096, 1);
    assert(bodybuf);

    printf("\r\n\r\n#### FTP LIST TEST\r\n");
    printf("     List files on FTP server\r\n");

    res = Curl_FTP(0, Ftp_testURL, Ftp_user_pass, NULL, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);
	if (res) goto exit;
    vTaskDelay(1000 / portTICK_RATE_MS);

    printf("\r\n\r\n#### FTP GET TEXT FILE\r\n");
    printf("     Get small text file from FTP server\r\n");

    res = Curl_FTP(0, Ftp_gettext_testURL, Ftp_user_pass, NULL, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);
	if (res) goto exit;
	// Save the text file to fs, we will use it as POST parameter
	exists = check_file("/spiflash/postpar.txt");
	if (exists == 0) {
		FILE * fd =fopen("/spiflash/postpar.txt", "wb");
		if (fd) {
			fwrite(bodybuf, 1, strlen(bodybuf), fd);
			fclose(fd);
			printf("     Received file saved to '/spiflash/postpar.txt' file\r\n");
		}
	}

    vTaskDelay(1000 / portTICK_RATE_MS);

	exists = check_file("/spiflash/tiger.jpg");
	printf("\r\n\r\n#### FTP GET JPG FILE\r\n");
    printf("     Get JPG file (~95 KB) from FTP server");
    if (exists == 0) {
    	printf(" and save it to file system\r\n");
    	res = Curl_FTP(0, Ftp_getfile_testURL, Ftp_user_pass, "/spiflash/tiger.jpg", hdrbuf, bodybuf, 1024, 4096);
    }
    else {
    	printf(", simulate saving to file system\r\n");
    	res = Curl_FTP(0, Ftp_getfile_testURL, Ftp_user_pass, SIMULATE_FS, hdrbuf, bodybuf, 1024, 4096);
    }
	print_response(hdrbuf, bodybuf,res);
	if (res) goto exit;

	exists = check_file("/spiflash/tiger.jpg");
    if (exists == 1) {
		printf("\r\n\r\n#### FTP PUT JPG FILE\r\n");
	    printf("     Upload received JPG file back to FTP server\r\n");

		res = Curl_FTP(1, Ftp_putfile_testURL, Ftp_user_pass, "/spiflash/tiger.jpg", hdrbuf, bodybuf, 1024, 4096);
    }
    else {
		printf("\r\n\r\n#### FTP PUT TEXT FILE\r\n");
	    printf("     Upload text file to FTP server, simulate reading from file system\r\n");

		res = Curl_FTP(1, Ftp_puttextfile_testURL, Ftp_user_pass, SIMULATE_FS, hdrbuf, bodybuf, 1024, 4096);
    }
	print_response(hdrbuf, bodybuf,res);

exit:
    free(bodybuf);
    free(hdrbuf);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

//--------------------
static void testSFTP()
{
	int exists, res;

    char *hdrbuf = calloc(1024, 1);
    assert(hdrbuf);
    char *bodybuf = calloc(4096, 1);
    assert(bodybuf);

    printf("\r\n\r\n#### SFTP DOWNLOAD TEXT FILE\r\n");
    printf("     Download small text file from SFTP (SSH) server\r\n");

    res = Curl_SFTP(0, SFtp_getfile_testURL, SFtp_pass, NULL, hdrbuf, bodybuf, 1024, 4096);
	print_response(hdrbuf, bodybuf,res);
	if (res == ESP_OK) {
		// Save the text file to fs, we will use it as POST parameter
		exists = check_file("/spiflash/sftptest.txt");
		if (exists == 0) {
			FILE * fd =fopen("/spiflash/sftptest.txt", "wb");
			if (fd) {
				fwrite(bodybuf, 1, strlen(bodybuf), fd);
				fclose(fd);
				printf("     Received file saved to '/spiflash/sftptest.txt' file\r\n");
			}
		}
	}
    vTaskDelay(2000 / portTICK_RATE_MS);

	exists = check_file("/spiflash/tiger.jpg");
    if (exists == 1) {
		printf("\r\n\r\n#### SFTP UPLOAD JPG FILE\r\n");
	    printf("     Upload JPG file to SFTP (SSH) server\r\n");

		res = Curl_SFTP(1, SFtp_putfile_testURL, SFtp_pass2, "/spiflash/tiger.jpg", hdrbuf, bodybuf, 1024, 4096);
    }
    else {
		printf("\r\n\r\n#### SFTP UPLOAD TEXT FILE\r\n");
	    printf("     Upload text file to SFTP (SSH) server, simulate reading from file system\r\n");

		res = Curl_SFTP(1, SFtp_putfile_testURL, SFtp_pass2, SIMULATE_FS, hdrbuf, bodybuf, 1024, 4096);
    }
	print_response(hdrbuf, bodybuf,res);

    free(bodybuf);
    free(hdrbuf);
    vTaskDelay(1000 / portTICK_RATE_MS);
}

//------------------------------------
static void testSMTP(uint8_t dosend) {
	quickmail_progress = curl_progress;

	printf("\r\n\r\n#### SMTP: SEND MAIL\r\n");
    printf("     Send email using '%s' mail server\r\n", mail_server);

    // Create quickmail object
	quickmail mailobj =  quickmail_create(mail_user, mail_subj);
	if (mailobj == NULL) {
		printf("     ERROR creating mail object\r\n");
		return;
	}

	const char* errmsg = NULL;

	// add recipient, more than one can be added
	quickmail_add_to(mailobj, mail_to);

	// add CC recipient, more than one can be added
	//quickmail_add_cc(mailobj, mail_cc);

	// set mail body
	quickmail_set_body(mailobj, mail_msg);

	// Add file attachment
	int exists = check_file("/spiflash/tiger.jpg");
	if (exists == 1) quickmail_add_attachment_file(mailobj, "/spiflash/tiger.jpg", NULL);
	else {
		printf("     Attachment file not found\r\n");
		// Add text attachment
		quickmail_add_body_memory(mailobj, "text/html", mail_attach, strlen(mail_attach), 0);
	}

	// set some options
	quickmail_add_header(mailobj, "Importance: Low");
	quickmail_add_header(mailobj, "X-Priority: 5");
	quickmail_add_header(mailobj, "X-MSMail-Priority: Low");

	if (curl_verbose) quickmail_set_debug_log(mailobj, stderr);
	else quickmail_set_debug_log(mailobj, NULL);

	// ** Send email **
	if (dosend == 0) errmsg = quickmail_protocol_send(mailobj, mail_server, mail_port, mail_protocol, mail_user, mail_pass);
	else printf("     Sending mail skipped\r\n");

	if (errmsg) {
		printf("     ERROR: %s\r\f", errmsg);
		num_errors++;
	}
	else {
		printf("     OK.\r\n");
	}
	// Cleanup
	quickmail_destroy(mailobj);
	quickmail_cleanup();
}


//--------------------------
static void delete_files() {
	int exists, ret;

	printf("Removing files on file system...\r\n");
	exists = check_file("/spiflash/postpar.txt");
	if (exists == 1) {
		ret = remove("/spiflash/postpar.txt");
		if (ret == 0) printf("* removed \"/spiflash/postpar.txt\"\r\n");
	}
	exists = check_file("/spiflash/tiger.bmp");
	if (exists == 1) {
		ret = remove("/spiflash/tiger.bmp");
		if (ret == 0) printf("* removed \"/spiflash/tiger.bmp\"\r\n");
	}
	exists = check_file("/spiflash/tiger.jpg");
	if (exists == 1) {
		ret = remove("/spiflash/tiger.jpg");
		if (ret == 0) printf("* removed \"/spiflash/tiger.jpg\"\r\n");
	}
	exists = check_file("/spiflash/sftptest.txt");
	if (exists == 1) {
		ret = remove("/spiflash/sftptest.txt");
		if (ret == 0) printf("* removed \"/spiflash/sftptest.txt\"\r\n");
	}

}

//----------------------------------
static void print_pass(uint32_t n) {
	char snum[16] = {'\0'};
	char srst[16] = {'\0'};

	sprintf(snum, "%u", n);
	sprintf(srst, "%u", restart_cnt);

	printf("\r\n\r\n------------------------------------------------------");
	printf("%.*s", strlen(snum), "----------------");
	printf("%.*s\r\n", strlen(srst), "----------------");
	ESP_LOGI(tag, "=== PASS %u; restart_cnt=%u ===", n, restart_cnt);
	printf("------------------------------------------------------");
	printf("%.*s", strlen(snum), "----------------");
	printf("%.*s\r\n", strlen(srst), "----------------");
}

//---------------------------
static void set_reset_cnt() {
	if (nvs_ok) return;

    // Open NVS
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_get_u32(my_handle, "restart_cnt", &restart_cnt);
        if (err != ESP_OK) restart_cnt = 0;

        restart_cnt++;
        err = nvs_set_i32(my_handle, "restart_cnt", restart_cnt);
        err = nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

//=============================
void testCurl(void *taskData) {

	if (has_fs) delete_files();

	curl_info();

	int nwait = 12;
    printf("\r\n");
    while (nwait > 0) {
    	printf("Starting in %d seconds...   \r", nwait);
    	fflush(stdout);
    	vTaskDelay(1000 / portTICK_RATE_MS);
    	nwait--;
    }

    // =========================================================================================
    // === Set the variables defining example behaviour ========================================
    curl_verbose = 1;	// If set to 1 verbose information about curl operations will be printed
    curl_progress = 5;	// Upload/download progress interval in seconds, set to 0 to disabče
    print_header = 0;	// Print response header if set to 1
    print_body = 1;		// Print response body if set to 1
    curl_timeout = 20;	// curl operations timeout
    // =========================================================================================

    num_errors = 0;

    uint32_t npass = 0;
    uint8_t sendmail = 1;
    while (1) {
    	sendmail = (npass % 10);
    	print_pass(++npass);

		testGET();
		if (num_errors > 5) break;

		testGET_SSL();
		if (num_errors > 5) break;

		testPOST();
		if (num_errors > 5) break;

		testFTP();
		if (num_errors > 5) break;

		testSFTP();
		if (num_errors > 5) break;

		testSMTP(sendmail);
		if (num_errors > 5) break;

	    int nwait = 60;
	    printf("\r\n");
	    while (nwait > 0) {
	    	printf("Waiting %d seconds...   \r", nwait);
	    	fflush(stdout);
	    	vTaskDelay(1000 / portTICK_RATE_MS);
	    	nwait--;
	    }
    }

	_restarting = 1;
	set_reset_cnt();

	printf("\r\n\n");
	ESP_LOGE(tag, "Too many errors, restarting...");
	ESP_LOGI(tag, "Unmounting FAT filesystem");
	esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle);

	vTaskDelay(5000 / portTICK_RATE_MS);
	esp_restart();

    vTaskDelete(NULL);
} // End of testCurl

//------------------------------------------------------------
esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	if (_restarting) return ESP_OK;

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_START");
		ESP_ERROR_CHECK(esp_wifi_connect());
		break;
    case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_GOT_IP");
		ESP_LOGI(tag, "got ip:%s ... ready to go!\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		if (thread_started == 0) {
			xTaskCreatePinnedToCore(&testCurl, "testCurl", 10*1024, NULL, 5, NULL, tskNO_AFFINITY);
			thread_started = 1;
		}
		break;
    case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_CONNECTED");
		break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGI(tag, "SYSTEM_EVENT_STA_DISCONNECTED");
		ESP_ERROR_CHECK(esp_wifi_connect());
		break;
    default:
		ESP_LOGI(tag, "=== WiFi EVENT: %d ===", event->event_id);
        break;
    }
    return ESP_OK;

}

//--------------------
static void mount_fs()
{
    ESP_LOGI(tag, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(tag, "=====================");
        ESP_LOGE(tag, "Failed to mount FATFS (0x%x)", err);
        ESP_LOGE(tag, "=====================\r\n");
        return;
    }

    ESP_LOGI(tag, "==============");
    ESP_LOGI(tag, "FATFS mounted.");
    ESP_LOGI(tag, "==============\r\n");
    has_fs = 1;
}


//================
int app_main(void)
{
    // Initialize NVS
    nvs_ok = nvs_flash_init();
    if (nvs_ok == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        const esp_partition_t* nvs_partition = esp_partition_find_first(
                ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
        assert(nvs_partition && "partition table must have an NVS partition");
        ESP_ERROR_CHECK( esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
        // Retry nvs_flash_init
        nvs_ok = nvs_flash_init();
    }
    if (nvs_ok == ESP_OK) {
		// Open NVS
		nvs_handle my_handle;
		esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
		if (err == ESP_OK) {
			err = nvs_get_u32(my_handle, "restart_cnt", &restart_cnt);
			if (err != ESP_OK) {
				restart_cnt = 0;
				err = nvs_set_i32(my_handle, "restart_cnt", restart_cnt);
				err = nvs_commit(my_handle);
				nvs_close(my_handle);
			}
		}
    }
    tcpip_adapter_init();

	mount_fs();
	vTaskDelay(1000 / portTICK_RATE_MS);

	ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
            .bssid_set = 0
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE) );

    return 0;
}

