/******************************************************************************
 * 
 * Copyright 2018 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define TAG "Interface"
#include "interface.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "eeprom.h"

#include "webclient.h"
#include "webserver.h"
#include "vs1053.h"
#include "gpios.h"
#include "ota.h"
#include "spiram_fifo.h"
#include "addon.h"
#include "main.h"
#include "xpt2046.h"
#include "ClickEncoder.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_system.h"
#include "mdns.h"

#include "esp_wifi.h"
#include "bt_x01.h"

const char parslashquote[] = {"(\""};
const char parquoteslash[] = {"\")"};
const char msgsys[] = {"##SYS."};
const char msgcli[] = {"##CLI."};

const char stritWIFISTATUS[] = {"#WIFI.STATUS#\nIP: %d.%d.%d.%d\nMask: %d.%d.%d.%d\nGateway: %d.%d.%d.%d\n##WIFI.STATUS#\n"};
const char stritWIFISTATION[] = {"#WIFI.STATION#\nSSID: %s\nPASSWORD: %s\n##WIFI.STATION#\n"};
const char stritCMDERROR[] = {"##CMD_ERROR#\n"};
const char wifiHELP[] = {"\
Commands:\n\
---------\n\
 Wifi related commands\n\
//////////////////\n\
wifi.lis or wifi.scan: give the list of received SSID\n\
wifi.con: Display the AP1 and AP2 SSID\n\
wifi.recon: Reconnect wifi if disconnected by wifi.discon\n\
wifi.con(\"ssid\",\"password\"): Record the given AP ssid with password in AP1 for next reboot\n\
wifi.discon: disconnect the current ssid\n\
wifi.station: the current ssid and password\n\
wifi.status: give the current IP GW and mask\n\
wifi.rssi: print the rssi (power of the reception\n\
wifi.auto[(\"x\")]  show the autoconnection  state or set it to x. x=0: reboot on wifi disconnect or 1: try reconnection.\n\n\
"};

const char clientHelp[] = {"\
//////////////////\n\
  Station Client commands\n\
//////////////////\n\
cli.url(\"url\"): the name or ip of the station on instant play\n\
cli.path(\"/path\"): the path of the station on instant play\n\
cli.port(\"xxxx\"): the port number of the station on instant play\n\
cli.instant: play the instant station\n\
cli.start: start to play the current station\n\
cli.play(\"x\"): play the x recorded station in the list\n\
cli.prev (or cli.previous): select the previous station in the list and play it\n\
"};
const char clientHelp1[] = {"\
cli.next: select the next station in the list and play it\
cli.stop: stop the playing station or instant\n\
cli.list: list all recorded stations\n\
cli.list(\"x\"): list only one of the recorded stations. Answer with #CLI.LISTINFO#: followed by infos\n\
cli.vol(\"x\"): set the volume to x with x from 0 to 254 (volume max)\n\
cli.vol: display the current volume. respond with ##CLI.VOL# xxx\n\
cli.vol-: Decrement the volume by 10 \n\
cli.vol+: Increment the volume by 10 \n\
Every vol command from uart or web or browser respond with ##CLI.VOL#: xxx\n\
cli.info: Respond with nameset, all icy, meta, volume and stae playing or stopped. Used to refresh the lcd informations \n\n\
"};

const char sysHELP[] = {"\
//////////////////\n\
  System commands\n\
//////////////////\n\
sys.uart(\"x\"): Change the baudrate of the uart on the next reset.\n\
 Valid x are: 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76880, 115200, 230400\n\
sys.i2s: Display the current I2S speed\n\
sys.i2s(\"x\"): Change and record the I2S clock speed of the vs1053 GPIO5 MCLK of the i2s interface to external dac.\n\
: 0=48kHz, 1=96kHz, 2=192kHz, other equal 0\n\
sys.erase: erase all recorded configuration and stations.\n\
sys.heap: show the ram heap size\n\
sys.update: start an OTA (On The Air) update of the software\n\
sys.boot: reboot.\n\
 "};
const char sysHELP1[] = {"\
sys.version: Display the Release and Revision numbers\n\
sys.date: Send a ntp request and Display the current locale time\n\
sys.dlog: Display the current log level\n\
sys.logx: Set log level to x with x=n for none, v for verbose, d for debug, i for info, w for warning, e for error\n\
sys.log: do nothing apart a trace on uart (debug use)\n\
sys.lcdout and sys.lcdout(\"x\"): Timer in seconds to switch off the lcd. 0= no timer\n\
sys.ddmm and sys.ddmm(\"x\"):  Display and Change  the date format. 0:MMDD, 1:DDMM\n\
sys.host and sys.host(\"your hostname\"): display and change the hostname for mDNS\n\
sys.rotat and sys.rotat(\"x\"): Change and display the lcd rotation option (reset needed). 0:no rotation, 1: rotation\n\
sys.henc: Display the current step setting for the encoder. Normal= 4 steps/notch, Half: 2 steps/notch\n\
sys.henc(\"y\") with y=0 Normal, y=1 Half\n\
sys.calibrate: start a touch screen calibration\n\n\
"};

const char otherHELP[] = {"\
///////////\n\
  Commands for BT201\n\
///////////\n\
bt.play: Play/Pause music into BT201.\n\
bt.next: Next melody into BT201.\n\
bt.prev: Previous melody into BT201.\n\
bt.volup: Volume + melody into BT201.\n\
bt.voldown: Volume - melody into BT201.\n\
bt.reset: Reset BT201\n\
bt.factory: Return to factory settings BT201\n\
bt.AT+COMMAND: Send command to BT201 \n\
eg, bt.AT+AA00 - Stopping play music it now \n\n\
///////////\n\
  Other\n\
///////////\n\
help: this command\n\
<enter> will display\n\
#INFO:\"\"\n\
\n\
A command error display:\n\
##CMD_ERROR#\n\r"};

static IRAM_ATTR uint32_t lcd_out = 0xFFFFFFFF;
static esp_log_level_t s_log_default_level = CONFIG_LOG_BOOTLOADER_LEVEL_INFO;
extern void wsVol(char *vol);
extern void playStation(char *id);
void clientVol(char *s);

#define MAX_WIFI_STATIONS 50
bool inside = false;
static uint8_t ddmm;
static uint8_t rotat;
uint8_t getDdmm()
{
	return ddmm;
}
void setDataFormat(uint8_t dm)
{
	if (dm == 0)
		ddmm = 0;
	else
		ddmm = 1;
}
uint8_t getRotat()
{
	return rotat;
}
void setRotat(uint8_t dm)
{
	if (dm == 0)
		rotat = 0;
	else
		rotat = 1;
}

void setVolumePlus()
{
	setRelVolume(10);
}
void setVolumeMinus()
{
	setRelVolume(-10);
}
void setVolumew(char *vol)
{
	setVolume(vol);
	wsVol(vol);
}

uint8_t getCurrentStation()
{
	return MainConfig->CurrentStation;
}
void setCurrentStation(uint8_t num)
{
	if (num != MainConfig->CurrentStation)
	{
		MainConfig->CurrentStation = num;
		SaveConfig();
	}
}

unsigned short adcdiv;

uint8_t startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void readRssi()
{
	int8_t rssi = -30;
	wifi_ap_record_t wifidata;
	esp_wifi_sta_get_ap_info(&wifidata);
	if (wifidata.primary != 0)
	{
		rssi = wifidata.rssi;
		// info.channel = wifidata.primary;
	}
	kprintf("##RSSI: %d\n", rssi);
}

void printInfo(char *s)
{
	kprintf("#INFO:\"%s\"\n", s);
}

const char htitle[] = {"\
=============================================================================\n\
             SSID                   |    RSSI    |           AUTH            \n\
=============================================================================\n\
"};
const char hscan1[] = {"#WIFI.SCAN#\n Number of access points found: %d\n"};

void wifiScan()
{
	// from https://github.com/VALERE91/ESP32_WifiScan
	uint16_t number;
	wifi_ap_record_t *records;
	wifi_scan_config_t config = {
		.ssid = NULL,
		.bssid = NULL,
		.channel = 0,
		.show_hidden = true};

	config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
	config.scan_time.passive = 500;
	esp_wifi_scan_start(&config, true);
	esp_wifi_scan_get_ap_num(&number);
	records = malloc(sizeof(wifi_ap_record_t) * number);
	if (records == NULL)
	{
		free(records);
		return;
	}
	esp_wifi_scan_get_ap_records(&number, records); // get the records
	kprintf(hscan1, number);
	if (number == 0)
	{
		free(records);
		return;
	}
	int i;
	kprintf(htitle);

	for (i = 0; i < number; i++)
	{
		char *authmode;
		switch (records[i].authmode)
		{
		case WIFI_AUTH_OPEN:
			authmode = (char *)"WIFI_AUTH_OPEN";
			break;
		case WIFI_AUTH_WEP:
			authmode = (char *)"WIFI_AUTH_WEP";
			break;
		case WIFI_AUTH_WPA_PSK:
			authmode = (char *)"WIFI_AUTH_WPA_PSK";
			break;
		case WIFI_AUTH_WPA2_PSK:
			authmode = (char *)"WIFI_AUTH_WPA2_PSK";
			break;
		case WIFI_AUTH_WPA_WPA2_PSK:
			authmode = (char *)"WIFI_AUTH_WPA_WPA2_PSK";
			break;
		default:
			authmode = (char *)"Unknown";
			break;
		}
		kprintf("%32.32s    |    % 4d    |    %22.22s\n", records[i].ssid, records[i].rssi, authmode);
	}
	kprintf("##WIFI.SCAN#: DONE\n");

	free(records);
}

void wifiConnect(char *cmd)
{
	int i;
	for (i = 0; i < 32; i++)
		MainConfig->ssid1[i] = 0;
	for (i = 0; i < 64; i++)
		MainConfig->pass1[i] = 0;
	char *t = strstr(cmd, parslashquote);
	if (t == 0)
	{
		kprintf(stritCMDERROR);
		return;
	}
	char *t_end = strstr(t, "\",\"");
	if (t_end == 0)
	{
		kprintf(stritCMDERROR);
		return;
	}

	strncpy(MainConfig->ssid1, (t + 2), (t_end - t - 2));

	t = t_end + 3;
	t_end = strstr(t, parquoteslash);
	if (t_end == 0)
	{
		kprintf(stritCMDERROR);
		return;
	}

	strncpy(MainConfig->pass1, t, (t_end - t));
	MainConfig->current_ap = 1;
	MainConfig->dhcpEn1 = 1;
	SaveConfig();
	//
	kprintf("#WIFI.CON#\n");
	kprintf("##AP1: %s with dhcp on next reset#\n", MainConfig->ssid1);
	kprintf("##WIFI.CON#\n");
}

void wifiConnectMem()
{
	kprintf("#WIFI.CON#\n");
	kprintf("##AP1: %s#\n", MainConfig->ssid1);
	kprintf("##AP2: %s#\n", MainConfig->ssid2);
	kprintf("##WIFI.CON#\n");
}

static bool autoConWifi = true; // control for wifiReConnect & wifiDisconnect
static uint8_t autoWifi = 0;	// auto reconnect wifi if disconnected
uint8_t getAutoWifi(void)
{
	return autoWifi;
}
void setAutoWifi()
{
	autoWifi = (MainConfig->options & Y_WIFIAUTO) ? 1 : 0;
}

void wifiAuto(char *cmd)
{
	char *t = strstr(cmd, parslashquote);
	if (t == 0)
	{
		kprintf("##Wifi Auto is %s#\n", autoWifi ? "On" : "Off");
		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t value = atoi(t + 2);

	if (value == 0)
		MainConfig->options &= N_WIFIAUTO;
	else
		MainConfig->options |= Y_WIFIAUTO;
	autoWifi = value;
	SaveConfig();

	wifiAuto((char *)"");
}

void wifiReConnect()
{
	if (autoConWifi == false)
		esp_wifi_connect();
	autoConWifi = true;
}

void wifiDisconnect()
{
	esp_err_t err;
	autoConWifi = false;
	err = esp_wifi_disconnect();
	if (err == ESP_OK)
		kprintf("##WIFI.NOT_CONNECTED#\n");
	else
		kprintf("##WIFI.DISCONNECT_FAILED %d#\n", err);
}

void wifiStatus()
{
	tcpip_adapter_ip_info_t ipi;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipi);
	kprintf(stritWIFISTATUS,
			(ipi.ip.addr & 0xff), ((ipi.ip.addr >> 8) & 0xff), ((ipi.ip.addr >> 16) & 0xff), ((ipi.ip.addr >> 24) & 0xff),
			(ipi.netmask.addr & 0xff), ((ipi.netmask.addr >> 8) & 0xff), ((ipi.netmask.addr >> 16) & 0xff), ((ipi.netmask.addr >> 24) & 0xff),
			(ipi.gw.addr & 0xff), ((ipi.gw.addr >> 8) & 0xff), ((ipi.gw.addr >> 16) & 0xff), ((ipi.gw.addr >> 24) & 0xff));
}

void wifiGetStation()
{
	wifi_config_t conf;
	esp_wifi_get_config(ESP_IF_WIFI_STA, &conf);
	kprintf(stritWIFISTATION, conf.sta.ssid, conf.sta.password);
}

void clientParseUrl(char *s)
{
	char *t_end = NULL;
	char *t = strstr(s, parslashquote);
	if (t)
		t_end = strstr(t, parquoteslash);
	if ((!t) || (!t_end))
	{
		kprintf(stritCMDERROR);
		return;
	}
	t_end -= 2;

	char *url = (char *)malloc((t_end - t + 1) * sizeof(char));
	if (url != NULL)
	{
		uint8_t tmp;
		for (tmp = 0; tmp < (t_end - t + 1); tmp++)
			url[tmp] = 0;
		strncpy(url, t + 2, (t_end - t));
		clientSetURL(url);
		char *title = malloc(88);
		if (title == NULL)
		{
			free(title);
			return;
		}
		sprintf(title, "{\"iurl\":\"%s\"}", url);
		websocketbroadcast(title, strlen(title));
		free(title);
		free(url);
	}
	else
	{
		free(url);
	}
}

void clientParsePath(char *s)
{
	char *t_end = NULL;
	char *t = strstr(s, parslashquote);
	if (t)
		t_end = strstr(t, parquoteslash);
	if ((!t) || (!t_end))
	{
		kprintf(stritCMDERROR);
		return;
	}
	t_end -= 2;

	char *path = (char *)malloc((t_end - t + 1) * sizeof(char));
	if (path != NULL)
	{
		uint8_t tmp;
		for (tmp = 0; tmp < (t_end - t + 1); tmp++)
			path[tmp] = 0;
		strncpy(path, t + 2, (t_end - t));
		//kprintf("cli.path: %s\n",path);
		clientSetPath(path);
		char *title = malloc(130);
		if (title == NULL)
		{
			free(title);
			return;
		}
		sprintf(title, "{\"ipath\":\"%s\"}", path);
		websocketbroadcast(title, strlen(title));
		free(title);
		free(path);
	}
	else
		free(path);
}

void clientParsePort(char *s)
{
	char *t_end = NULL;
	char *t = strstr(s, parslashquote);
	if (t)
		t_end = strstr(t, parquoteslash);
	if ((!t) || (!t_end))
	{
		kprintf(stritCMDERROR);
		return;
	}
	t_end -= 2;

	char *port = (char *)malloc((t_end - t + 1) * sizeof(char));
	if (port != NULL)
	{
		uint8_t tmp;
		for (tmp = 0; tmp < (t_end - t + 1); tmp++)
			port[tmp] = 0;
		strncpy(port, t + 2, (t_end - t));
		uint16_t porti = atoi(port);
		clientSetPort(porti);
		char *title = malloc(130);
		if (title == NULL)
		{
			free(title);
			return;
		}
		sprintf(title, "{\"iport\":\"%d\"}", porti);
		websocketbroadcast(title, strlen(title));
		free(title);
		free(port);
	}
	else
		free(port);
}

void clientPlay(char *s)
{
	char *t_end = NULL;
	char *t = strstr(s, parslashquote);
	if (t)
		t_end = strstr(t, parquoteslash);
	if ((!t) || (!t_end))
	{
		kprintf(stritCMDERROR);
		return;
	}
	t_end -= 2;

	char *id = (char *)malloc((t_end - t + 1) * sizeof(char));
	if (id != NULL)
	{
		uint8_t tmp;
		for (tmp = 0; tmp < (t_end - t + 1); tmp++)
			id[tmp] = 0;
		strncpy(id, t + 2, (t_end - t));
		playStationInt((atoi(id) - 1));
		free(id);
	}
	else
		free(id);
}

const char strilLIST[] = {"##CLI.LIST#\n"};
const char strilINFO[] = {"#CLI.LISTINFO#: %3d: %s, %s:%d%s\n"};
const char strilNUM[] = {"#CLI.LISTNUM#: %3d: %s, %s:%d%s\n"};
const char strilDLIST[] = {"\n#CLI.LIST#\n"};

void clientList(char *s)
{
	uint8_t i = 0, j = MainConfig->TotalStations;
	bool onlyOne = false;

	char *t = strstr(s, parslashquote);
	if (t != NULL) // a number specified
	{
		char *t_end = strstr(t, parquoteslash) - 2;
		if (t_end == (char *)0)
		{
			kprintf(stritCMDERROR);
			return;
		}
		i = atoi(t + 2);
		if (i > MainConfig->TotalStations)
			i = 0;
		j = i + 1;
		onlyOne = true;
	}

	if (!onlyOne)
		kprintf(strilDLIST);
	for (; i < j; i++)
	{
		vTaskDelay(1);
		station_slot_s *si = getStation(i);

		if ((si == NULL) || (si->port == 0))
		{
			if (si != NULL)
			{
				free(si);
			}
			continue;
		}

		if (si != NULL)
		{
			if (si->port != 0)
			{
				if (onlyOne)
					kprintf(strilINFO, (i + 1), si->name, si->domain, si->port, si->file);
				else
					kprintf(strilNUM, (i + 1), si->name, si->domain, si->port, si->file);
			}
			free(si);
		}
	}
	if (!onlyOne)
		kprintf(strilLIST);
}
// parse url
bool parseUrl(char *src, char *url, char *path, uint16_t *port)
{
	char *teu, *tbu, *tbpa;
	char *tmp;
	tmp = strstr(src, "://");
	if (tmp)
		tmp += 3;
	else
		tmp = src;
	tbu = tmp;
	//printf("tbu: %s\n",tbu);
	teu = strchr(tbu, ':');
	if (teu)
	{
		tmp = teu + 1;
		*port = atoi(tmp);
	}
	tbpa = strchr(tmp, '/');
	if (tbpa)
	{
		if (!teu)
			teu = tbpa - 1;
		strcpy(path, tbpa);
	}
	if (teu)
	{
		strncpy(url, tbu, teu - tbu);
		url[teu - tbu] = 0;
	}
	else
		strcpy(url, src);
	return true;
}

//edit a station
void clientEdit(char *s)
{
	station_slot_s *si;
	uint8_t id = 0xFF;
	char *tmp;
	char *tmpend;
	char url[200];

	si = malloc(sizeof(station_slot_s));
	if (si == NULL)
	{
		kprintf("##CLI.EDIT#: ERROR MEM#");
		return;
	}
	memset(si->domain, 0, sizeof(si->domain));
	memset(si->file, 0, sizeof(si->file));
	memset(si->name, 0, sizeof(si->name));
	memset(url, 0, 200);
	si->port = 80;
	//	printf("##CLI.EDIT: %s",s);
	ESP_LOGI(TAG, "%s", s);
	tmp = s + 10;
	tmpend = strchr(tmp, ':');
	if (tmpend - tmp)
		id = atoi(tmp);
	tmp = ++tmpend; //:
	tmpend = strchr(tmp, ',');
	if (tmpend - tmp)
	{
		strncpy(si->name, tmp, tmpend - tmp);
	}
	tmp = ++tmpend; //,
	tmpend = strchr(tmp, '%');
	if (tmpend == NULL)
		tmpend = strchr(tmp, '"');
	if (tmpend - tmp)
	{
		strncpy(url, tmp, tmpend - tmp);
	}
	else
		url[0] = 0;

	//printf("==> id: %d, name: %s, url: %s\n",id,si->name,url);

	// Parsing
	if (url[0] != 0)
		parseUrl(url, si->domain, si->file, &(si->port));

	//printf(" id: %d, name: %s, url: %s, port: %d, path: %s\n",id,si->name,si->domain,si->port,si->file);
	if (id < 0xFF)
	{
		if (si->domain[0] == 0)
		{
			si->port = 0;
			si->file[0] = 0;
		}
		saveStation(si, id);
		kprintf("##CLI.EDIT#: OK (%d)\n", id);
	}
	else
		kprintf("##CLI.EDIT#: ERROR\n");
}
void clientInfo()
{
	station_slot_s *si;
	kprintf("##CLI.INFO#\n");
	si = getStation(MainConfig->CurrentStation);
	if (si != NULL)
	{
		ntp_print_time();
		clientSetName(si->name, MainConfig->CurrentStation);
		clientPrintHeaders();
		clientVol((char *)"");
		clientPrintState();
		free(si);
	}
}

char *webInfo()
{
	station_slot_s *si;
	si = getStation(MainConfig->CurrentStation);
	char *resp = malloc(1024);
	if (si != NULL)
	{
		if (resp != NULL)
		{
			sprintf(resp, "vol: %d\nnum: %d\nstn: %s\ntit: %s\nsts: %d\n", getVolume(), MainConfig->CurrentStation, si->name, getMeta(), getState());
		}
		free(si);
	}
	return resp;
}

void sysI2S(char *s)
{
	char *t = strstr(s, parslashquote);
	if (t == NULL)
	{
		kprintf("##I2S speed of the vs1053: %d, 0=48kHz, 1=96kHz, 2=192kHz#\n", MainConfig->i2sspeed);
		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t speed = atoi(t + 2);
	VS1053_I2SRate(speed);

	MainConfig->i2sspeed = speed;
	SaveConfig();
	sysI2S((char *)"");
}

void sysUart(char *s)
{
	bool empty = false;
	char *t;
	t = NULL;
	if (s != NULL)
	{
		t = strstr(s, parslashquote);
		if (t == NULL)
		{
			empty = true;
		}
		else
		{
			char *t_end = strstr(t, parquoteslash);
			if (t_end == NULL)
			{
				empty = true;
			}
		}
	}
	if ((!empty) && (t != NULL))
	{
		uint32_t speed = atoi(t + 2);
		speed = checkUart(speed);
		MainConfig->uartspeed = speed;
		SaveConfig();
		kprintf("Speed: %u\n", speed);
	}
	kprintf("\n%sUART= %u# on next reset\n", msgsys, MainConfig->uartspeed);
}

void clientVol(char *s)
{
	char *t = strstr(s, parslashquote);
	if (t == 0)
	{
		// no argument, return the current volume
		kprintf("%sVOL#: %u\n", msgcli, getVolume());
		return;
	}
	char *t_end = strstr(t, parquoteslash) - 2;
	if (t_end == (char *)0)
	{

		kprintf(stritCMDERROR);
		return;
	}
	char *vol = (char *)malloc((t_end - t + 1) * sizeof(char));
	if (vol != NULL)
	{
		uint8_t tmp;
		for (tmp = 0; tmp < (t_end - t + 1); tmp++)
			vol[tmp] = 0;
		strncpy(vol, t + 2, (t_end - t));
		if ((atoi(vol) >= 0) && (atoi(vol) <= 254))
		{
			setVolumew(vol);
		}
		free(vol);
	}
	else
		free(vol);
}

// display or change the DDMM display mode
void sysddmm(char *s)
{
	char *t = strstr(s, parslashquote);

	if (t == NULL)
	{
		if (ddmm)
			kprintf("##Time is DDMM#\n");
		else
			kprintf("##Time is MMDD#\n");

		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t value = atoi(t + 2);
	if (value == 0)
		MainConfig->options &= N_DDMM;
	else
		MainConfig->options |= Y_DDMM;
	ddmm = (value) ? 1 : 0;
	SaveConfig();
	set_ddmm(ddmm);
	sysddmm((char *)"");
}

// get or set the encoder half resolution. Must be set depending of the hardware
void syshenc(char *s)
{
	char *t = strstr(s, parslashquote);
	encoder_s *encoder;
	bool encvalue;
	encoder = (encoder_s *)getEncoder();
	if (encoder == NULL)
	{
		kprintf("Encoder not defined#\n");
		return;
	}
	uint8_t options = MainConfig->options;
	encvalue = options & Y_ENC;

	if (t == NULL)
	{
		kprintf("##Step for encoder is ");
		if (encvalue)
			kprintf("half#\n");
		else
			kprintf("normal#\n");

		//		kprintf("Current value: %d\n",getHalfStep(encoder) );
		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t value = atoi(t + 2);
	if (value == 0)
	{
		MainConfig->options &= N_ENC;
	}
	else
	{
		MainConfig->options |= Y_ENC;
	}
	setHalfStep(encoder, value);
	syshenc((char *)"");
	SaveConfig();
}

// display or change the rotation lcd mode
void sysrotat(char *s)
{
	char *t = strstr(s, parslashquote);

	if (t == NULL)
	{
		kprintf("##Lcd rotation is ");
		if (rotat)
			kprintf("on#\n");
		else
			kprintf("off#\n");
		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t value = atoi(t + 2);
	if (value == 0)
		MainConfig->options &= N_ROTAT;
	else
		MainConfig->options |= Y_ROTAT;
	rotat = value;
	set_lcd_rotat(rotat);
	SaveConfig();
	sysrotat((char *)"");
}

// Timer in seconds to switch off the lcd
void syslcdout(char *s)
{
	char *t = strstr(s, parslashquote);
	lcd_out = MainConfig->lcd_out;
	if (t == NULL)
	{
		kprintf("##LCD out is ");
		kprintf("%u#\n", lcd_out);
		return;
	}
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}
	uint8_t value = atoi(t + 2);
	MainConfig->lcd_out = value;
	lcd_out = value;
	SaveConfig();
	set_lcd_out(lcd_out);
	syslcdout((char *)"");
	wakeLcd();
}

uint32_t getLcdOut()
{
	int increm = 0;
	get_lcd_out(&lcd_out);
	if (lcd_out == 0xFFFFFFFF)
	{
		lcd_out = MainConfig->lcd_out;
	}
	if (lcd_out > 0)
		increm++; //adjust
	return lcd_out + increm;
}

// print the heapsize
void heapSize()
{
	int hps = xPortGetFreeHeapSize();
	kprintf("%sHEAP: %d #\n", msgsys, hps);
}

// set hostname in mDNS
void setHostname(char *s)
{
	ESP_ERROR_CHECK(mdns_service_remove("_http", "_tcp"));
	ESP_ERROR_CHECK(mdns_service_remove("_telnet", "_tcp"));
	vTaskDelay(10);
	ESP_ERROR_CHECK(mdns_hostname_set(s));
	ESP_ERROR_CHECK(mdns_instance_name_set(s));
	vTaskDelay(10);
	ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
	ESP_ERROR_CHECK(mdns_service_add(NULL, "_telnet", "_tcp", 23, NULL, 0));
}

//display or change the hostname and services
void hostname(char *s)
{
	char *t = strstr(s, parslashquote);
	if (t == NULL)
	{
		kprintf("##SYS.HOST#: %s.local\n  IP:%s #\n", MainConfig->hostname, getIp());
		return;
	}

	t += 2;
	char *t_end = strstr(t, parquoteslash);
	if (t_end == NULL)
	{
		kprintf(stritCMDERROR);
		return;
	}

	if (t_end - t == 0)
		strcpy(MainConfig->hostname, "ESP32Radiola");
	else
	{
		if (t_end - t >= HOSTLEN)
			t_end = t + HOSTLEN;
		strncpy(MainConfig->hostname, t, (t_end - t) * sizeof(char));
		MainConfig->hostname[(t_end - t) * sizeof(char)] = 0;
	}
	SaveConfig();
	setHostname(MainConfig->hostname);
	hostname((char *)"");
}

void displayLogLevel()
{
	switch (s_log_default_level)
	{
	case ESP_LOG_NONE:
		kprintf("Log level is now ESP_LOG_NONE\n");
		break;
	case ESP_LOG_ERROR:
		kprintf("Log level is now ESP_LOG_ERROR\n");
		break;
	case ESP_LOG_WARN:
		kprintf("Log level is now ESP_LOG_WARN\n");
		break;
	case ESP_LOG_INFO:
		kprintf("Log level is now ESP_LOG_INFO\n");
		break;
	case ESP_LOG_DEBUG:
		kprintf("Log level is now ESP_LOG_DEBUG\n");
		break;
	case ESP_LOG_VERBOSE:
		kprintf("Log level is now ESP_LOG_VERBOSE\n");
		break;
	default:
		kprintf("Log level is now Unknonwn\n");
	}
}

esp_log_level_t getLogLevel()
{
	return s_log_default_level;
}

void setLogLevel(esp_log_level_t level)
{
	esp_log_level_set("*", level);
	s_log_default_level = level;
	if (MainConfig->trace_level != level)
	{
		MainConfig->trace_level = level;
		SaveConfig();
	}
	displayLogLevel();
}

// print  date time in ISO-8601 local time format
void ntp_print_time()
{
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);

	char msg[30];

	strftime(msg, 48, "%Y-%m-%dT%H:%M:%S", &timeinfo);
	//	ISO-8601 local time   https://www.w3.org/TR/NOTE-datetime
	//  YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
	kprintf("##SYS.DATE#: %s\n", msg);
}

void checkCommand(int size, char *s)
{
	if (size < 2)
		return;
	char *tmp = (char *)malloc((size + 1) * sizeof(char));
	int i;
	for (i = 0; i < size; i++)
		tmp[i] = s[i];
	tmp[size] = 0;
	//	kprintf("size: %d, cmd=%s\n",size,tmp);
	if (startsWith("bt.", tmp))
	{
		if (bt.present)
		{
			if (strcmp(tmp + 3, "reset") == 0)
			{
				kprintf("Reset BT201...\n");
				sendData("AT+CZ");
			}
			else if (strcmp(tmp + 3, "play") == 0)
			{
				kprintf("Play/Pause music into BT201...\n");
				sendData("AT+CB");
			}
			else if (strcmp(tmp + 3, "next") == 0)
			{
				kprintf("Next melody into BT201...\n");
				sendData("AT+CC");
			}
			else if (strcmp(tmp + 3, "prev") == 0)
			{
				kprintf("Previous melody into BT201...\n");
				sendData("AT+CD");
			}
			else if (strcmp(tmp + 3, "volup") == 0)
			{
				kprintf("Volume + into BT201...\n");
				sendData("AT+CE");
				vTaskDelay(10);
				sendData("AT+QA");
			}
			else if (strcmp(tmp + 3, "voldown") == 0)
			{
				kprintf("Volume - into BT201...\n");
				sendData("AT+CF");
				vTaskDelay(10);
				sendData("AT+QA");
			}
			else if (strcmp(tmp + 3, "factory") == 0)
			{
				kprintf("Return to factory settings BT201...\n");
				sendData("AT+CW");
			}
			else if (strcmp(tmp + 3, "debugon") == 0)
			{
				kprintf("Extended output of module BT201 events enabled...\n");
				bt.debug = true;
			}
			else if (strcmp(tmp + 3, "debugoff") == 0)
			{
				kprintf("Extended output of module BT201 events disabled...\n");
				bt.debug = false;
			}
			else if (strncmp(tmp + 3, "AT+", 3) == 0)
			{
				// разделитель
				char sep[] = ".";
				// Выделение первой части строки
				char *bt_cmd = strtok(tmp, sep);
				// Выделение второй части строки
				bt_cmd = strtok(NULL, sep);
				kprintf("Send command: \"%s\" to BT201...\n", bt_cmd);
				sendData(bt_cmd);
			}
			else
				printInfo(tmp);
		}
		else
			kprintf("BT201 %s!", MainConfig->bt_ver);
	}
	else if (startsWith("dbg.", tmp))
	{
		if (strcmp(tmp + 4, "fifo") == 0)
			kprintf("Buffer fill %u%%, %u bytes, OverRun: %ld, UnderRun: %ld\n",
					(spiRamFifoFill() * 100) / spiRamFifoLen(), spiRamFifoFill(), spiRamGetOverrunCt(), spiRamGetUnderrunCt());
		else if (strcmp(tmp + 4, "clear") == 0)
			spiRamFifoReset();

		else
			printInfo(tmp);
	}
	else if (startsWith("wifi.", tmp))
	{
		if (strcmp(tmp + 5, "list") == 0)
			wifiScan();
		else if (strcmp(tmp + 5, "scan") == 0)
			wifiScan();
		else if (strcmp(tmp + 5, "con") == 0)
			wifiConnectMem();
		else if (strcmp(tmp + 5, "recon") == 0)
			wifiReConnect();
		else if (startsWith("con", tmp + 5))
			wifiConnect(tmp);
		else if (strcmp(tmp + 5, "rssi") == 0)
			readRssi();
		else if (strcmp(tmp + 5, "discon") == 0)
			wifiDisconnect();
		else if (strcmp(tmp + 5, "status") == 0)
			wifiStatus();
		else if (strcmp(tmp + 5, "station") == 0)
			wifiGetStation();
		else if (startsWith("auto", tmp + 5))
			wifiAuto(tmp);
		else
			printInfo(tmp);
	}
	else if (startsWith("cli.", tmp))
	{
		if (startsWith("url", tmp + 4))
			clientParseUrl(tmp);
		else if (startsWith("path", tmp + 4))
			clientParsePath(tmp);
		else if (startsWith("port", tmp + 4))
			clientParsePort(tmp);
		else if (strcmp(tmp + 4, "instant") == 0)
		{
			clientDisconnect("cli instantplay");
			clientConnectOnce();
		}
		else if (strcmp(tmp + 4, "start") == 0)
			clientPlay((char *)"(\"255\")"); // outside value to play the current station
		else if (strcmp(tmp + 4, "stop") == 0)
			clientDisconnect("cli stop");
		else if (startsWith("list", tmp + 4))
			clientList(tmp);
		else if (strcmp(tmp + 4, "next") == 0)
			wsStationNext();
		else if (strncmp(tmp + 4, "previous", 4) == 0)
			wsStationPrev();
		else if (startsWith("play", tmp + 4))
			clientPlay(tmp);
		else if (strcmp(tmp + 4, "vol+") == 0)
			setVolumePlus();
		else if (strcmp(tmp + 4, "vol-") == 0)
			setVolumeMinus();
		else if (strcmp(tmp + 4, "info") == 0)
			clientInfo();
		else if (startsWith("vol", tmp + 4))
			clientVol(tmp);
		else if (startsWith("edit", tmp + 4))
			clientEdit(tmp);
		else
			printInfo(tmp);
	}
	else if (startsWith("sys.", tmp))
	{
		if (startsWith("i2s", tmp + 4))
			sysI2S(tmp);
		//		else if(strcmp(tmp+4, "adc") == 0) 		readAdc();
		else if (startsWith("uart", tmp + 4))
			sysUart(tmp);
		else if (strcmp(tmp + 4, "erase") == 0)
		{
			eeEraseAll();
		}
		else if (strcmp(tmp + 4, "heap") == 0)
			heapSize();
		else if (strcmp(tmp + 4, "boot") == 0)
		{
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}
		else if (strcmp(tmp + 4, "update") == 0)
			update_firmware((char *)"ESP32Radiola");
		else if (strcmp(tmp + 4, "date") == 0)
			ntp_print_time();
		else if (strncmp(tmp + 4, "vers", 4) == 0)
			kprintf("Release: %s, Revision: %s, ESP32Radiola\n", RELEASE, REVISION);
		else if (strcmp(tmp + 4, "logn") == 0)
			setLogLevel(ESP_LOG_NONE);
		else if (strcmp(tmp + 4, "loge") == 0)
			setLogLevel(ESP_LOG_ERROR);
		else if (strcmp(tmp + 4, "logw") == 0)
			setLogLevel(ESP_LOG_WARN);
		else if (strcmp(tmp + 4, "logi") == 0)
			setLogLevel(ESP_LOG_INFO);
		else if (strcmp(tmp + 4, "logd") == 0)
			setLogLevel(ESP_LOG_DEBUG);
		else if (strcmp(tmp + 4, "logv") == 0)
			setLogLevel(ESP_LOG_VERBOSE);
		else if (strcmp(tmp + 4, "dlog") == 0)
			displayLogLevel();
		else if (strncmp(tmp + 4, "cali", 4) == 0)
			xpt_calibrate();
		else if (startsWith("log", tmp + 4))
			; // do nothing
		else if (startsWith("lcdo", tmp + 4))
			syslcdout(tmp); // lcdout timer to switch off the lcd
		else if (startsWith("ddmm", tmp + 4))
			sysddmm(tmp);
		else if (startsWith("host", tmp + 4))
			hostname(tmp);
		else if (startsWith("rotat", tmp + 4))
			sysrotat(tmp);
		else if (startsWith("henc", tmp + 4))
			syshenc(tmp);
		else
			printInfo(tmp);
	}
	else if (strcmp(tmp, "help") == 0)
	{
		kprintfl(wifiHELP);
		vTaskDelay(1);
		kprintfl(clientHelp);
		vTaskDelay(1);
		kprintfl(clientHelp1);
		vTaskDelay(1);
		kprintfl(sysHELP);
		vTaskDelay(1);
		kprintfl(sysHELP1);
		vTaskDelay(1);
		kprintfl(otherHELP);
	}
	else
		printInfo(tmp);

	free(tmp);
}
