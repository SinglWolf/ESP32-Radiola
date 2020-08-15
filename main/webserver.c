/*
 * Copyright 2016 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <string.h>
#include "webserver.h"
#include "serv-fs.h"
#include "servers.h"
#include "driver/timer.h"
#include "driver/uart.h"
#include "main.h"
#include "ota.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "webclient.h"
#include "vs1053.h"
#include "eeprom.h"
#include "interface.h"
#include "addon.h"
#include "custom.h"
#include "gpios.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

#define TAG "webserver"

xSemaphoreHandle semfile = NULL;

const char strsROK[] = {"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: keep-alive\r\n\r\n%s"};
const char tryagain[] = {"try again"};

const char lowmemory[] = {"HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nlow memory\n"};
const char strsMALLOC[] = {"WebServer inmalloc fails for %d\n"};
const char strsMALLOC1[] = {"WebServer %s malloc fails\n"};
const char strsSOCKET[] = {"WebServer Socket fails %s errno: %d\n"};
const char strsID[] = {"getstation, no id or Wrong id %d\n"};
const char strsR13[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:13\r\n\r\n{\"%s\":\"%c\"}"};
const char strsICY[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"curst\":\"%s\",\
\"descr\":\"%s\",\
\"name\":\"%s\",\
\"bitr\":\"%s\",\
\"url1\":\"%s\",\
\"not1\":\"%s\",\
\"not2\":\"%s\",\
\"genre\":\"%s\",\
\"meta\":\"%s\",\
\"vol\":\"%s\",\
\"treb\":\"%s\",\
\"bass\":\"%s\",\
\"tfreq\":\"%s\",\
\"bfreq\":\"%s\",\
\"spac\":\"%s\",\
\"auto\":\"%c\"\
}"\
};
const char strsWIFI[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"ssid\":\"%s\",\
\"pasw\":\"%s\",\
\"ssid2\":\"%s\",\
\"pasw2\":\"%s\",\
\"ip\":\"%s\",\
\"msk\":\"%s\",\
\"gw\":\"%s\",\
\"ip2\":\"%s\",\
\"msk2\":\"%s\",\
\"gw2\":\"%s\",\
\"ua\":\"%s\",\
\"dhcp\":\"%s\",\
\"dhcp2\":\"%s\",\
\"mac\":\"%s\",\
\"host\":\"%s\"\
}"\
};
const char strsGSTAT[] = {"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n{\
\"Name\":\"%s\",\
\"URL\":\"%s\",\
\"File\":\"%s\",\
\"Port\":\"%d\"\
}"\
};
const char HARDWARE[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"present\":\"%u\",\
\"inputnum\":\"%u\",\
\"Volume\":\"%02u\",\
\"Treble\":\"%02u\",\
\"Bass\":\"%02u\",\
\"rear_on\":\"%u\",\
\"attlf\":\"%02u\",\
\"attrf\":\"%02u\",\
\"attlr\":\"%02u\",\
\"attrr\":\"%02u\",\
\"loud1\":\"%u\",\
\"loud2\":\"%u\",\
\"loud3\":\"%u\",\
\"sla1\":\"%u\",\
\"sla2\":\"%u\",\
\"sla3\":\"%u\",\
\"mute\":\"%u\"\
}"\
};
const char VERSION[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"RELEASE\":\"%s\",\
\"REVISION\":\"%s\",\
\"curtemp\":\"%05.1f\",\
\"rpmfan\":\"%04u\"\
}"\
};
const char GPIOS[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"GPIO_MODE\":\"%u\",\
\"ERROR\":\"%04X\",\
\"K_SPI\":\"%u\",\
\"P_MISO\":\"%03u\",\
\"P_MOSI\":\"%03u\",\
\"P_CLK\":\"%03u\",\
\"P_XCS\":\"%03u\",\
\"P_XDCS\":\"%03u\",\
\"P_DREQ\":\"%03u\",\
\"P_ENC0_A\":\"%03u\",\
\"P_ENC0_B\":\"%03u\",\
\"P_ENC0_BTN\":\"%03u\",\
\"P_I2C_SCL\":\"%03u\",\
\"P_I2C_SDA\":\"%03u\",\
\"P_LCD_CS\":\"%03u\",\
\"P_LCD_A0\":\"%03u\",\
\"P_IR_SIGNAL\":\"%03u\",\
\"P_BACKLIGHT\":\"%03u\",\
\"P_TACHOMETER\":\"%03u\",\
\"P_FAN_SPEED\":\"%03u\",\
\"P_DS18B20\":\"%03u\",\
\"P_TOUCH_CS\":\"%03u\",\
\"P_BUZZER\":\"%03u\",\
\"P_RXD\":\"%03u\",\
\"P_TXD\":\"%03u\",\
\"P_LDR\":\"%03u\"\
}"\
};
const char DEVOPTIONS[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"O_ESPLOG\":\"%u\",\
\"O_NTP\":\"%u\",\
\"O_NTP0\":\"%s\",\
\"O_NTP1\":\"%s\",\
\"O_NTP2\":\"%s\",\
\"O_NTP3\":\"%s\",\
\"O_TZONE\":\"%s\",\
\"O_TIME_FORMAT\":\"%u\",\
\"O_LCD_ROTA\":\"%u\"\
}"};
/* ,\
\"custom_NTP0\":\"%s\",\
\"custom_NTP1\":\"%s\",\
\"custom_NTP2\":\"%s\",\
\"custom_NTP3\":\"%s\",\
\"custom_TZ\":\"%s\",\
\"O_LCD_BRG\":\"%u\",\
\"begin_h\":\"%02u\",\
\"begin_m\":\"%02u\",\
\"end_h\":\"%02u\",\
\"end_m\":\"%02u\",\
\"day_brightness\":\"%03u\",\
\"night_brightness\":\"%03u\",\
\"brightness_trigger\":\"%04u\",\
\"hand_brightness\":\"%03u\",\
\"FanControl\":\"%u\",\
\"min_temp\":\"%02u\",\
\"max_temp\":\"%02u\",\
\"min_pwm\":\"%03u\",\
\"hand_pwm\":\"%03u\",\ */

