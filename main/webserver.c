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
#define SOCKETFAIL "WebServer Socket fails %s errno: %d\n"

xSemaphoreHandle semfile = NULL;

const char JSON_header[] = {"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n"};

const char strsROK[] = {"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: keep-alive\r\n\r\n%s"};

const char lowmemory[] = {"HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nlow memory\n"};

const char RAUTO[] = {"\
{\"rauto\":\"%c\"\
}"};
const char strsICY[] = {"\
{\
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
}"};

const char strsWIFI[] = {"\
{\
\"auto\":\"%u\",\
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
}"};

const char STATION[] = {"\
{\
\"Name\":\"%s\",\
\"URL\":\"%s\",\
\"File\":\"%s\",\
\"Port\":\"%d\",\
\"fav\":\"%u\",\
\"Total\":\"%u\"\
}"};

const char HARDWARE[] = {"\
{\
\"present\":\"%u\",\
\"audioinput\":\"%u\",\
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
}"};

const char VERSION[] = {"\
{\
\"RELEASE\":\"%s\",\
\"REVISION\":\"%s\",\
\"curtemp\":\"%05.1f\",\
\"rpmfan\":\"%04u\"\
}"};

const char GPIOS[] = {"\
{\
\"GPIO_MODE\":\"%u\",\
\"ERROR\":\"%04X\",\
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
}"};

const char DEVOPTIONS[] = {"\
{\
\"ESPLOG\":\"%u\",\
\"NTP\":\"%u\",\
\"NTP0\":\"%s\",\
\"NTP1\":\"%s\",\
\"NTP2\":\"%s\",\
\"NTP3\":\"%s\",\
\"TZONE\":\"%s\",\
\"TIME_FORMAT\":\"%u\",\
\"LCD_ROTA\":\"%u\"\
}"};

const char CONTROL[] = {"\
{\
\"lcd_brg\":\"%u\",\
\"begin_h\":\"%02u\",\
\"begin_m\":\"%02u\",\
\"end_h\":\"%02u\",\
\"end_m\":\"%02u\",\
\"day_brg\":\"%03u\",\
\"night_brg\":\"%03u\",\
\"hand_brg\":\"%03u\",\
\"fan_control\":\"%u\",\
\"min_temp\":\"%02u\",\
\"max_temp\":\"%02u\",\
\"min_pwm\":\"%03u\",\
\"hand_pwm\":\"%03u\",\
\"BRG_ON\":\"%u\",\
\"FAN_ON\":\"%u\",\
\"LDR\":\"%u\"\
}"};

const char IRCODES[] = {"\
{\
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

char *concat(const char *s1, const char *s2)
{

	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);

	char *result = malloc(len1 + len2 + 1);

	if (!result)
	{
		fprintf(stderr, "malloc() failed: insufficient memory!\n");
		return NULL;
	}

	memcpy(result, s1, len1);
	memcpy(result + len1, s2, len2 + 1);

	return result;
}