const char IRCODES[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n{\
\"IR_MODE\":\"%u\",\
\"ERROR\":\"%04X\",\
\"K_0\":\"%s\",\
\"K_1\":\"%s\",\
\"K_2\":\"%s\",\
\"K_3\":\"%s\",\
\"K_4\":\"%s\",\
\"K_5\":\"%s\",\
\"K_6\":\"%s\",\
\"K_7\":\"%s\",\
\"K_8\":\"%s\",\
\"K_9\":\"%s\",\
\"K_10\":\"%s\",\
\"K_11\":\"%s\",\
\"K_12\":\"%s\",\
\"K_13\":\"%s\",\
\"K_14\":\"%s\",\
\"K_15\":\"%s\",\
\"K_16\":\"%s\"\
}"};

static uint32_t IR_Key[KEY_MAX][2];
esp_err_t get_ir_key(bool ir_mode)
{
	ir_key_t indexKey;
	nvs_handle handle;
	const char *klab[] = {
		"K_UP",
		"K_LEFT",
		"K_OK",
		"K_RIGHT",
		"K_DOWN",
		"K_0",
		"K_1",
		"K_2",
		"K_3",
		"K_4",
		"K_5",
		"K_6",
		"K_7",
		"K_8",
		"K_9",
		"K_STAR",
		"K_DIESE",
	};
	const uint32_t keydef[] = {
		0xFF0018, /* KEY_UP */
		0xFF0008, /* KEY_LEFT */
		0xFF001C, /* KEY_OK */
		0xFF005A, /* KEY_RIGHT */
		0xFF0052, /* KEY_DOWN */
		0xFF0019, /* KEY_0 */
		0xFF0045, /* KEY_1 */
		0xFF0046, /* KEY_2 */
		0xFF0047, /* KEY_3 */
		0xFF0044, /* KEY_4 */
		0xFF0040, /* KEY_5 */
		0xFF0043, /* KEY_6 */
		0xFF0007, /* KEY_7 */
		0xFF0015, /* KEY_8 */
		0xFF0009, /* KEY_9 */
		0xFF0016, /* KEY_STAR */
		0xFF000D, /* KEY_DIESE */
	};
	esp_err_t err = ESP_OK;
	memset(&IR_Key, 0, sizeof(uint32_t) * 2 * KEY_MAX); // clear custom
	if (ir_mode == IR_DEFAULD)
	{
		for (indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
		{
			// get the key default
			IR_Key[indexKey][0] = keydef[indexKey];
			ESP_LOGD(TAG, " IrDefaultKey is %s for set0: %X", klab[indexKey], IR_Key[indexKey][0]);
			taskYIELD();
		}
	}
	else
	{
		err = open_partition("hardware", "gpio_space", NVS_READONLY, &handle);
		if (err != ESP_OK)
		{
			close_partition(handle, "hardware");
			return err;
		}
		for (indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
		{
			// get the key in the nvs
			err |= gpio_get_ir_key(handle, klab[indexKey], (uint32_t *)&(IR_Key[indexKey][0]), (uint32_t *)&(IR_Key[indexKey][1]));
			taskYIELD();
		}
		close_partition(handle, "hardware");
	}
	return err;
}
//
//
uint8_t get_code(char *buf, uint32_t arg1, uint32_t arg2)
{
	if (arg2 > 0)
		sprintf(buf, "%X %X", arg1, arg2);
	else
		sprintf(buf, "%X", arg1);
	return strlen(buf);
}
//
void *inmalloc(size_t n)
{
	void *ret;
	//	ESP_LOGV(TAG, "server Malloc of %d %d,  Heap size: %d",n,((n / 32) + 1) * 32,xPortGetFreeHeapSize( ));
	ret = malloc(n);
	ESP_LOGV(TAG, "server Malloc of %x : %u bytes Heap size: %u", (int)ret, n, xPortGetFreeHeapSize());
	//	if (n <4) printf("Server: incmalloc size:%d\n",n);
	return ret;
}

void infree(void *p)
{
	ESP_LOGV(TAG, "server free of   %x,            Heap size: %u", (int)p, xPortGetFreeHeapSize());
	if (p != NULL)
		free(p);
}

static struct servFile *findFile(char *name)
{
	struct servFile *f = (struct servFile *)&indexFile;
	while (1)
	{
		if (strcmp(f->name, name) == 0)
			return f;
		else
			f = f->next;
		if (f == NULL)
			return NULL;
	}
}

static void respOk(int conn, const char *message)
{
	char rempty[] = {""};
	if (message == NULL)
		message = rempty;
	char fresp[strlen(strsROK) + strlen(message) + 15]; // = inmalloc(strlen(strsROK)+strlen(message)+15);
	sprintf(fresp, strsROK, "text/plain", (unsigned long)strlen(message), message);
	ESP_LOGV(TAG, "respOk %s", fresp);
	write(conn, fresp, strlen(fresp));
}

static void respKo(int conn)
{
	write(conn, lowmemory, strlen(lowmemory));
}

static void serveFile(char *name, int conn)
{
#define PART 1460
	int length;
	int gpart;

	char *content;
	struct servFile *f = findFile(name);
	ESP_LOGV(TAG, "find %s at %x", name, (int)f);
	ESP_LOGV(TAG, "Heap size: %u", xPortGetFreeHeapSize());
	gpart = PART;
	if (f != NULL)
	{
		length = f->size;
		content = (char *)f->content;
	}
	else
		length = 0;
	if (length > 0)
	{
		if (xSemaphoreTake(semfile, portMAX_DELAY))
		{
			char buf[150];
			ESP_LOGV(TAG, "serveFile socket:%d,  %s. Length: %d  sliced in %d", conn, name, length, gpart);
			sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Encoding: gzip\r\nContent-Length: %d\r\nConnection: keep-alive\r\n\r\n", (f != NULL ? f->type : "text/plain"), length);
			ESP_LOGV(TAG, "serveFile send %u bytes\n%s", strlen(buf), buf);
			vTaskDelay(1); // why i need it? Don't know.
			if (write(conn, buf, strlen(buf)) == -1)
			{
				respKo(conn);
				ESP_LOGE(TAG, "semfile fails 0 errno:%d", errno);
				xSemaphoreGive(semfile);
				return;
			}
			int progress = length;
			int part = gpart;
			if (progress <= part)
				part = progress;
			while (progress > 0)
			{
				if (write(conn, content, part) == -1)
				{
					respKo(conn);
					ESP_LOGE(TAG, "semfile fails 1 errno:%d", errno);
					xSemaphoreGive(semfile);
					return;
				}

				//				ESP_LOGV(TAG,"serveFile socket:%d,  read at %x len: %d",conn,(int)content,(int)part);
				content += part;
				progress -= part;
				if (progress <= part)
					part = progress;
				vTaskDelay(1);
			}
			xSemaphoreGive(semfile);
		}
		else
		{
			respKo(conn);
			ESP_LOGE(TAG, "semfile fails 2 errno:%d", errno);
			xSemaphoreGive(semfile);
			return;
		}
	}
	else
	{
		respKo(conn);
	}
	//	ESP_LOGV(TAG,"serveFile socket:%d, end",conn);
}

static bool getSParameter(char *result, uint32_t len, const char *sep, const char *param, char *data, uint16_t data_length)
{
	if ((data == NULL) || (param == NULL))
		return false;
	char *p = strstr(data, param);
	if (p != NULL)
	{
		p += strlen(param);
		char *p_end = strstr(p, sep);
		if (p_end == NULL)
			p_end = data_length + data;
		if (p_end != NULL)
		{
			if (p_end == p)
				return false;
			int i;
			if (len > (p_end - p))
				len = p_end - p;
			for (i = 0; i < len; i++)
				result[i] = 0;
			strncpy(result, p, len);
			result[len] = 0;
			ESP_LOGV(TAG, "getSParam: in: \"%s\"   \"%s\"", data, result);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

static char *getParameter(const char *sep, const char *param, char *data, uint16_t data_length)
{
	if ((data == NULL) || (param == NULL))
		return NULL;
	char *p = strstr(data, param);
	if (p != NULL)
	{
		p += strlen(param);
		char *p_end = strstr(p, sep);
		if (p_end == NULL)
			p_end = data_length + data;
		if (p_end != NULL)
		{
			if (p_end == p)
				return NULL;
			char *t = inmalloc(p_end - p + 1);
			if (t == NULL)
			{
				printf("getParameterF fails\n");
				return NULL;
			}
			ESP_LOGV(TAG, "getParameter malloc of %d  for %s", p_end - p + 1, param);
			int i;
			for (i = 0; i < (p_end - p + 1); i++)
				t[i] = 0;
			strncpy(t, p, p_end - p);
			ESP_LOGV(TAG, "getParam: in: \"%s\"   \"%s\"", data, t);
			return t;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

static char *getParameterFromResponse(const char *param, char *data, uint16_t data_length)
{
	return getParameter("&", param, data, data_length);
}
static bool getSParameterFromResponse(char *result, uint32_t size, const char *param, char *data, uint16_t data_length)
{
	return getSParameter(result, size, "&", param, data, data_length);
}
static char *getParameterFromComment(const char *param, char *data, uint16_t data_length)
{
	return getParameter("\"", param, data, data_length);
}

// set the volume with vol
void setVolumei(int8_t vol)
{
	if (vol < 0)
		vol = 0;
	VS1053_SetVolume(vol);
}

void setVolume(char *vol)
{
	int8_t uvol = atoi(vol);
	setIvol(uvol);
	if (vol != NULL)
	{
		VS1053_SetVolume(uvol);
		kprintf("##CLI.VOL#: %u\n", getIvol());
	}
}

uint16_t getVolume()
{
	return (getIvol());
}

// Set the volume with increment vol
void setRelVolume(int8_t vol)
{
	beep(10);
	char Vol[5];
	int16_t rvol;
	rvol = getIvol() + vol;
	if (rvol < 0)
		rvol = 0;
	if (rvol > 254)
		rvol = 254;
	sprintf(Vol, "%d", rvol);
	setVolume(Vol);
	wsVol(Vol);
}

// send the rssi
static void rssi(int socket)
{
	char answer[20];
	int8_t rssi = -30;
	wifi_ap_record_t wifidata;
	esp_wifi_sta_get_ap_info(&wifidata);
	if (wifidata.primary != 0)
	{
		rssi = wifidata.rssi;
	}
	sprintf(answer, "{\"wsrssi\":\"%d\"}", rssi);
	websocketwrite(socket, answer, strlen(answer));
}

// treat the received message of the websocket
void websockethandle(int socket, wsopcode_t opcode, uint8_t *payload, size_t length)
{
	//wsvol
	ESP_LOGV(TAG, "websocketHandle: %s", payload);
	if (strstr((char *)payload, "wsvol=") != NULL)
	{
		char answer[17];
		if (strstr((char *)payload, "&") != NULL)
			*strstr((char *)payload, "&") = 0;
		else
			return;
		//		setVolume(payload+6);
		sprintf(answer, "{\"wsvol\":\"%s\"}", payload + 6);
		websocketlimitedbroadcast(socket, answer, strlen(answer));
	}
	else if (strstr((char *)payload, "startSleep=") != NULL)
	{
		if (strstr((char *)payload, "&") != NULL)
			*strstr((char *)payload, "&") = 0;
		else
			return;
		startSleep(atoi((char *)payload + 11));
	}
	else if (strstr((char *)payload, "stopSleep") != NULL)
	{
		stopSleep();
	}
	else if (strstr((char *)payload, "startWake=") != NULL)
	{
		if (strstr((char *)payload, "&") != NULL)
			*strstr((char *)payload, "&") = 0;
		else
			return;
		startWake(atoi((char *)payload + 10));
	}
	else if (strstr((char *)payload, "stopWake") != NULL)
	{
		stopWake();
	}
	//monitor
	else if (strstr((char *)payload, "monitor") != NULL)
	{
		wsMonitor();
	}
	else if (strstr((char *)payload, "wsrssi") != NULL)
	{
		rssi(socket);
	}
}

void playStationInt(int sid)
{
	beep(20);
	struct shoutcast_info *si;
	char answer[24];
	si = getStation(sid);

	if (si != NULL && si->domain && si->file)
	{
		int i;
		vTaskDelay(4);
		clientSilentDisconnect();
		ESP_LOGV(TAG, "playstationInt: %d, new station: %s", sid, si->name);
		clientSetName(si->name, sid);
		clientSetURL(si->domain);
		clientSetPath(si->file);
		clientSetPort(si->port);

		//printf("Name: %s, url: %s, path: %s\n",	si->name,	si->domain, si->file);

		clientConnect();
		for (i = 0; i < 100; i++)
		{
			if (clientIsConnected())
				break;
			vTaskDelay(5);
		}
	}
	infree(si);
	sprintf(answer, "{\"wsstation\":\"%d\"}", sid);
	websocketbroadcast(answer, strlen(answer));
	ESP_LOGI(TAG, "playstationInt: %d, g_device: %u", sid, g_device->currentstation);
	if (g_device->currentstation != sid)
	{
		g_device->currentstation = sid;
		setCurrentStation(sid);
		saveDeviceSettings(g_device);
	}
}

void playStation(char *id)
{

	int uid;
	uid = atoi(id);
	ESP_LOGV(TAG, "playstation: %d", uid);
	if (uid < 255)
		setCurrentStation(atoi(id));
	playStationInt(getCurrentStation());
}

// replace special  json char
static void pathParse(char *str)
{
	int i;
	char *pend;
	char num[3] = {0, 0, 0};
	uint8_t cc;
	if (str == NULL)
		return;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == '%')
		{
			num[0] = str[i + 1];
			num[1] = str[i + 2];
			cc = strtol(num, &pend, 16);
			if (cc == '"') // for " in the string
			{
				str[i] = '\\';
				str[i + 1] = cc;
				str[i + 2] = 0;
			}
			else
			{
				str[i] = cc;
				str[i + 1] = 0;
			}
			if (str[i + 3] != 0)
				strcat(str, str + i + 3);
		}
	}
}

static void handlePOST(char *name, char *data, int data_size, int conn)
{
	ESP_LOGD(TAG, "HandlePost %s\n", name);
	int i;

	bool changed;
	if (strcmp(name, "/instant_play") == 0)
	{
		if (data_size > 0)
		{
			bool tst;
			char url[100];
			tst = getSParameterFromResponse(url, 100, "url=", data, data_size);
			char path[200];
			tst &= getSParameterFromResponse(path, 200, "path=", data, data_size);
			pathParse(path);
			char port[10];
			tst &= getSParameterFromResponse(port, 10, "port=", data, data_size);
			if (tst)
			{
				clientDisconnect("Post instant_play");
				for (i = 0; i < 100; i++)
				{
					if (!clientIsConnected())
						break;
					vTaskDelay(4);
				}
				clientSetURL(url);
				clientSetPath(path);
				clientSetPort(atoi(port));
				clientConnectOnce();
				for (i = 0; i < 100; i++)
				{
					if (clientIsConnected())
						break;
					vTaskDelay(5);
				}
			}
		}
	}
	else if (strcmp(name, "/soundvol") == 0)
	{
		if (data_size > 0)
		{
			char *vol = data + 4;
			data[data_size - 1] = 0;
			ESP_LOGD(TAG, "/sounvol vol: %s num:%d", vol, atoi(vol));
			setVolume(vol);
			respOk(conn, NULL);
			return;
		}
	}
	else if (strcmp(name, "/sound") == 0)
	{
		if (data_size > 0)
		{
			char bass[6];
			char treble[6];
			char bassfreq[6];
			char treblefreq[6];
			char spacial[6];
			changed = false;
			if (getSParameterFromResponse(bass, 6, "bass=", data, data_size))
			{
				if (g_device->bass != atoi(bass))
				{
					VS1053_SetBass(atoi(bass));
					changed = true;
					g_device->bass = atoi(bass);
				}
			}
			if (getSParameterFromResponse(treble, 6, "treble=", data, data_size))
			{
				if (g_device->treble != atoi(treble))
				{
					VS1053_SetTreble(atoi(treble));
					changed = true;
					g_device->treble = atoi(treble);
				}
			}
			if (getSParameterFromResponse(bassfreq, 6, "bassfreq=", data, data_size))
			{
				if (g_device->freqbass != atoi(bassfreq))
				{
					VS1053_SetBassFreq(atoi(bassfreq));
					changed = true;
					g_device->freqbass = atoi(bassfreq);
				}
			}
			if (getSParameterFromResponse(treblefreq, 6, "treblefreq=", data, data_size))
			{
				if (g_device->freqtreble != atoi(treblefreq))
				{
					VS1053_SetTrebleFreq(atoi(treblefreq));
					changed = true;
					g_device->freqtreble = atoi(treblefreq);
				}
			}
			if (getSParameterFromResponse(spacial, 6, "spacial=", data, data_size))
			{
				if (g_device->spacial != atoi(spacial))
				{
					VS1053_SetSpatial(atoi(spacial));
					changed = true;
					g_device->spacial = atoi(spacial);
				}
			}
			if (changed)
				saveDeviceSettings(g_device);
		}
	}
	else if (strcmp(name, "/getStation") == 0)
	{
		if (data_size > 0)
		{
			char id[6];
			if (getSParameterFromResponse(id, 6, "idgp=", data, data_size))
			{
				if ((atoi(id) >= 0) && (atoi(id) < 255))
				{
					char ibuf[10];
					char *buf;
					for (i = 0; i < sizeof(ibuf); i++)
						ibuf[i] = 0;
					struct shoutcast_info *si;
					si = getStation(atoi(id));
					if (strlen(si->domain) > sizeof(si->domain))
						si->domain[sizeof(si->domain) - 1] = 0; //truncate if any (rom crash)
					if (strlen(si->file) > sizeof(si->file))
						si->file[sizeof(si->file) - 1] = 0; //truncate if any (rom crash)
					if (strlen(si->name) > sizeof(si->name))
						si->name[sizeof(si->name) - 1] = 0; //truncate if any (rom crash)
					sprintf(ibuf, "%u", si->port);
					int json_length = strlen(si->domain) + strlen(si->file) + strlen(si->name) + strlen(ibuf) + 40;
					buf = inmalloc(json_length + 75);

					if (buf == NULL)
					{
						ESP_LOGE(TAG, " %s malloc fails", "getStation");
						respKo(conn);
						//return;
					}
					else
					{

						for (i = 0; i < sizeof(buf); i++)
							buf[i] = 0;
						sprintf(buf, strsGSTAT,
								json_length, si->name, si->domain, si->file, si->port);
						ESP_LOGV(TAG, "getStation Buf len:%u : %s", strlen(buf), buf);
						write(conn, buf, strlen(buf));
						infree(buf);
					}
					infree(si);
					return;
				}
				else
					printf(strsID, atoi(id));
				//				infree (id);
			}
		}
	}
	else if (strcmp(name, "/setStation") == 0)
	{
		if (data_size > 0)
		{
			//printf("data:%s\n",data);
			char nb[6];
			bool res;
			uint16_t unb, uid = 0;
			bool pState = getState(); // remember if we are playing
			res = getSParameterFromResponse(nb, 6, "nb=", data, data_size);
			if (res)
			{
				ESP_LOGV(TAG, "Setstation: nb init:%s", nb);
				unb = atoi(nb);
			}
			else
				unb = 1;

			if (!res)
			{
				ESP_LOGE(TAG, " %s nb null", "setStation");
				respKo(conn);
				return;
			}
			ESP_LOGV(TAG, "unb init:%u", unb);

			struct shoutcast_info *si = inmalloc(sizeof(struct shoutcast_info) * unb);

			if (si == NULL)
			{
				ESP_LOGE(TAG, " %s malloc fails", "setStation");
				respKo(conn);
				return;
			}
			char *bsi = (char *)si;
			int j;
			for (j = 0; j < sizeof(struct shoutcast_info) * unb; j++)
				bsi[j] = 0; //clean

			char id[6];
			char port[6];
			for (i = 0; i < unb; i++)
			{
				char *url;
				char *file;
				char *Name;
				struct shoutcast_info *nsi = si + i;
				url = getParameterFromResponse("url=", data, data_size);
				file = getParameterFromResponse("file=", data, data_size);
				pathParse(file);
				Name = getParameterFromResponse("name=", data, data_size);
				if (getSParameterFromResponse(id, 6, "id=", data, data_size))
				{
					//					ESP_LOGW(TAG,"nb:%d,si:%x,nsi:%x,id:%s,url:%s,file:%s",i,(int)si,(int)nsi,id,url,file);
					ESP_LOGV(TAG, "nb:%d, id:%s", i, id);
					if (i == 0)
						uid = atoi(id);
					if ((atoi(id) >= 0) && (atoi(id) < 255))
					{
						if (url && file && Name && getSParameterFromResponse(port, 6, "port=", data, data_size))
						{
							if (strlen(url) > sizeof(nsi->domain))
								url[sizeof(nsi->domain) - 1] = 0; //truncate if any
							strcpy(nsi->domain, url);
							if (strlen(file) > sizeof(nsi->file))
								url[sizeof(nsi->file) - 1] = 0; //truncate if any
							strcpy(nsi->file, file);
							if (strlen(Name) > sizeof(nsi->name))
								url[sizeof(nsi->name) - 1] = 0; //truncate if any
							strcpy(nsi->name, Name);
							nsi->port = atoi(port);
						}
					}
				}
				infree(Name);
				infree(file);
				infree(url);

				data = strstr(data, "&&") + 2;
				ESP_LOGV(TAG, "si:%x, nsi:%x, addr:%x", (int)si, (int)nsi, (int)data);
			}

			ESP_LOGV(TAG, "save station: %u, unb:%u, addr:%x", uid, unb, (int)si);
			saveMultiStation(si, uid, unb);
			ESP_LOGV(TAG, "save station return: %u, unb:%u, addr:%x", uid, unb, (int)si);
			infree(si);
			if (pState != getState())
				if (pState)
				{
					clientConnect();
					vTaskDelay(200);
				} //we was playing so start again the play
		}
	}
	else if (strcmp(name, "/play") == 0)
	{
		if (data_size > 4)
		{
			char *id = data + 3;
			data[data_size - 1] = 0;
			playStation(id);
		}
	}
	else if (strcmp(name, "/auto") == 0)
	{
		if (data_size > 4)
		{
			char *id = data + 3;
			data[data_size - 1] = 0;
			if ((strcmp(id, "true")) && (g_device->autostart == 1))
			{
				g_device->autostart = 0;
				ESP_LOGV(TAG, "autostart: %s, num:%u", id, g_device->autostart);
				saveDeviceSettings(g_device);
			}
			else if ((strcmp(id, "false")) && (g_device->autostart == 0))
			{
				g_device->autostart = 1;
				ESP_LOGV(TAG, "autostart: %s, num:%u", id, g_device->autostart);
				saveDeviceSettings(g_device);
			}
		}
	}
	else if (strcmp(name, "/rauto") == 0)
	{
		char buf[strlen(strsR13) + 16]; // = inmalloc( strlen(strsRAUTO)+16);
		sprintf(buf, strsR13, "rauto", (g_device->autostart) ? '1' : '0');
		write(conn, buf, strlen(buf));
		return;
	}
	else if (strcmp(name, "/stop") == 0)
	{
		if (clientIsConnected())
		{
			clientDisconnect("Post Stop");
			for (i = 0; i < 100; i++)
			{
				if (!clientIsConnected())
					break;
				vTaskDelay(4);
			}
		}
	}
	else if (strcmp(name, "/upgrade") == 0)
	{
		update_firmware("ESP32Radiola"); // start the OTA
	}
	else if (strcmp(name, "/version") == 0)
	{
		int json_length = 53 + strlen(RELEASE) + strlen(REVISION) + 5 + 4;
		char buf[446];
		sprintf(buf, VERSION,
				json_length,
				RELEASE,
				REVISION,
				getTemperature(),
				(uint16_t)getRpmFan() * 20);
		ESP_LOGD(TAG, "Test RELEASE and REVISION\nBuf len: %u\n\nBuf:\n%s", strlen(buf), buf);
		write(conn, buf, strlen(buf));
		return;
	}
	else if (strcmp(name, "/ircode") == 0) // start the IR TRAINING
	{
		if (data_size > 0)
		{
			char mode[1];
			if (getSParameterFromResponse(mode, 1, "mode=", data, data_size))
			{
				if (strcmp(mode, "1") == 0)
					set_ir_training(true);
				else
					set_ir_training(false); //возвращаем режим работы пульта
			}
		}
	}
	else if (strcmp(name, "/icy") == 0)
	{
		ESP_LOGV(TAG, "icy vol");
		char currentSt[6];
		sprintf(currentSt, "%u", getCurrentStation());
		char vol[6];
		sprintf(vol, "%u", (getVolume()));
		char treble[5];
		sprintf(treble, "%d", VS1053_GetTreble());
		char bass[5];
		sprintf(bass, "%u", VS1053_GetBass());
		char tfreq[5];
		sprintf(tfreq, "%d", VS1053_GetTrebleFreq());
		char bfreq[5];
		sprintf(bfreq, "%u", VS1053_GetBassFreq());
		char spac[5];
		sprintf(spac, "%u", VS1053_GetSpatial());

		struct icyHeader *header = clientGetHeader();
		ESP_LOGV(TAG, "icy start header %x", (int)header);
		char *not2;
		not2 = header->members.single.notice2;
		if (not2 == NULL)
			not2 = header->members.single.audioinfo;
		//if ((header->members.single.notice2 != NULL)&&(strlen(header->members.single.notice2)==0)) not2=header->members.single.audioinfo;
		int json_length;
		json_length = 166 + //
					  ((header->members.single.description == NULL) ? 0 : strlen(header->members.single.description)) +
					  ((header->members.single.name == NULL) ? 0 : strlen(header->members.single.name)) +
					  ((header->members.single.bitrate == NULL) ? 0 : strlen(header->members.single.bitrate)) +
					  ((header->members.single.url == NULL) ? 0 : strlen(header->members.single.url)) +
					  ((header->members.single.notice1 == NULL) ? 0 : strlen(header->members.single.notice1)) +
					  ((not2 == NULL) ? 0 : strlen(not2)) +
					  ((header->members.single.genre == NULL) ? 0 : strlen(header->members.single.genre)) +
					  ((header->members.single.metadata == NULL) ? 0 : strlen(header->members.single.metadata)) + strlen(currentSt) + strlen(vol) + strlen(treble) + strlen(bass) + strlen(tfreq) + strlen(bfreq) + strlen(spac);
		ESP_LOGD(TAG, "icy start header %x  len:%d vollen:%u vol:%s", (int)header, json_length, strlen(vol), vol);

		char *buf = inmalloc(json_length + 75);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post icy");
			infree(buf);
			respKo(conn);
			return;
		}
		else
		{
			uint8_t vauto = 0;
			vauto = (g_device->autostart) ? '1' : '0';
			sprintf(buf, strsICY,
					json_length,
					currentSt,
					(header->members.single.description == NULL) ? "" : header->members.single.description,
					(header->members.single.name == NULL) ? "" : header->members.single.name,
					(header->members.single.bitrate == NULL) ? "" : header->members.single.bitrate,
					(header->members.single.url == NULL) ? "" : header->members.single.url,
					(header->members.single.notice1 == NULL) ? "" : header->members.single.notice1,
					(not2 == NULL) ? "" : not2,
					(header->members.single.genre == NULL) ? "" : header->members.single.genre,
					(header->members.single.metadata == NULL) ? "" : header->members.single.metadata,
					vol, treble, bass, tfreq, bfreq, spac,
					vauto);
			ESP_LOGV(TAG, "test: len fmt:%u %u\n%s\n", strlen(strsICY), strlen(strsICY), buf);
			write(conn, buf, strlen(buf));
			infree(buf);
			wsMonitor();
			return;
		}
	}
	else if (strcmp(name, "/hardware") == 0)
	{
		changed = false;
		if (data_size > 0)
		{
			char save[1];
			if (getSParameterFromResponse(save, 1, "save=", data, data_size))
				if (strcmp(save, "1") == 0)
					changed = true;
			if (changed && tda7313_get_present())
			{
				char arg[2];
				getSParameterFromResponse(arg, 2, "inputnum=", data, data_size);
				if (tda7313_get_input() != atoi(arg))
				{
					ESP_ERROR_CHECK(tda7313_set_input(atoi(arg)));
					g_device->audio_input_num = atoi(arg);
				}
				getSParameterFromResponse(arg, 2, "Volume=", data, data_size);
				if (tda7313_get_volume() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_volume(atoi(arg)));
				getSParameterFromResponse(arg, 2, "Treble=", data, data_size);
				if (tda7313_get_treble() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_treble(atoi(arg)));
				getSParameterFromResponse(arg, 2, "Bass=", data, data_size);
				if (tda7313_get_bass() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_bass(atoi(arg)));
				getSParameterFromResponse(arg, 2, "rear_on=", data, data_size);
				if (tda7313_get_rear() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_rear(atoi(arg)));
				getSParameterFromResponse(arg, 2, "attlf=", data, data_size);
				if (tda7313_get_attlf() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_attlf(atoi(arg)));
				getSParameterFromResponse(arg, 2, "attrf=", data, data_size);
				if (tda7313_get_attrf() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_attrf(atoi(arg)));
				getSParameterFromResponse(arg, 2, "attlr=", data, data_size);
				if (tda7313_get_attlr() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_attlr(atoi(arg)));
				getSParameterFromResponse(arg, 2, "attrr=", data, data_size);
				if (tda7313_get_attrr() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_attrr(atoi(arg)));
				getSParameterFromResponse(arg, 2, "loud=", data, data_size);
				if (tda7313_get_loud(g_device->audio_input_num) != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_loud(atoi(arg)));
				getSParameterFromResponse(arg, 2, "sla=", data, data_size);
				if (tda7313_get_sla(g_device->audio_input_num) != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_sla(atoi(arg)));
				getSParameterFromResponse(arg, 2, "mute=", data, data_size);
				if (tda7313_get_mute() != atoi(arg))
					ESP_ERROR_CHECK(tda7313_set_mute(atoi(arg)));
				saveDeviceSettings(g_device);
			}
			int json_length;
			json_length = 216;

			char buf[302];
			sprintf(buf, HARDWARE,
					json_length,
					tda7313_get_present(),
					(uint8_t)g_device->audio_input_num,
					tda7313_get_volume(),
					tda7313_get_treble(),
					tda7313_get_bass(),
					tda7313_get_rear(),
					tda7313_get_attlf(),
					tda7313_get_attrf(),
					tda7313_get_attlr(),
					tda7313_get_attrr(),
					tda7313_get_loud(1),
					tda7313_get_loud(2),
					tda7313_get_loud(3),
					tda7313_get_sla(1),
					tda7313_get_sla(2),
					tda7313_get_sla(3),
					tda7313_get_mute());
			ESP_LOGD(TAG, "Test hardware\nSave: %s\nBuf len:%u\n%s", save, strlen(buf), buf);
			write(conn, buf, strlen(buf));
			return;
		}
	}
	else if (strcmp(name, "/wifi") == 0)
	{
		changed = false;
		if (data_size > 0)
		{
			bool val = false;
			char valid[5];
			if (getSParameterFromResponse(valid, 5, "valid=", data, data_size))
				if (strcmp(valid, "1") == 0)
					val = true;
			char *aua = getParameterFromResponse("ua=", data, data_size);
			pathParse(aua);
			char *host = getParameterFromResponse("host=", data, data_size);
			pathParse(host);

			//			ESP_LOGV(TAG,"wifi received  valid:%s,val:%d, ssid:%s, pasw:%s, aip:%s, amsk:%s, agw:%s, adhcp:%s, aua:%s",valid,val,ssid,pasw,aip,amsk,agw,adhcp,aua);
			if (val)
			{
				char adhcp[4], adhcp2[4];
				char *ssid = getParameterFromResponse("ssid=", data, data_size);
				pathParse(ssid);
				char *pasw = getParameterFromResponse("pasw=", data, data_size);
				pathParse(pasw);
				char *ssid2 = getParameterFromResponse("ssid2=", data, data_size);
				pathParse(ssid2);
				char *pasw2 = getParameterFromResponse("pasw2=", data, data_size);
				pathParse(pasw2);
				char *aip = getParameterFromResponse("ip=", data, data_size);
				char *amsk = getParameterFromResponse("msk=", data, data_size);
				char *agw = getParameterFromResponse("gw=", data, data_size);
				char *aip2 = getParameterFromResponse("ip2=", data, data_size);
				char *amsk2 = getParameterFromResponse("msk2=", data, data_size);
				char *agw2 = getParameterFromResponse("gw2=", data, data_size);

				changed = true;
				ip_addr_t valu;
				if (aip != NULL)
				{
					ipaddr_aton(aip, &valu);
					memcpy(g_device->ipAddr1, &valu, sizeof(uint32_t));
					ipaddr_aton(amsk, &valu);
					memcpy(g_device->mask1, &valu, sizeof(uint32_t));
					ipaddr_aton(agw, &valu);
					memcpy(g_device->gate1, &valu, sizeof(uint32_t));
				}
				if (aip2 != NULL)
				{
					ipaddr_aton(aip2, &valu);
					memcpy(g_device->ipAddr2, &valu, sizeof(uint32_t));
					ipaddr_aton(amsk2, &valu);
					memcpy(g_device->mask2, &valu, sizeof(uint32_t));
					ipaddr_aton(agw2, &valu);
					memcpy(g_device->gate2, &valu, sizeof(uint32_t));
				}
				if (getSParameterFromResponse(adhcp, 4, "dhcp=", data, data_size))
					if (strlen(adhcp) != 0)
					{
						if (strcmp(adhcp, "true") == 0)
							g_device->dhcpEn1 = 1;
						else
							g_device->dhcpEn1 = 0;
					}
				if (getSParameterFromResponse(adhcp2, 4, "dhcp2=", data, data_size))
					if (strlen(adhcp2) != 0)
					{
						if (strcmp(adhcp2, "true") == 0)
							g_device->dhcpEn2 = 1;
						else
							g_device->dhcpEn2 = 0;
					}

				strcpy(g_device->ssid1, (ssid == NULL) ? "" : ssid);
				strcpy(g_device->pass1, (pasw == NULL) ? "" : pasw);
				strcpy(g_device->ssid2, (ssid2 == NULL) ? "" : ssid2);
				strcpy(g_device->pass2, (pasw2 == NULL) ? "" : pasw2);

				infree(ssid);
				infree(pasw);
				infree(ssid2);
				infree(pasw2);
				infree(aip);
				infree(amsk);
				infree(agw);
				infree(aip2);
				infree(amsk2);
				infree(agw2);
			}

			if ((g_device->ua != NULL) && (strlen(g_device->ua) == 0))
			{
				if (aua == NULL)
				{
					aua = inmalloc(12);
					strcpy(aua, "ESP32Radiola/1.5");
				}
			}
			if (aua != NULL)
			{
				if ((strcmp(g_device->ua, aua) != 0) && (strcmp(aua, "undefined") != 0))
				{
					strcpy(g_device->ua, aua);
					changed = true;
				}
				infree(aua);
			}

			if (host != NULL)
			{
				if (strlen(host) > 0)
				{
					if ((strcmp(g_device->hostname, host) != 0) && (strcmp(host, "undefined") != 0))
					{
						strncpy(g_device->hostname, host, HOSTLEN - 1);
						setHostname(g_device->hostname);
						changed = true;
					}
				}
				infree(host);
			}

			if (changed)
			{
				saveDeviceSettings(g_device);
			}
			uint8_t macaddr[10]; // = inmalloc(10*sizeof(uint8_t));
			char macstr[20];	 // = inmalloc(20*sizeof(char));
			char adhcp[4], adhcp2[4];
			char tmpip[16], tmpmsk[16], tmpgw[16];
			char tmpip2[16], tmpmsk2[16], tmpgw2[16];
			esp_wifi_get_mac(WIFI_IF_STA, macaddr);
			int json_length;
			json_length = 95 + 39 + 10 +
						  strlen(g_device->ssid1) +
						  strlen(g_device->pass1) +
						  strlen(g_device->ssid2) +
						  strlen(g_device->pass2) +
						  strlen(g_device->ua) +
						  strlen(g_device->hostname) +
						  sprintf(tmpip, "%u.%u.%u.%u", g_device->ipAddr1[0], g_device->ipAddr1[1], g_device->ipAddr1[2], g_device->ipAddr1[3]) +
						  sprintf(tmpmsk, "%u.%u.%u.%u", g_device->mask1[0], g_device->mask1[1], g_device->mask1[2], g_device->mask1[3]) +
						  sprintf(tmpgw, "%u.%u.%u.%u", g_device->gate1[0], g_device->gate1[1], g_device->gate1[2], g_device->gate1[3]) +
						  sprintf(adhcp, "%u", g_device->dhcpEn1) +
						  sprintf(tmpip2, "%u.%u.%u.%u", g_device->ipAddr2[0], g_device->ipAddr2[1], g_device->ipAddr2[2], g_device->ipAddr2[3]) +
						  sprintf(tmpmsk2, "%u.%u.%u.%u", g_device->mask2[0], g_device->mask2[1], g_device->mask2[2], g_device->mask2[3]) +
						  sprintf(tmpgw2, "%u.%u.%u.%u", g_device->gate2[0], g_device->gate2[1], g_device->gate2[2], g_device->gate2[3]) +
						  sprintf(adhcp2, "%u", g_device->dhcpEn2) +
						  sprintf(macstr, MACSTR, MAC2STR(macaddr));

			char *buf = inmalloc(json_length + 95 + 39 + 10);
			if (buf == NULL)
			{
				ESP_LOGE(TAG, " %s malloc fails", "post wifi");
				respKo(conn);
				//return;
			}
			else
			{
				sprintf(buf, strsWIFI,
						json_length,
						g_device->ssid1, g_device->pass1, g_device->ssid2, g_device->pass2, tmpip, tmpmsk, tmpgw, tmpip2, tmpmsk2, tmpgw2, g_device->ua, adhcp, adhcp2, macstr, g_device->hostname);
				ESP_LOGD(TAG, "TEST WIFI:\nContent-Length: %d\nBuf len:%u\n%s", json_length, strlen(buf), buf);
				write(conn, buf, strlen(buf));
				infree(buf);
			}

			if (val)
			{
				// set current_ap to the first filled ssid
				ESP_LOGD(TAG, "currentAP: %u", g_device->current_ap);
				if (g_device->current_ap == APMODE)
				{
					if (strlen(g_device->ssid1) != 0)
						g_device->current_ap = STA1;
					else if (strlen(g_device->ssid2) != 0)
						g_device->current_ap = STA2;
					saveDeviceSettings(g_device);
				}
				ESP_LOGD(TAG, "currentAP: %u", g_device->current_ap);
				copyDeviceSettings(); // save the current one
				fflush(stdout);
				vTaskDelay(100);
				esp_restart();
			}
			return;
		}
	}
	else if (strcmp(name, "/devoptions") == 0)
	{
		changed = false;
		if (data_size > 0)
		{
			esp_log_level_t log_level = g_device->trace_level;
			uint8_t O_NTP = g_device->ntp_mode;
			uint8_t O_TIME_FORMAT;
			uint8_t O_LCD_ROTA;
			//uint8_t begin_h, begin_m, end_h, end_m;
			option_get_lcd_rotat(&O_LCD_ROTA);
			option_get_ddmm(&O_TIME_FORMAT);
			char *O_NTP0 = _NTP0, *O_NTP1 = _NTP1, *O_NTP2 = _NTP2, *O_NTP3 = _NTP3;
			if (O_NTP == 1)
			{
				O_NTP0 = g_device->ntp_server[0];
				O_NTP1 = g_device->ntp_server[1];
				O_NTP2 = g_device->ntp_server[2];
				O_NTP3 = g_device->ntp_server[3];
			}
			char *O_TZONE = g_device->tzone;
			bool reboot = false;
			char val_1[1];
			if (getSParameterFromResponse(val_1, 1, "save=", data, data_size))
				if (strcmp(val_1, "1") == 0)
					changed = true;
			if (changed)
			{
				if (getSParameterFromResponse(val_1, 1, "O_ESPLOG=", data, data_size))
				{
					log_level = atoi(val_1);
					if (g_device->trace_level != log_level)
						setLogLevel(log_level);
				}
				if (getSParameterFromResponse(val_1, 1, "O_NTP=", data, data_size))
				{
					O_NTP = atoi(val_1);
					if (g_device->ntp_mode != O_NTP)
						g_device->ntp_mode = O_NTP;
				}
				if (O_NTP == 1)
				{
					O_NTP0 = getParameterFromResponse("O_NTP0=", data, data_size);
					pathParse(O_NTP0);
					strcpy(g_device->ntp_server[0], O_NTP0);
					O_NTP1 = getParameterFromResponse("O_NTP1=", data, data_size);
					pathParse(O_NTP1);
					strcpy(g_device->ntp_server[1], O_NTP1);
					O_NTP2 = getParameterFromResponse("O_NTP2=", data, data_size);
					pathParse(O_NTP2);
					strcpy(g_device->ntp_server[2], O_NTP2);
					O_NTP3 = getParameterFromResponse("O_NTP3=", data, data_size);
					pathParse(O_NTP3);
					strcpy(g_device->ntp_server[3], O_NTP3);
					reboot = true;
				}
				O_TZONE = getParameterFromResponse("O_TZONE=", data, data_size);
				pathParse(O_TZONE);
				if (O_TZONE != g_device->tzone)
				{
					strcpy(g_device->tzone, O_TZONE);
					reboot = true;
				}

				if (getSParameterFromResponse(val_1, 1, "O_TIME_FORMAT=", data, data_size))
				{
					if (O_TIME_FORMAT != atoi(val_1))
					{
						if (atoi(val_1) == 0)
							g_device->options &= N_DDMM;
						else
							g_device->options |= Y_DDMM;
						reboot = true;
					}
				}
				if (getSParameterFromResponse(val_1, 1, "O_LCD_ROTA=", data, data_size))
				{
					if (O_LCD_ROTA != atoi(val_1))
					{
						if (atoi(val_1) == 0)
							g_device->options &= N_ROTAT;
						else
							g_device->options |= Y_ROTAT;
						reboot = true;
					}
				}
				saveDeviceSettings(g_device);

				// infree(ssid);
				// infree(pasw);
				// infree(ssid2);
				// infree(pasw2);
				// infree(aip);
				// infree(amsk);
				// infree(agw);
				// infree(aip2);
				// infree(amsk2);
				// infree(agw2);
			}
			int json_length = 122 +
							  1 + //log_level
							  1 + //   O_NTP
							  strlen(O_NTP0) +
							  strlen(O_NTP1) +
							  strlen(O_NTP2) +
							  strlen(O_NTP3) +
							  strlen(O_TZONE) +
							  1 + //O_TIME_FORMAT
							  1;  //O_LCD_ROTA

			char *buf = inmalloc(json_length + 95 + 39 + 10);
			if (buf == NULL)
			{
				ESP_LOGE(TAG, " %s malloc fails", "post devoptions");
				respKo(conn);
				//return;
			}
			else
			{
				sprintf(buf, DEVOPTIONS,
						json_length,
						log_level,
						O_NTP,
						O_NTP0,
						O_NTP1,
						O_NTP2,
						O_NTP3,
						O_TZONE,
						O_TIME_FORMAT,
						O_LCD_ROTA);
				ESP_LOGD(TAG, "TEST devoptions\nBuf len:%u\n%s", strlen(buf), buf);
				write(conn, buf, strlen(buf));
				infree(buf);
			}

			if (reboot)
			{
				copyDeviceSettings(); // save the current one
				fflush(stdout);
				vTaskDelay(100);
				esp_restart();
			}
			return;
		}
	}
	else if (strcmp(name, "/clear") == 0)
	{
		eeEraseStations(); //clear all stations
	}
	else if (strcmp(name, "/gpios") == 0)
	{
		if (data_size > 0)
		{
			changed = false;
			char arg[4];
			bool gpio_mode = false;
			g_device->options &= N_GPIOMODE;
			nvs_handle hardware_handle;
			esp_err_t err = ESP_OK;

			uint8_t spi_no;
			gpio_num_t miso, mosi, sclk, xcs, xdcs, dreq, enca, encb, encbtn, sda, scl, cs, a0, ir, led, tach, fanspeed, ds18b20, touch, buzzer, rxd, txd, ldr;
			if (getSParameterFromResponse(arg, 4, "save=", data, data_size))
				if (strcmp(arg, "1") == 0)
					changed = true;
			if (getSParameterFromResponse(arg, 4, "gpio_mode=", data, data_size))
				if ((strcmp(arg, "1") == 0))
				{
					gpio_mode = true;
					g_device->options |= Y_GPIOMODE;
				}
			if (gpio_mode)
			{
				err = open_partition("hardware", "gpio_space", (changed == false ? NVS_READONLY : NVS_READWRITE), &hardware_handle);
				if (err != ESP_OK)
				{
					g_device->options &= N_GPIOMODE;
				}
				else
					close_partition(hardware_handle, "hardware");
			}

			if (!changed)
			{
				err |= gpio_get_spi_bus(&spi_no, &miso, &mosi, &sclk);
				err |= gpio_get_vs1053(&xcs, &xdcs, &dreq);
				err |= gpio_get_encoders(&enca, &encb, &encbtn);
				err |= gpio_get_i2c(&sda, &scl);
				err |= gpio_get_spi_lcd(&cs, &a0);
				err |= gpio_get_ir_signal(&ir);
				err |= gpio_get_backlightl(&led);
				err |= gpio_get_tachometer(&tach);
				err |= gpio_get_fanspeed(&fanspeed);
				err |= gpio_get_ds18b20(&ds18b20);
				err |= gpio_get_touch(&touch);
				err |= gpio_get_buzzer(&buzzer);
				err |= gpio_get_uart(&rxd, &txd);
				err |= gpio_get_ldr(&ldr);
				if (err == 0x1102)
				{
					g_device->options &= N_GPIOMODE;

					err |= gpio_get_spi_bus(&spi_no, &miso, &mosi, &sclk);
					err |= gpio_get_vs1053(&xcs, &xdcs, &dreq);
					err |= gpio_get_encoders(&enca, &encb, &encbtn);
					err |= gpio_get_i2c(&sda, &scl);
					err |= gpio_get_spi_lcd(&cs, &a0);
					err |= gpio_get_ir_signal(&ir);
					err |= gpio_get_backlightl(&led);
					err |= gpio_get_tachometer(&tach);
					err |= gpio_get_fanspeed(&fanspeed);
					err |= gpio_get_ds18b20(&ds18b20);
					err |= gpio_get_touch(&touch);
					err |= gpio_get_buzzer(&buzzer);
					err |= gpio_get_uart(&rxd, &txd);
					err |= gpio_get_ldr(&ldr);
				}
			}
			else
			{
				getSParameterFromResponse(arg, 4, "K_SPI=", data, data_size);
				spi_no = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_MISO=", data, data_size);
				miso = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_MOSI=", data, data_size);
				mosi = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_CLK=", data, data_size);
				sclk = atoi(arg);
				err |= gpio_set_spi_bus(spi_no, miso, mosi, sclk);
				getSParameterFromResponse(arg, 4, "P_XCS=", data, data_size);
				xcs = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_XDCS=", data, data_size);
				xdcs = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_DREQ=", data, data_size);
				dreq = atoi(arg);
				err |= gpio_set_vs1053(xcs, xdcs, dreq);
				getSParameterFromResponse(arg, 4, "P_ENC0_A=", data, data_size);
				enca = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_ENC0_B=", data, data_size);
				encb = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_ENC0_BTN=", data, data_size);
				encbtn = atoi(arg);
				err |= gpio_set_encoders(enca, encb, encbtn);
				getSParameterFromResponse(arg, 4, "P_I2C_SCL=", data, data_size);
				sda = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_I2C_SDA=", data, data_size);
				scl = atoi(arg);
				err |= gpio_set_i2c(sda, scl);
				getSParameterFromResponse(arg, 4, "P_LCD_CS=", data, data_size);
				cs = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_LCD_A0=", data, data_size);
				a0 = atoi(arg);
				err |= gpio_set_spi_lcd(cs, a0);
				getSParameterFromResponse(arg, 4, "P_IR_SIGNAL=", data, data_size);
				ir = atoi(arg);
				err |= gpio_set_ir_signal(ir);
				getSParameterFromResponse(arg, 4, "P_BACKLIGHT=", data, data_size);
				led = atoi(arg);
				err |= gpio_set_backlightl(led);
				getSParameterFromResponse(arg, 4, "P_TACHOMETER=", data, data_size);
				tach = atoi(arg);
				err |= gpio_set_tachometer(tach);
				getSParameterFromResponse(arg, 4, "P_FAN_SPEED=", data, data_size);
				fanspeed = atoi(arg);
				err |= gpio_set_fanspeed(fanspeed);
				getSParameterFromResponse(arg, 4, "P_DS18B20=", data, data_size);
				ds18b20 = atoi(arg);
				err |= gpio_set_ds18b20(ds18b20);
				getSParameterFromResponse(arg, 4, "P_TOUCH_CS=", data, data_size);
				touch = atoi(arg);
				err |= gpio_set_touch(touch);
				getSParameterFromResponse(arg, 4, "P_BUZZER=", data, data_size);
				buzzer = atoi(arg);
				err |= gpio_set_buzzer(buzzer);
				getSParameterFromResponse(arg, 4, "P_RXD=", data, data_size);
				rxd = atoi(arg);
				getSParameterFromResponse(arg, 4, "P_TXD=", data, data_size);
				txd = atoi(arg);
				err |= gpio_set_uart(rxd, txd);
				getSParameterFromResponse(arg, 4, "P_LDR=", data, data_size);
				ldr = atoi(arg);
				err |= gpio_set_ldr(ldr);
				if (err != ESP_OK)
				{
					changed = false;
				}
			}

			int json_length = 432;
			char buf[77 + json_length];
			sprintf(buf, GPIOS,
					json_length,
					option_get_gpio_mode(),
					err,
					spi_no, (uint8_t)miso, (uint8_t)mosi, (uint8_t)sclk,
					(uint8_t)xcs, (uint8_t)xdcs, (uint8_t)dreq,
					(uint8_t)enca, (uint8_t)encb, (uint8_t)encbtn,
					(uint8_t)sda, (uint8_t)scl,
					(uint8_t)cs, (uint8_t)a0,
					(uint8_t)ir,
					(uint8_t)led,
					(uint8_t)tach,
					(uint8_t)fanspeed,
					(uint8_t)ds18b20,
					(uint8_t)touch,
					(uint8_t)buzzer,
					(uint8_t)rxd, (uint8_t)txd,
					(uint8_t)ldr);
			// ESP_LOGE(TAG, "Test GPIOS\nSave: %d\nERR: %X\nBuf len: %u\nBuf: %s\nData: %s\nData size: %d\n\n", changed, err, strlen(buf), buf, data, data_size);
			write(conn, buf, strlen(buf));
			if (changed)
			{
				option_set_gpio_mode(true);
				fflush(stdout);
				vTaskDelay(100);
				esp_restart();
			}
			return;
		}
	}
	else if (strcmp(name, "/ircodes") == 0)
	{
		if (data_size > 0)
		{
			changed = false;
			char buf_code[14];
			bool ir_mode = false;
			nvs_handle hardware_handle;
			esp_err_t err = ESP_OK;
			if (getSParameterFromResponse(buf_code, 14, "save=", data, data_size))
				if (strcmp(buf_code, "1") == 0)
					changed = true;
			if (getSParameterFromResponse(buf_code, 14, "ir_mode=", data, data_size))
				if ((strcmp(buf_code, "1") == 0))
				{
					ir_mode = true;
				}

			if (ir_mode)
			{
				err = open_partition("hardware", "gpio_space", (changed == false ? NVS_READONLY : NVS_READWRITE), &hardware_handle);
				if (err != ESP_OK)
				{
					ir_mode = false;
				}
				else
					close_partition(hardware_handle, "hardware");
			}
			if (changed)
			{
				ir_mode = false;
			}
			err |= get_ir_key(ir_mode);
			if (err == 0x1102)
			{
				err |= get_ir_key(false);
				changed = false;
			}
			if (changed)
			{
				getSParameterFromResponse(buf_code, 14, "K_0=", data, data_size);
				gpio_set_ir_key("K_UP", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_1=", data, data_size);
				gpio_set_ir_key("K_LEFT", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_2=", data, data_size);
				gpio_set_ir_key("K_OK", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_3=", data, data_size);
				gpio_set_ir_key("K_RIGHT", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_4=", data, data_size);
				gpio_set_ir_key("K_DOWN", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_5=", data, data_size);
				gpio_set_ir_key("K_0", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_6=", data, data_size);
				gpio_set_ir_key("K_1", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_7=", data, data_size);
				gpio_set_ir_key("K_2", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_8=", data, data_size);
				gpio_set_ir_key("K_3", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_9=", data, data_size);
				gpio_set_ir_key("K_4", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_10=", data, data_size);
				gpio_set_ir_key("K_5", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_11=", data, data_size);
				gpio_set_ir_key("K_6", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_12=", data, data_size);
				gpio_set_ir_key("K_7", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_13=", data, data_size);
				gpio_set_ir_key("K_8", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_14=", data, data_size);
				gpio_set_ir_key("K_9", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_15=", data, data_size);
				gpio_set_ir_key("K_STAR", buf_code);
				getSParameterFromResponse(buf_code, 14, "K_16=", data, data_size);
				gpio_set_ir_key("K_DIESE", buf_code);
				err |= get_ir_key(ir_mode);
				if (err != ESP_OK)
				{
					changed = false;
				}
			}
			char str_sodes[KEY_MAX][14];
			uint8_t len_codes = 0;
			for (uint8_t indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
			{
				len_codes += get_code(buf_code, IR_Key[indexKey][0], IR_Key[indexKey][1]);
				strcpy(str_sodes[indexKey], buf_code);
				ESP_LOGD(TAG, " IrKey for set0: %X, for set1: %X str_sodes: %s", IR_Key[indexKey][0], IR_Key[indexKey][1], str_sodes[indexKey]);
			}
			ir_mode = g_device->ir_mode;
			int json_length = 190 + len_codes;
			char buf[190 + len_codes];
			sprintf(buf, IRCODES,
					json_length,
					ir_mode,
					err,
					str_sodes[KEY_UP],
					str_sodes[KEY_LEFT],
					str_sodes[KEY_OK],
					str_sodes[KEY_RIGHT],
					str_sodes[KEY_DOWN],
					str_sodes[KEY_0],
					str_sodes[KEY_1],
					str_sodes[KEY_2],
					str_sodes[KEY_3],
					str_sodes[KEY_4],
					str_sodes[KEY_5],
					str_sodes[KEY_6],
					str_sodes[KEY_7],
					str_sodes[KEY_8],
					str_sodes[KEY_9],
					str_sodes[KEY_STAR],
					str_sodes[KEY_DIESE]);

			//ESP_LOGE(TAG, "Test IRCODE\nSave: %d\nERR: %X\nir_mode: %d\nBuf len: %u\nBuf: %s\nData: %s\nlen_codes size: %d\n\n", changed, err, ir_mode, strlen(buf), buf, data, len_codes);
			write(conn, buf, strlen(buf));
			if (changed)
			{
				g_device->ir_mode = IR_CUSTOM;
				saveDeviceSettings(g_device);
				fflush(stdout);
				vTaskDelay(100);
				esp_restart();
			}
			return;
		}
	}
	respOk(conn, NULL);
}

static bool httpServerHandleConnection(int conn, char *buf, uint16_t buflen)
{
	char *c;

	ESP_LOGD(TAG, "Heap size: %u", xPortGetFreeHeapSize());
	//printf("httpServerHandleConnection  %20c \n",&buf);
	if ((c = strstr(buf, "GET ")) != NULL)
	{
		char *d = strstr(buf, "Connection:"), *d1 = strstr(d, " Upgrade");
		ESP_LOGV(TAG, "GET socket:%d len: %u, str:\n%s", conn, buflen, buf);
		if ((d != NULL) && (d1 != NULL))
		{ // a websocket request
			websocketAccept(conn, buf, buflen);
			ESP_LOGD(TAG, "websocketAccept socket: %d", conn);
			return false;
		}
		else
		{
			c += 4;
			char *c_end = strstr(c, "HTTP");
			if (c_end == NULL)
				return true;
			*(c_end - 1) = 0;
			c_end = strstr(c, "?");
			//
			// web command api,
			///////////////////
			if (c_end != NULL) // commands api
			{
				char *param;
				//printf("GET commands  socket:%d command:%s\n",conn,c);
				// uart command
				param = strstr(c, "uart");
				if (param != NULL)
				{
					uart_set_baudrate(0, 115200);
				} //UART_SetBaudrate(0, 115200);}
				  // volume command
				param = getParameterFromResponse("volume=", c, strlen(c));
				if ((param != NULL) && (atoi(param) >= 0) && (atoi(param) <= 254))
				{
					setVolume(param);
					wsVol(param);
				}
				infree(param);
				// play command
				param = getParameterFromResponse("play=", c, strlen(c));
				if (param != NULL)
				{
					playStation(param);
					infree(param);
				}
				// start command
				param = strstr(c, "start");
				if (param != NULL)
				{
					playStationInt(getCurrentStation());
				}
				// stop command
				param = strstr(c, "stop");
				if (param != NULL)
				{
					clientDisconnect("Web stop");
				}
				// next command
				param = strstr(c, "next");
				if (param != NULL)
				{
					wsStationNext();
				}
				// prev command
				param = strstr(c, "prev");
				if (param != NULL)
				{
					wsStationPrev();
				}
				// instantplay command
				param = getParameterFromComment("instant=", c, strlen(c));
				if (param != NULL)
				{
					clientDisconnect("Web Instant");
					pathParse(param);
					clientParsePlaylist(param);
					infree(param);
					clientSetName("Instant Play", 255);
					clientConnectOnce();
					vTaskDelay(1);
				}
				// version command
				param = strstr(c, "version");
				if (param != NULL)
				{
					char vr[30 + strlen(RELEASE) + strlen(REVISION)];
					sprintf(vr, "Версия: %s Rev: %s\n", RELEASE, REVISION);
					respOk(conn, vr);
					return true;
				}
				// infos command
				param = strstr(c, "infos");
				if (param != NULL)
				{
					char *vr = webInfo();
					respOk(conn, vr);
					infree(vr);
					return true;
				}
				// list command	 ?list=1 to list the name of the station 1
				param = getParameterFromResponse("list=", c, strlen(c));
				if ((param != NULL) && (atoi(param) >= 0) && (atoi(param) <= 254))
				{
					char *vr = webList(atoi(param));
					respOk(conn, vr);
					infree(vr);
					return true;
				}
				respOk(conn, NULL); // response OK to the origin
			}
			else
			// file GET
			{
				if (strlen(c) > 32)
				{
					respKo(conn);
					return true;
				}
				ESP_LOGV(TAG, "GET file  socket:%d file:%s", conn, c);
				serveFile(c, conn);
				ESP_LOGV(TAG, "GET end socket:%d file:%s", conn, c);
			}
		}
	}
	else if ((c = strstr(buf, "POST ")) != NULL)
	{
		// a post request
		ESP_LOGV(TAG, "POST socket: %d  buflen: %u", conn, buflen);
		char fname[32];
		uint8_t i;
		for (i = 0; i < 32; i++)
			fname[i] = 0;
		c += 5;
		char *c_end = strstr(c, " ");
		if (c_end == NULL)
			return true;
		uint8_t len = c_end - c;
		if (len > 32)
			return true;
		strncpy(fname, c, len);
		ESP_LOGV(TAG, "POST Name: %s", fname);
		// DATA
		char *d_start = strstr(buf, "\r\n\r\n");
		//		ESP_LOGV(TAG,"dstart:%s",d_start);
		if (d_start != NULL)
		{
			d_start += 4;
			uint16_t Len = buflen - (d_start - buf);
			handlePOST(fname, d_start, Len, conn);
		}
	}
	return true;
}

#define RECLEN 768
#define DRECLEN (RECLEN * 2)
// Server child task to handle a request from a browser.
void serverclientTask(void *pvParams)
{
	struct timeval timeout;
	timeout.tv_sec = 6;
	timeout.tv_usec = 0;

	char buf[DRECLEN];
	portBASE_TYPE uxHighWaterMark;
	int client_sock = (int)pvParams;
	//   char *buf = (char *)inmalloc(reclen);
	bool result = true;

	if (buf != NULL)
	{
		int recbytes, recb;
		memset(buf, 0, DRECLEN);
		if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			printf(strsSOCKET, "setsockopt", errno);

		while (((recbytes = read(client_sock, buf, RECLEN)) != 0))
		{ // For now we assume max. RECLEN bytes for request
			if (recbytes < 0)
			{
				if (errno != EAGAIN)
				{
					printf(strsSOCKET, "client_sock", errno);
					vTaskDelay(10);
					break;
				}
				else
				{
					printf(strsSOCKET, tryagain, errno);
					break;
				}
				break;
			}
			char *bend = NULL;
			do
			{
				bend = strstr(buf, "\r\n\r\n");
				if (bend != NULL)
				{
					bend += 4;
					if (strstr(buf, "POST")) //rest of post?
					{
						uint16_t cl = atoi(strstr(buf, "Content-Length: ") + 16);
						vTaskDelay(1);
						if ((bend - buf + cl) > recbytes)
						{
							//printf ("Server: try receive more:%d bytes. , must be %d\n", recbytes,bend - buf +cl);
							while (((recb = read(client_sock, buf + recbytes, cl)) == 0) || (errno == EAGAIN))
							{
								vTaskDelay(1);
								if ((recb < 0) && (errno != EAGAIN))
								{
									ESP_LOGE(TAG, "read fails 0  errno:%d", errno);
									respKo(client_sock);
									break;
								}
								else
									recb = 0;
							}
							//							printf ("Server: received more for end now: %d bytes\n", recbytes+recb);
							buf[recbytes + recb] = 0;
							recbytes += recb;
						}
					}
				}
				else
				{

					//					printf ("Server: try receive more for end:%d bytes\n", recbytes);
					while (((recb = read(client_sock, buf + recbytes, DRECLEN - recbytes)) == 0) || (errno == EAGAIN))
					{
						vTaskDelay(1);
						//						printf ("Server: received more for end now: %d bytes\n", recbytes+recb);
						if ((recb < 0) && (errno != EAGAIN))
						{
							ESP_LOGE(TAG, "read fails 1  errno:%d", errno);
							respKo(client_sock);
							break;
						}
						else
							recb = 0;
					}
					recbytes += recb;
				} //until "\r\n\r\n"
			} while (bend == NULL);
			if (bend != NULL)
				result = httpServerHandleConnection(client_sock, buf, recbytes);
			memset(buf, 0, DRECLEN);
			if (!result)
			{
				break; // only a websocket created. exit without closing the socket
			}
			vTaskDelay(1);
		}
		//		infree(buf);
	}
	else
		printf(strsMALLOC1, "buf");
	if (result)
	{
		int err;
		err = close(client_sock);
		if (err != ERR_OK)
		{
			err = close(client_sock);
			ESP_LOGE(TAG, "closeERR:%d\n", err);
		}
	}
	xSemaphoreGive(semclient);
	ESP_LOGV(TAG, "Give client_sock: %d", client_sock);
	uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
	ESP_LOGD(TAG, "watermark serverClientTask: %x  %d", uxHighWaterMark, uxHighWaterMark);

	vTaskDelete(NULL);
}