static uint32_t IR_Key[KEY_MAX][2];
esp_err_t get_ir_key(bool ir_mode)
{
	ir_key_e indexKey;
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
		err = open_storage(IRCODE_SPACE, NVS_READONLY, &handle);
		if (err == ESP_OK)
		{

			for (indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
			{
				// get the key in the nvs
				err |= nvs_get_ir_key(handle, klab[indexKey], (uint32_t *)&(IR_Key[indexKey][0]), (uint32_t *)&(IR_Key[indexKey][1]));
				taskYIELD();
			}
		}
		else
		{
			ESP_LOGE(TAG, "get ir_key fail, err No: 0x%x", err);
		}
		close_storage(handle);
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
void *incalloc(size_t n)
{
	void *ret = calloc(1, n);
	ESP_LOGV(TAG, "server Calloc of %x : %u bytes Heap size: %u", (int)ret, n, xPortGetFreeHeapSize());
	return ret;
}

void infree(void *p)
{
	ESP_LOGV(TAG, "server free of %x, Heap size: %u", (int)p, xPortGetFreeHeapSize());
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
	char fresp[strlen(strsROK) + strlen(message) + 15];
	sprintf(fresp, strsROK, "text/plain", (unsigned long)strlen(message), message);
	ESP_LOGV(TAG, "respOk %s", fresp);
	write(conn, fresp, strlen(fresp));
}

static void resp_low_memory(int conn)
{
	write(conn, lowmemory, strlen(lowmemory));
}

static void serveFile(char *name, int conn)
{
#define PART 1460
	int length;
	int gpart = PART;

	char *content;
	struct servFile *f = findFile(name);
	ESP_LOGV(TAG, "find %s at %x", name, (int)f);
	ESP_LOGV(TAG, "Heap size: %u", xPortGetFreeHeapSize());

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
				resp_low_memory(conn);
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
					resp_low_memory(conn);
					ESP_LOGE(TAG, "semfile fails 1 errno:%d", errno);
					xSemaphoreGive(semfile);
					return;
				}

				// ESP_LOGV(TAG,"serveFile socket:%d,  read at %x len: %d",conn,(int)content,(int)part);
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
			resp_low_memory(conn);
			ESP_LOGE(TAG, "semfile fails 2 errno:%d", errno);
			xSemaphoreGive(semfile);
			return;
		}
	}
	else
	{
		resp_low_memory(conn);
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
			char *t = incalloc(p_end - p + 1);
			if (t == NULL)
			{
				ESP_LOGE(TAG, "getParameterF fails\n");
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
static bool findParamFromResp(char *result, uint32_t size, const char *param, char *data, uint16_t data_length)
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

// treat the received message of the websocket
void websockethandle(int socket, wsopcode_e opcode, uint8_t *payload, size_t length)
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
}

void playStationInt(uint8_t id)
{
	beep(20);
	station_slot_s *si = getStation(id);
	char answer[24];

	sprintf(answer, "{\"wsstation\":\"%d\"}", id);
	setCurrentStation(id);

	id++;

	if (si != NULL && si->domain && si->file)
	{
		vTaskDelay(4);
		clientSilentDisconnect();
		ESP_LOGV(TAG, "playstationInt: %d, new station: %s", id, si->name);
		clientSetName(si->name, id);
		clientSetURL(si->domain);
		clientSetPath(si->file);
		clientSetPort(si->port);

		//ESP_LOGI(TAG, "Name: %s, url: %s, path: %s\n", si->name, si->domain, si->file);

		clientConnect();
		for (uint8_t i = 0; i < 100; i++)
		{
			if (clientIsConnected())
				break;
			vTaskDelay(5);
		}
	}
	infree(si);

	websocketbroadcast(answer, strlen(answer));
}

void playStation(char *id)
{
	if (atoi(id) < MainConfig->TotalStations)
		playStationInt(atoi(id));
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
	if (strcmp(name, "/local_play") == 0)
	{
		bool tst;
		char url[100];
		tst = findParamFromResp(url, 100, "url=", data, data_size);
		char path[200];
		tst &= findParamFromResp(path, 200, "path=", data, data_size);
		pathParse(path);
		char port[10];
		tst &= findParamFromResp(port, 10, "port=", data, data_size);
		if (tst)
		{
			clientDisconnect("Post local_play");
			for (i = 0; i < 100; i++)
			{
				if (!clientIsConnected())
					break;
				vTaskDelay(4);
			}
			const char *NoName = "БЕЗ ИМЕНИ";
			clientSetName(NoName, 0);
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
	else if (strcmp(name, "/soundvol") == 0)
	{
		char *vol = data + 4;
		data[data_size - 1] = 0;
		ESP_LOGD(TAG, "/sounvol vol: %s num:%d", vol, atoi(vol));
		setVolume(vol);
		respOk(conn, NULL);
		return;
	}
	else if (strcmp(name, "/sound") == 0)
	{
		char bass[6];
		char treble[6];
		char bassfreq[6];
		char treblefreq[6];
		char spacial[6];
		changed = false;
		if (findParamFromResp(bass, 6, "bass=", data, data_size))
		{
			if (MainConfig->bass != atoi(bass))
			{
				VS1053_SetBass(atoi(bass));
				changed = true;
				MainConfig->bass = atoi(bass);
			}
		}
		if (findParamFromResp(treble, 6, "treble=", data, data_size))
		{
			if (MainConfig->treble != atoi(treble))
			{
				VS1053_SetTreble(atoi(treble));
				changed = true;
				MainConfig->treble = atoi(treble);
			}
		}
		if (findParamFromResp(bassfreq, 6, "bassfreq=", data, data_size))
		{
			if (MainConfig->freqbass != atoi(bassfreq))
			{
				VS1053_SetBassFreq(atoi(bassfreq));
				changed = true;
				MainConfig->freqbass = atoi(bassfreq);
			}
		}
		if (findParamFromResp(treblefreq, 6, "treblefreq=", data, data_size))
		{
			if (MainConfig->freqtreble != atoi(treblefreq))
			{
				VS1053_SetTrebleFreq(atoi(treblefreq));
				changed = true;
				MainConfig->freqtreble = atoi(treblefreq);
			}
		}
		if (findParamFromResp(spacial, 6, "spacial=", data, data_size))
		{
			if (MainConfig->spacial != atoi(spacial))
			{
				VS1053_SetSpatial(atoi(spacial));
				changed = true;
				MainConfig->spacial = atoi(spacial);
			}
		}
		if (changed)
			SaveConfig();
	}
	else if (strcmp(name, "/getStation") == 0)
	{
		char id[3];
		if (findParamFromResp(id, 3, "ID=", data, data_size))
		{
			if ((atoi(id) >= 0) && (atoi(id) < MainConfig->TotalStations))
			{
				station_slot_s *si = getStation(atoi(id));
				if (si == NULL)
				{
					si = incalloc(sizeof(station_slot_s));
				}
				char *buf = incalloc(1024);

				if (buf == NULL)
				{
					ESP_LOGE(TAG, " %s malloc fails", "getStation");
					resp_low_memory(conn);
				}
				else
				{
					sprintf(buf, STATION,
							si->name,
							si->domain,
							si->file,
							si->port,
							si->fav,
							MainConfig->TotalStations);
					// ESP_LOGI(TAG, "TEST getStation\njson_length len:%u\n%s", strlen(buf), buf);
					int json_length = strlen(buf);
					char *s = concat(JSON_header, buf);

					sprintf(buf, s, json_length);
					free(s);

					write(conn, buf, strlen(buf));
				}
				infree(buf);
				infree(si);
				return;
			}
			else
				ESP_LOGE(TAG, "getstation, no id or Wrong id %d\n", atoi(id));
		}
	}
	else if (strcmp(name, "/setStation") == 0)
	{
		char save[1];
		if (findParamFromResp(save, 1, "save=", data, data_size))
		{
			if (strcmp(save, "1") == 0)
			{
				MainConfig->TotalStations = GetTotalStations();
				SaveConfig();
			}
			respOk(conn, NULL);
			return;
		}
		//ESP_LOGI(TAG, "data:%s\n",data);
		bool pState = getState(); // remember if we are playing

		station_slot_s *slot = incalloc(sizeof(station_slot_s));

		if (slot == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "setStation");
			resp_low_memory(conn);
			return;
		}
		// char *fillslot = (char *)slot;
		// int j;
		// for (j = 0; j < sizeof(station_slot_s); j++)
		// 	fillslot[j] = 0; //clean

		char FAV[1];
		char ID[3];
		char port[5];

		char *url = getParameterFromResponse("url=", data, data_size);
		char *file = getParameterFromResponse("file=", data, data_size);
		pathParse(file);
		char *Name = getParameterFromResponse("name=", data, data_size);
		findParamFromResp(FAV, 1, "fav=", data, data_size);
		findParamFromResp(ID, 3, "ID=", data, data_size);
		findParamFromResp(port, 5, "port=", data, data_size);

		// if (strlen(url) > sizeof(slot->domain))
		// 	url[sizeof(slot->domain) - 1] = 0; //truncate if any
		strcpy(slot->domain, url);
		// if (strlen(file) > sizeof(slot->file))
		// 	url[sizeof(slot->file) - 1] = 0; //truncate if any
		strcpy(slot->file, file);
		// if (strlen(Name) > sizeof(slot->name))
		// 	url[sizeof(slot->name) - 1] = 0; //truncate if any
		strcpy(slot->name, Name);
		slot->port = atoi(port);
		slot->fav = atoi(FAV);

		saveStation(slot, atoi(ID));
		infree(slot);
		if (pState != getState())
			if (pState)
			{
				clientConnect();
				vTaskDelay(200);
			} //we was playing so start again the play
	}
	else if (strcmp(name, "/play") == 0)
	{
		char *id = data + 3;
		data[data_size - 1] = 0;
		playStation(id);
	}
	else if (strcmp(name, "/auto") == 0)
	{
		char *id = data + 3;
		data[data_size - 1] = 0;
		if ((strcmp(id, "true")) && (MainConfig->autostart == 1))
		{
			MainConfig->autostart = 0;
			ESP_LOGV(TAG, "autostart: %s, num:%u", id, MainConfig->autostart);
			SaveConfig();
		}
		else if ((strcmp(id, "false")) && (MainConfig->autostart == 0))
		{
			MainConfig->autostart = 1;
			ESP_LOGV(TAG, "autostart: %s, num:%u", id, MainConfig->autostart);
			SaveConfig();
		}
	}
	else if (strcmp(name, "/rauto") == 0)
	{
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post RAUTO");
			resp_low_memory(conn);
		}
		else
		{
			sprintf(buf, RAUTO,
					(MainConfig->autostart) ? '1' : '0');

			// ESP_LOGI(TAG, "TEST RAUTO\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
		}
		infree(buf);
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
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post VERSION");
			resp_low_memory(conn);
		}
		else
		{
			sprintf(buf, VERSION,
					RELEASE,
					REVISION,
					getTemperature(),
					(uint16_t)getRpmFan() * 20);
			// ESP_LOGI(TAG, "TEST VERSION\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
		}
		infree(buf);
		return;
	}
	else if (strcmp(name, "/ircode") == 0) // start the IR TRAINING
	{
		char mode[1];
		if (findParamFromResp(mode, 1, "mode=", data, data_size))
		{
			if (strcmp(mode, "1") == 0)
				set_ir_training(true);
			else
				set_ir_training(false); //возвращаем режим работы пульта
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

		icy_header_s *header = clientGetHeader();
		ESP_LOGV(TAG, "icy start header %x", (int)header);
		char *not2;
		not2 = header->members.single.notice2;
		if (not2 == NULL)
			not2 = header->members.single.audioinfo;
		//if ((header->members.single.notice2 != NULL)&&(strlen(header->members.single.notice2)==0)) not2=header->members.single.audioinfo;

		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post ICY");
			resp_low_memory(conn);
		}
		else
		{
			uint8_t vauto = 0;
			vauto = (MainConfig->autostart) ? '1' : '0';
			sprintf(buf, strsICY,
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

			// ESP_LOGI(TAG, "TEST strsICY\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
			wsMonitor();
		}
		infree(buf);
		return;
	}
	else if (strcmp(name, "/hardware") == 0)
	{
		changed = false;
		char save[1];
		if (findParamFromResp(save, 1, "save=", data, data_size))
			if (strcmp(save, "1") == 0)
				changed = true;
		if (changed && tda7313_get_present())
		{
			char arg[2];
			findParamFromResp(arg, 2, "audioinput=", data, data_size);
			if (tda7313_get_input() != atoi(arg))
			{
				(tda7313_set_input(atoi(arg)));
				MainConfig->audio_input_num = atoi(arg);
			}
			findParamFromResp(arg, 2, "Volume=", data, data_size);
			if (tda7313_get_volume() != atoi(arg))
				(tda7313_set_volume(atoi(arg)));
			findParamFromResp(arg, 2, "Treble=", data, data_size);
			if (tda7313_get_treble() != atoi(arg))
				(tda7313_set_treble(atoi(arg)));
			findParamFromResp(arg, 2, "Bass=", data, data_size);
			if (tda7313_get_bass() != atoi(arg))
				(tda7313_set_bass(atoi(arg)));
			findParamFromResp(arg, 2, "rear_on=", data, data_size);
			if (tda7313_get_rear() != atoi(arg))
				(tda7313_set_rear(atoi(arg)));
			findParamFromResp(arg, 2, "attlf=", data, data_size);
			if (tda7313_get_attlf() != atoi(arg))
				(tda7313_set_attlf(atoi(arg)));
			findParamFromResp(arg, 2, "attrf=", data, data_size);
			if (tda7313_get_attrf() != atoi(arg))
				(tda7313_set_attrf(atoi(arg)));
			findParamFromResp(arg, 2, "attlr=", data, data_size);
			if (tda7313_get_attlr() != atoi(arg))
				(tda7313_set_attlr(atoi(arg)));
			findParamFromResp(arg, 2, "attrr=", data, data_size);
			if (tda7313_get_attrr() != atoi(arg))
				(tda7313_set_attrr(atoi(arg)));
			findParamFromResp(arg, 2, "loud=", data, data_size);
			if (tda7313_get_loud(MainConfig->audio_input_num) != atoi(arg))
				(tda7313_set_loud(atoi(arg)));
			findParamFromResp(arg, 2, "sla=", data, data_size);
			if (tda7313_get_sla(MainConfig->audio_input_num) != atoi(arg))
				(tda7313_set_sla(atoi(arg)));
			findParamFromResp(arg, 2, "mute=", data, data_size);
			if (tda7313_get_mute() != atoi(arg))
				(tda7313_set_mute(atoi(arg)));
			SaveConfig();
		}
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post HARDWARE");
			resp_low_memory(conn);
		}
		else
		{

			sprintf(buf, HARDWARE,
					tda7313_get_present(),
					(uint8_t)MainConfig->audio_input_num,
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
			// ESP_LOGI(TAG, "TEST HARDWARE\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
		}
		infree(buf);
		return;
	}
	else if (strcmp(name, "/wifi") == 0)
	{
		changed = false;
		char valid[5];
		if (findParamFromResp(valid, 5, "valid=", data, data_size))
			if (strcmp(valid, "1") == 0)
				changed = true;
		char *aua = getParameterFromResponse("ua=", data, data_size);
		pathParse(aua);
		char *host = getParameterFromResponse("host=", data, data_size);
		pathParse(host);

		if (changed)
		{
			if (findParamFromResp(valid, 5, "auto=", data, data_size))
			{
				if (strcmp(valid, "false") == 0)
					MainConfig->options &= N_WIFIAUTO;
				else
					MainConfig->options |= Y_WIFIAUTO;
				setAutoWifi();
			}
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

			ip_addr_t valu;
			if (aip != NULL)
			{
				ipaddr_aton(aip, &valu);
				memcpy(MainConfig->ipAddr1, &valu, sizeof(uint32_t));
				ipaddr_aton(amsk, &valu);
				memcpy(MainConfig->mask1, &valu, sizeof(uint32_t));
				ipaddr_aton(agw, &valu);
				memcpy(MainConfig->gate1, &valu, sizeof(uint32_t));
			}
			if (aip2 != NULL)
			{
				ipaddr_aton(aip2, &valu);
				memcpy(MainConfig->ipAddr2, &valu, sizeof(uint32_t));
				ipaddr_aton(amsk2, &valu);
				memcpy(MainConfig->mask2, &valu, sizeof(uint32_t));
				ipaddr_aton(agw2, &valu);
				memcpy(MainConfig->gate2, &valu, sizeof(uint32_t));
			}
			if (findParamFromResp(adhcp, 4, "dhcp=", data, data_size))
				if (strlen(adhcp) != 0)
				{
					if (strcmp(adhcp, "true") == 0)
						MainConfig->dhcpEn1 = 1;
					else
						MainConfig->dhcpEn1 = 0;
				}
			if (findParamFromResp(adhcp2, 4, "dhcp2=", data, data_size))
				if (strlen(adhcp2) != 0)
				{
					if (strcmp(adhcp2, "true") == 0)
						MainConfig->dhcpEn2 = 1;
					else
						MainConfig->dhcpEn2 = 0;
				}

			strcpy(MainConfig->ssid1, (ssid == NULL) ? "" : ssid);
			strcpy(MainConfig->pass1, (pasw == NULL) ? "" : pasw);
			strcpy(MainConfig->ssid2, (ssid2 == NULL) ? "" : ssid2);
			strcpy(MainConfig->pass2, (pasw2 == NULL) ? "" : pasw2);

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

		if ((MainConfig->ua != NULL) && (strlen(MainConfig->ua) == 0))
		{
			if (aua == NULL)
			{
				aua = incalloc(17);
				strcpy(aua, "ESP32Radiola/1.5");
			}
		}
		if (aua != NULL)
		{
			if ((strcmp(MainConfig->ua, aua) != 0) && (strcmp(aua, "undefined") != 0))
			{
				strcpy(MainConfig->ua, aua);
				changed = true;
			}
			infree(aua);
		}

		if (host != NULL)
		{
			if (strlen(host) > 0)
			{
				if ((strcmp(MainConfig->hostname, host) != 0) && (strcmp(host, "undefined") != 0))
				{
					strncpy(MainConfig->hostname, host, HOSTLEN - 1);
					setHostname(MainConfig->hostname);
					changed = true;
				}
			}
			infree(host);
		}

		uint8_t macaddr[10];
		char macstr[20];
		char adhcp[4], adhcp2[4];
		char tmpip[16], tmpmsk[16], tmpgw[16];
		char tmpip2[16], tmpmsk2[16], tmpgw2[16];
		esp_wifi_get_mac(WIFI_IF_STA, macaddr);
		sprintf(tmpip, "%u.%u.%u.%u", MainConfig->ipAddr1[0], MainConfig->ipAddr1[1], MainConfig->ipAddr1[2], MainConfig->ipAddr1[3]);
		sprintf(tmpmsk, "%u.%u.%u.%u", MainConfig->mask1[0], MainConfig->mask1[1], MainConfig->mask1[2], MainConfig->mask1[3]);
		sprintf(tmpgw, "%u.%u.%u.%u", MainConfig->gate1[0], MainConfig->gate1[1], MainConfig->gate1[2], MainConfig->gate1[3]);
		sprintf(adhcp, "%u", MainConfig->dhcpEn1);
		sprintf(tmpip2, "%u.%u.%u.%u", MainConfig->ipAddr2[0], MainConfig->ipAddr2[1], MainConfig->ipAddr2[2], MainConfig->ipAddr2[3]);
		sprintf(tmpmsk2, "%u.%u.%u.%u", MainConfig->mask2[0], MainConfig->mask2[1], MainConfig->mask2[2], MainConfig->mask2[3]);
		sprintf(tmpgw2, "%u.%u.%u.%u", MainConfig->gate2[0], MainConfig->gate2[1], MainConfig->gate2[2], MainConfig->gate2[3]);
		sprintf(adhcp2, "%u", MainConfig->dhcpEn2);
		sprintf(macstr, MACSTR, MAC2STR(macaddr));
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post WIFI");
			resp_low_memory(conn);
		}
		else
		{
			sprintf(buf, strsWIFI,
					getAutoWifi(),
					MainConfig->ssid1,
					MainConfig->pass1,
					MainConfig->ssid2,
					MainConfig->pass2,
					tmpip,
					tmpmsk,
					tmpgw,
					tmpip2,
					tmpmsk2,
					tmpgw2,
					MainConfig->ua,
					adhcp,
					adhcp2,
					macstr,
					MainConfig->hostname);
			// ESP_LOGI(TAG, "TEST WIFI\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);
			write(conn, buf, strlen(buf));
		}
		infree(buf);
		if (changed)
		{
			// set current_ap to the first filled ssid
			ESP_LOGD(TAG, "currentAP: %u", MainConfig->current_ap);
			if (MainConfig->current_ap == APMODE)
			{
				if (strlen(MainConfig->ssid1) != 0)
					MainConfig->current_ap = STA1;
				else if (strlen(MainConfig->ssid2) != 0)
					MainConfig->current_ap = STA2;
			}
			ESP_LOGD(TAG, "currentAP: %u", MainConfig->current_ap);
			SaveConfig(); // save the current one
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}
		return;
	}
	else if (strcmp(name, "/control") == 0)
	{
		changed = false;
		/*
\"lcd_brg\":\"%u\",\
\"begin_h\":\"%02u\",\
\"begin_m\":\"%02u\",\
\"end_h\":\"%02u\",\
\"end_m\":\"%02u\",\
\"day_brg\":\"%03u\",\
\"night_brg\":\"%03u\",\
\"hand_brg\":\"%03u\",\
\"fan_control\":\"%u\",\
\"min_temp\":\"%02u\",\
\"max_temp\":\"%02u\",\
\"min_pwm\":\"%03u\",\
\"hand_pwm\":\"%03u\",\
\"BRG_ON\":\"%u\",\
\"FAN_ON\":\"%u\",\
\"LDR\":\"%u\"\

 */

		u_int8_t lcd_brg = MainConfig->backlight_mode;
		u_int8_t begin_h = MainConfig->begin_h;
		u_int8_t begin_m = MainConfig->begin_m;
		u_int8_t end_h = MainConfig->end_h;
		u_int8_t end_m = MainConfig->end_m;
		u_int8_t day_brg = MainConfig->day_brightness;
		u_int8_t night_brg = MainConfig->night_brightness;
		u_int8_t hand_brg = MainConfig->hand_brightness;
		u_int8_t fan_control = 0, min_temp = 0, max_temp = 0, min_pwm = 0, hand_pwm = 0;
		u_int8_t BRG_ON;
		u_int8_t FAN_ON;
		u_int8_t LDR = MainConfig->ldr;

		get_bright(&BRG_ON);
		get_fan(&FAN_ON);
		char val_1[1];
		if (findParamFromResp(val_1, 1, "save=", data, data_size))
			if (strcmp(val_1, "1") == 0)
				changed = true;
		if (changed)
		{
			if (findParamFromResp(val_1, 1, "lcd_brg=", data, data_size))
			{
				lcd_brg = atoi(val_1);
				if (MainConfig->backlight_mode != lcd_brg)
					MainConfig->backlight_mode = lcd_brg;
			}
			if (findParamFromResp(val_1, 1, "begin_h=", data, data_size))
			{
				begin_h = atoi(val_1);
			}
			if (findParamFromResp(val_1, 1, "begin_m=", data, data_size))
			{
				begin_m = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "end_h=", data, data_size))
			{
				end_h = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "end_m=", data, data_size))
			{
				end_m = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "day_brg=", data, data_size))
			{
				day_brg = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "night_brg=", data, data_size))
			{
				night_brg = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "hand_brg=", data, data_size))
			{
				hand_brg = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "fan_control=", data, data_size))
			{
				fan_control = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "min_temp=", data, data_size))
			{
				min_temp = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "max_temp=", data, data_size))
			{
				max_temp = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "min_pwm=", data, data_size))
			{
				min_pwm = atoi(val_1);
			}

			if (findParamFromResp(val_1, 1, "hand_pwm=", data, data_size))
			{
				hand_pwm = atoi(val_1);
			}

			SaveConfig();
		}

		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post CONTROL");
			resp_low_memory(conn);
		}
		else
		{
			sprintf(buf, CONTROL,
					lcd_brg,
					begin_h,
					begin_m,
					end_h,
					end_m,
					day_brg,
					night_brg,
					hand_brg,
					fan_control,
					min_temp,
					max_temp,
					min_pwm,
					hand_pwm,
					BRG_ON,
					FAN_ON,
					LDR);
			// ESP_LOGI(TAG, "TEST CONTROL\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
		}
		infree(buf);
		return;
	}
	else if (strcmp(name, "/devoptions") == 0)
	{
		changed = false;
		esp_log_level_t log_level = MainConfig->trace_level;
		uint8_t NTP = MainConfig->ntp_mode;
		uint8_t LCD_ROTA = get_lcd_rotat();
		uint8_t TIME_FORMAT = getDataFormat();
		char *NTP0 = CONFIG_NTP0, *NTP1 = CONFIG_NTP1, *NTP2 = CONFIG_NTP2, *NTP3 = CONFIG_NTP3;
		char *TZONE = MainConfig->tzone;
		bool reboot = false;
		bool erase = false;
		char val_1[1];
		if (NTP == 1)
		{
			NTP0 = MainConfig->ntp_server[0];
			NTP1 = MainConfig->ntp_server[1];
			NTP2 = MainConfig->ntp_server[2];
			NTP3 = MainConfig->ntp_server[3];
		}
		if (findParamFromResp(val_1, 1, "save=", data, data_size))
			if (strcmp(val_1, "1") == 0)
			{
				changed = true;
			}
		if (strcmp(val_1, "2") == 0)
		{
			erase = true;
			reboot = true;
		}
		if (changed)
		{
			if (findParamFromResp(val_1, 1, "ESPLOG=", data, data_size))
			{
				log_level = atoi(val_1);
				if (MainConfig->trace_level != log_level)
					setLogLevel(log_level);
			}
			if (findParamFromResp(val_1, 1, "NTP=", data, data_size))
			{
				NTP = atoi(val_1);
				if (MainConfig->ntp_mode != NTP)
					MainConfig->ntp_mode = NTP;
			}
			if (NTP == 1)
			{
				NTP0 = getParameterFromResponse("NTP0=", data, data_size);
				pathParse(NTP0);
				strcpy(MainConfig->ntp_server[0], NTP0);
				NTP1 = getParameterFromResponse("NTP1=", data, data_size);
				pathParse(NTP1);
				strcpy(MainConfig->ntp_server[1], NTP1);
				NTP2 = getParameterFromResponse("NTP2=", data, data_size);
				pathParse(NTP2);
				strcpy(MainConfig->ntp_server[2], NTP2);
				NTP3 = getParameterFromResponse("NTP3=", data, data_size);
				pathParse(NTP3);
				strcpy(MainConfig->ntp_server[3], NTP3);
				reboot = true;
			}
			TZONE = getParameterFromResponse("TZONE=", data, data_size);
			pathParse(TZONE);
			if (TZONE != MainConfig->tzone)
			{
				strcpy(MainConfig->tzone, TZONE);
				reboot = true;
			}

			if (findParamFromResp(val_1, 1, "TIME_FORMAT=", data, data_size))
			{
				if (TIME_FORMAT != atoi(val_1))
				{
					if (atoi(val_1) == 0)
						MainConfig->options &= N_DDMM;
					else
						MainConfig->options |= Y_DDMM;
					reboot = true;
				}
			}
			if (findParamFromResp(val_1, 1, "LCD_ROTA=", data, data_size))
			{
				if (LCD_ROTA != atoi(val_1))
				{
					if (atoi(val_1) == 0)
						MainConfig->options &= N_ROTAT;
					else
						MainConfig->options |= Y_ROTAT;
					reboot = true;
				}
			}
			SaveConfig();
		}
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post DEVOPTIONS");
			resp_low_memory(conn);
		}
		else
		{
			sprintf(buf, DEVOPTIONS,
					log_level,
					NTP,
					NTP0,
					NTP1,
					NTP2,
					NTP3,
					TZONE,
					TIME_FORMAT,
					LCD_ROTA);
			// ESP_LOGI(TAG, "TEST devoptions\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);
			write(conn, buf, strlen(buf));
		}
		infree(buf);
		if (reboot)
		{
			if (erase)
				eeEraseAll(); // стереть все настройки NVS
			else
				SaveConfig(); // сохранить текущие настройки в NVS
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}
		return;
	}
	else if (strcmp(name, "/clear") == 0)
	{
		// clear all stations
		EraseNameSpace(STATION_SPACE);
	}
	else if (strcmp(name, "/gpios") == 0)
	{
		changed = false;
		char arg[4];
		bool gpio_mode = get_gpio_mode();
		set_gpio_mode(false);
		nvs_handle gpio_handle;
		esp_err_t err = ESP_OK;

		gpio_num_t miso, mosi, sclk, xcs, xdcs, dreq, enca, encb, encbtn, sda, scl, cs, a0, ir, led, tach, fanspeed, ds18b20, touch, buzzer, rxd, txd, ldr;

		if (findParamFromResp(arg, 4, "gpio_mode=", data, data_size))
			if ((strcmp(arg, "1") == 0))
			{
				set_gpio_mode(true);
			}

		if (findParamFromResp(arg, 4, "save=", data, data_size))
			if (strcmp(arg, "1") == 0)
				changed = true;

		if (findParamFromResp(arg, 4, "load=", data, data_size))
			if (strcmp(arg, "1") == 0)
				set_gpio_mode(gpio_mode);

		if (get_gpio_mode())
		{
			err = open_storage(GPIOS_SPACE, (changed == false ? NVS_READONLY : NVS_READWRITE), &gpio_handle);
			if (err != ESP_OK)
			{
				set_gpio_mode(false);
			}
			else
				close_storage(gpio_handle);
		}

		if (!changed)
		{
			err |= gpio_get_spi_bus(&miso, &mosi, &sclk);
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
				set_gpio_mode(false);

				err |= gpio_get_spi_bus(&miso, &mosi, &sclk);
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
			findParamFromResp(arg, 4, "P_MISO=", data, data_size);
			miso = atoi(arg);
			findParamFromResp(arg, 4, "P_MOSI=", data, data_size);
			mosi = atoi(arg);
			findParamFromResp(arg, 4, "P_CLK=", data, data_size);
			sclk = atoi(arg);
			err |= gpio_set_spi_bus(miso, mosi, sclk);
			findParamFromResp(arg, 4, "P_XCS=", data, data_size);
			xcs = atoi(arg);
			findParamFromResp(arg, 4, "P_XDCS=", data, data_size);
			xdcs = atoi(arg);
			findParamFromResp(arg, 4, "P_DREQ=", data, data_size);
			dreq = atoi(arg);
			err |= gpio_set_vs1053(xcs, xdcs, dreq);
			findParamFromResp(arg, 4, "P_ENC0_A=", data, data_size);
			enca = atoi(arg);
			findParamFromResp(arg, 4, "P_ENC0_B=", data, data_size);
			encb = atoi(arg);
			findParamFromResp(arg, 4, "P_ENC0_BTN=", data, data_size);
			encbtn = atoi(arg);
			err |= gpio_set_encoders(enca, encb, encbtn);
			findParamFromResp(arg, 4, "P_I2C_SCL=", data, data_size);
			sda = atoi(arg);
			findParamFromResp(arg, 4, "P_I2C_SDA=", data, data_size);
			scl = atoi(arg);
			err |= gpio_set_i2c(sda, scl);
			findParamFromResp(arg, 4, "P_LCD_CS=", data, data_size);
			cs = atoi(arg);
			findParamFromResp(arg, 4, "P_LCD_A0=", data, data_size);
			a0 = atoi(arg);
			err |= gpio_set_spi_lcd(cs, a0);
			findParamFromResp(arg, 4, "P_IR_SIGNAL=", data, data_size);
			ir = atoi(arg);
			err |= gpio_set_ir_signal(ir);
			findParamFromResp(arg, 4, "P_BACKLIGHT=", data, data_size);
			led = atoi(arg);
			err |= gpio_set_backlightl(led);
			findParamFromResp(arg, 4, "P_TACHOMETER=", data, data_size);
			tach = atoi(arg);
			err |= gpio_set_tachometer(tach);
			findParamFromResp(arg, 4, "P_FAN_SPEED=", data, data_size);
			fanspeed = atoi(arg);
			err |= gpio_set_fanspeed(fanspeed);
			findParamFromResp(arg, 4, "P_DS18B20=", data, data_size);
			ds18b20 = atoi(arg);
			err |= gpio_set_ds18b20(ds18b20);
			findParamFromResp(arg, 4, "P_TOUCH_CS=", data, data_size);
			touch = atoi(arg);
			err |= gpio_set_touch(touch);
			findParamFromResp(arg, 4, "P_BUZZER=", data, data_size);
			buzzer = atoi(arg);
			err |= gpio_set_buzzer(buzzer);
			findParamFromResp(arg, 4, "P_RXD=", data, data_size);
			rxd = atoi(arg);
			findParamFromResp(arg, 4, "P_TXD=", data, data_size);
			txd = atoi(arg);
			err |= gpio_set_uart(rxd, txd);
			findParamFromResp(arg, 4, "P_LDR=", data, data_size);
			ldr = atoi(arg);
			err |= gpio_set_ldr(ldr);
			if (err != ESP_OK)
			{
				changed = false;
			}
		}
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post GPIOS");
			changed = false;
			resp_low_memory(conn);
		}
		else
		{
			set_gpio_mode(gpio_mode);
			sprintf(buf, GPIOS,
					gpio_mode,
					err,
					(uint8_t)miso, (uint8_t)mosi, (uint8_t)sclk,
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
			// ESP_LOGI(TAG, "TEST GPIOS\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);

			write(conn, buf, strlen(buf));
		}
		infree(buf);
		if (changed)
		{
			set_gpio_mode(true);
			SaveConfig();
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}
		return;
	}
	else if (strcmp(name, "/ircodes") == 0)
	{
		changed = false;
		char buf_code[14];
		bool ir_mode = false;
		nvs_handle gpio_handle;
		esp_err_t err = ESP_OK;
		if (findParamFromResp(buf_code, 14, "save=", data, data_size))
			if (strcmp(buf_code, "1") == 0)
				changed = true;
		if (findParamFromResp(buf_code, 14, "ir_mode=", data, data_size))
			if ((strcmp(buf_code, "1") == 0))
			{
				ir_mode = true;
			}
		if (findParamFromResp(buf_code, 14, "load=", data, data_size))
			if (strcmp(buf_code, "1") == 0)
				ir_mode = MainConfig->ir_mode;
		if (ir_mode)
		{
			err = open_storage(IRCODE_SPACE, (changed == false ? NVS_READONLY : NVS_READWRITE), &gpio_handle);
			if (err != ESP_OK)
			{
				ir_mode = false;
			}
			else
				close_storage(gpio_handle);
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
			findParamFromResp(buf_code, 14, "K_0=", data, data_size);
			nvs_set_ir_key("K_UP", buf_code);
			findParamFromResp(buf_code, 14, "K_1=", data, data_size);
			nvs_set_ir_key("K_LEFT", buf_code);
			findParamFromResp(buf_code, 14, "K_2=", data, data_size);
			nvs_set_ir_key("K_OK", buf_code);
			findParamFromResp(buf_code, 14, "K_3=", data, data_size);
			nvs_set_ir_key("K_RIGHT", buf_code);
			findParamFromResp(buf_code, 14, "K_4=", data, data_size);
			nvs_set_ir_key("K_DOWN", buf_code);
			findParamFromResp(buf_code, 14, "K_5=", data, data_size);
			nvs_set_ir_key("K_0", buf_code);
			findParamFromResp(buf_code, 14, "K_6=", data, data_size);
			nvs_set_ir_key("K_1", buf_code);
			findParamFromResp(buf_code, 14, "K_7=", data, data_size);
			nvs_set_ir_key("K_2", buf_code);
			findParamFromResp(buf_code, 14, "K_8=", data, data_size);
			nvs_set_ir_key("K_3", buf_code);
			findParamFromResp(buf_code, 14, "K_9=", data, data_size);
			nvs_set_ir_key("K_4", buf_code);
			findParamFromResp(buf_code, 14, "K_10=", data, data_size);
			nvs_set_ir_key("K_5", buf_code);
			findParamFromResp(buf_code, 14, "K_11=", data, data_size);
			nvs_set_ir_key("K_6", buf_code);
			findParamFromResp(buf_code, 14, "K_12=", data, data_size);
			nvs_set_ir_key("K_7", buf_code);
			findParamFromResp(buf_code, 14, "K_13=", data, data_size);
			nvs_set_ir_key("K_8", buf_code);
			findParamFromResp(buf_code, 14, "K_14=", data, data_size);
			nvs_set_ir_key("K_9", buf_code);
			findParamFromResp(buf_code, 14, "K_15=", data, data_size);
			nvs_set_ir_key("K_STAR", buf_code);
			findParamFromResp(buf_code, 14, "K_16=", data, data_size);
			nvs_set_ir_key("K_DIESE", buf_code);
			err |= get_ir_key(ir_mode);
			if (err != ESP_OK)
			{
				changed = false;
			}
		}
		char *buf = incalloc(1024);
		if (buf == NULL)
		{
			ESP_LOGE(TAG, " %s malloc fails", "post IRCODES");
			resp_low_memory(conn);
		}
		else
		{
			char str_sodes[KEY_MAX][14];
			for (uint8_t indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
			{
				get_code(buf_code, IR_Key[indexKey][0], IR_Key[indexKey][1]);
				strcpy(str_sodes[indexKey], buf_code);
				ESP_LOGD(TAG, " IrKey for set0: %X, for set1: %X str_sodes: %s", IR_Key[indexKey][0], IR_Key[indexKey][1], str_sodes[indexKey]);
			}
			ir_mode = MainConfig->ir_mode;
			sprintf(buf, IRCODES,
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

			// ESP_LOGI(TAG, "TEST IRCODES\njson_length len:%u\n%s", strlen(buf), buf);
			int json_length = strlen(buf);
			char *s = concat(JSON_header, buf);

			sprintf(buf, s, json_length);
			free(s);
			write(conn, buf, strlen(buf));
		}
		infree(buf);
		if (changed)
		{
			MainConfig->ir_mode = IR_CUSTOM;
			SaveConfig();
			fflush(stdout);
			vTaskDelay(100);
			esp_restart();
		}
		return;
	}
	respOk(conn, NULL);
}

static bool httpServerHandleConnection(int conn, char *buf, uint16_t buflen)
{
	char *c;

	ESP_LOGD(TAG, "Heap size: %u", xPortGetFreeHeapSize());
	//ESP_LOGI(TAG, "httpServerHandleConnection  %20c \n",&buf);
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
				//ESP_LOGI(TAG, "GET commands  socket:%d command:%s\n",conn,c);
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
					clientSetName("Instant Play", 0);
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

				if ((param != NULL) && (atoi(param) >= 0) && (atoi(param) <= MainConfig->TotalStations))
				{
					station_slot_s *si = incalloc(sizeof(station_slot_s));
					si = getStation(atoi(param));
					char *name = incalloc(64);
					if (si != NULL)
					{
						name = si->name;
						respOk(conn, name);
					}
					else
					{
						resp_low_memory(conn);
					}

					infree(si);
					infree(name);
					return true;
				}
				respOk(conn, NULL); // response OK to the origin
			}
			else
			// file GET
			{
				if (strlen(c) > 32)
				{
					resp_low_memory(conn);
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
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	char buf[DRECLEN];
	portBASE_TYPE uxHighWaterMark;
	int client_sock = (int)pvParams;

	bool result = true;

	if (buf != NULL)
	{
		int recbytes, recb;
		memset(buf, 0, DRECLEN);
		if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
			ESP_LOGE(TAG, SOCKETFAIL, "setsockopt", errno);

		while (((recbytes = read(client_sock, buf, RECLEN)) != 0))
		{ // For now we assume max. RECLEN bytes for request
			if (recbytes < 0)
			{
				if (errno != EAGAIN)
				{
					ESP_LOGE(TAG, SOCKETFAIL, "client_sock", errno);
					vTaskDelay(10);
					break;
				}
				else
				{
					ESP_LOGI(TAG, SOCKETFAIL, "try again", errno);
					// ESP_LOGW(TAG, "buf:\n%s\n", buf);
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
							// ESP_LOGI(TAG, "Server: try receive more:%d bytes. , must be %d\n", recbytes,bend - buf +cl);
							while (((recb = read(client_sock, buf + recbytes, cl)) == 0) || (errno == EAGAIN))
							{
								vTaskDelay(1);
								if ((recb < 0) && (errno != EAGAIN))
								{
									ESP_LOGE(TAG, "read fails 0  errno:%d", errno);
									resp_low_memory(client_sock);
									break;
								}
								else
									recb = 0;
							}
							// ESP_LOGI(TAG, "Server: received more for end now: %d bytes\n", recbytes+recb);
							buf[recbytes + recb] = 0;
							recbytes += recb;
						}
					}
				}
				else
				{

					// ESP_LOGI(TAG, "Server: try receive more for end:%d bytes\n", recbytes);
					while (((recb = read(client_sock, buf + recbytes, DRECLEN - recbytes)) == 0) || (errno == EAGAIN))
					{
						vTaskDelay(1);
						// ESP_LOGI(TAG, "Server: received more for end now: %d bytes\n", recbytes+recb);
						if ((recb < 0) && (errno != EAGAIN))
						{
							ESP_LOGE(TAG, "read fails 1  errno:%d", errno);
							resp_low_memory(client_sock);
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
		ESP_LOGE(TAG, "WebServer buf malloc fails\n");
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
