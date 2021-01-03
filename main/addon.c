/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include "driver/ledc.h"

#include "ClickEncoder.h"
#include "main.h"
#include "gpios.h"
#include "webclient.h"
#include "webserver.h"
#include "interface.h"

#include "addon.h"
#include "custom.h"
#include "ucg_esp32_hal.h"
#include <time.h>

#include "eeprom.h"
#include "addonucg.h"
#include "xpt2046.h"
#include "bt_x01.h"

#define TAG "addon"

static void evtClearScreen();
// время в секундах до отбражения часов после остановки проигрывания мелодий
#define TO_TIME_SCREEN 60
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static uint8_t old_backlight_level = 0; // Предыдущее состояние уровня подсветки дисплея

const char *stopped = "ОСТАНОВЛЕН";
const char *starting = "ЗАПУСК";

char StNumStr[4];
xQueueHandle event_ir = NULL;
xQueueHandle event_lcd = NULL;
ucg_t ucg;
static xTaskHandle pxTaskLcd;
// list of screen
typedef enum typeScreen
{
	EmptyScreen,
	MainScreen,
	VolumeScreen,
	StationScreen,
	NumberScreen,
	TimeScreen,

} type_screen_e;
static type_screen_e stateScreen = EmptyScreen;
static type_screen_e defaultStateScreen = MainScreen;
// state of the transient screen
static uint8_t mTscreen = MTNEW; // 0 dont display, 1 display full, 2 display variable part

static bool playable = true;
bool ir_training = false;
static uint8_t volume;
static uint8_t StNum = 0; // Номер станции, отображаемый на дисплее

static unsigned timerScreen = 0;
static unsigned timerLcdOut = 0;
static unsigned timer1s = 0;

static unsigned timein = 0;
static bool itAskStime = false; // start the time display
static uint8_t itLcdOut = 0;
//static bool itAskSsecond = false; // start the time display
static bool state = false; // start stop on Ok key

static int16_t currentValue = 0;
static bool dvolume = false; // display volume screen
static uint32_t ircode;

static uint32_t IR_Key[KEY_MAX][2];

static bool isEncoder = true;

void Screen(type_screen_e st);
void drawScreen();
static void evtScreen(lcd_cmd_e value);

encoder_s *encoder = NULL;

static adc1_channel_t ldr_channel, kbd_channel = GPIO_NONE;
static bool inside = false;

void *getEncoder()
{
	return (void *)encoder;
}

static void ClearBuffer()
{
	ucg_ClearScreen(&ucg);
}

static int16_t DrawString(int16_t x, int16_t y, const char *str)
{
	return ucg_DrawString(&ucg, x, y, 0, str);
}

static void DrawColor(uint8_t color, uint8_t r, uint8_t g, uint8_t b)
{
	ucg_SetColor(&ucg, 0, r, g, b);
}

static void DrawBox(ucg_int_t x, ucg_int_t y, ucg_int_t w, ucg_int_t h)
{
	ucg_DrawBox(&ucg, x, y, w, h);
}

uint16_t GetWidth()
{
	return ucg_GetWidth(&ucg);
}
uint16_t GetHeight()
{
	return ucg_GetHeight(&ucg);
}

void wakeLcd()
{
	// add the gpio switch on here gpioLedBacklight can be directly a GPIO_NUM_xx or declared in gpio.h
	SetLedBacklight();
	timerLcdOut = getLcdOut(); // rearm the tempo
	if (itLcdOut)
	{
		mTscreen = MTNEW;
		evtScreen(defaultStateScreen);
	}
	itLcdOut = 0; //0 not activated, 1 sleep requested, 2 in sleep
}

void sleepLcd()
{
	itLcdOut = 2; // in sleep
	// add the gpio switch off here
	MainConfig->backlight_level = 0;
	SetLedBacklight();
	evtClearScreen();
}

void lcd_init()
{
	uint8_t rt = get_lcd_rotat();
	ESP_LOGI(TAG, "LCD Rotat %d", rt);
	//lcd rotation
	setRotat(rt);
	// init the gpio for backlight
	LedBacklightInit();
	lcd_initUcg();
	setVolumeUcg(MainConfig->vol);
	vTaskDelay(1);
}

void lcd_welcome(char *ip, char *state)
{
	char verInfo[30 + strlen(RELEASE) + strlen(REVISION)];
	char *text1 = strdup(state);
	removeUtf8(text1);
	if ((strlen(ip) == 0) && (strlen(state) == 0))
		ClearBuffer();
	setfont(2);
	int y = -ucg_GetFontDescent(&ucg) + ucg_GetFontAscent(&ucg) + 3; //interline
	sprintf(verInfo, "ESP32-РАДИОЛА");
	removeUtf8(verInfo);
	DrawString((GetWidth() / 2) - (ucg_GetStrWidth(&ucg, verInfo) / 2), 5, verInfo);
	setfont(1);
	sprintf(verInfo, "ВЕРСИЯ: %s Rev: %s\n", RELEASE, REVISION);
	removeUtf8(verInfo);

	DrawString(2, 2 * y, verInfo);
	DrawColor(0, 0, 0, 0);
	DrawBox(2, 4 * y, GetWidth() - 2, y);
	DrawColor(1, 255, 255, 255);
	DrawString(2, 4 * y, text1);
	if (strlen(ip) != 0)
		DrawString(DrawString(2, 5 * y, "IP:") + 18, 5 * y, ip);
}

// ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//
void (*serviceAddon)() = NULL;
IRAM_ATTR void ServiceAddon(void)
{
	timer1s++;
	if (timer1s >= 1000)
	{
		// Time compute
		//timestamp++; // time update
		if (timerLcdOut > 0)
			timerLcdOut--; //
		timein++;

		if (((timein % TO_TIME_SCREEN) == 0) && (!state))
		{

			itAskStime = true;
			timein = 0;
		} // start the time display when paused

		if (timerLcdOut == 1)
			itLcdOut = 1; // ask to go to sleep

		timer1s = 0;
		// Other slow timers
		timerScreen++;
	}
}

////////////////////////////////////////
// StNum
void setStationNum(uint8_t num)
{
	StNum = num;
}

uint8_t getStationNum()
{
	return StNum;
}

////////////////////////////////////////
// scroll each line
void scroll()
{
	scrollUcg();
}

////////////////////////////
// Change the current screen
////////////////////////////
void Screen(type_screen_e st)
{
	// printf("Screen: st: %d, stateScreen: %d, mTscreen: %d, default: %d\n", st, stateScreen, mTscreen, defaultStateScreen);
	if (stateScreen != st)
	{
		mTscreen = MTNEW;
		wakeLcd();
	}
	else
	{
		if (mTscreen == MTNODISPLAY)
			mTscreen = MTREFRESH;
	}

	//  printf("Screenout: st: %d, stateScreen: %d, mTscreen: %d, default: %d, timerScreen: %d \n",st,stateScreen,mTscreen,defaultStateScreen,timerScreen);

	stateScreen = st;
	timein = 0;
	timerScreen = 0;
	drawScreen();
	//printf("Screendis: st: %d, stateScreen: %d, mTscreen: %d, default: %d\n",st,stateScreen,mTscreen,defaultStateScreen);
	//  vTaskDelay(1);
}

////////////////////////////////////////
// draw all lines
void drawFrame()
{
	drawFrameUcg(mTscreen);
}

//////////////////////////
void drawTTitle(char *ttitle)
{
	drawTTitleUcg(ttitle);
}

////////////////////
// draw the number entered from IR
void drawNumber()
{
	if (strlen(StNumStr) > 0)
		drawNumberUcg(mTscreen, StNumStr);
}

////////////////////
// draw the station screen
void drawStation()
{
	char sNum[7];
	char *ddot;
	station_slot_s *si;

	do
	{
		si = getStation(StNum - 1);
		sprintf(sNum, "%u", StNum);
		ddot = si->name;
		char *ptl = ddot;
		while (*ptl == 0x20)
		{
			ddot++;
			ptl++;
		}
		if (strlen(ddot) == 0) // don't start an undefined station
		{
			playable = false;
			free(si);
			if (currentValue < 0)
			{
				StNum--;
				if (StNum < 1)
					StNum = MainConfig->TotalStations;
			}
			else
			{
				StNum++;
				if (StNum > (MainConfig->TotalStations))
					StNum = 1;
			}
		}
		else
			playable = true;
	} while (playable == false);

	//drawTTitle(ststr);
	//printf ("drawStation: %s\n",sNum  );
	drawStationUcg(mTscreen, sNum, ddot);
	free(si);
}

////////////////////
// draw the volume screen
void drawVolume()
{
	//  printf("drawVolume. mTscreen: %d, Volume: %d\n",mTscreen,volume);
	drawVolumeUcg(mTscreen);
}

void drawTime()
{
	drawTimeUcg(mTscreen);
}

////////////////////
// Display a screen on the lcd
void drawScreen()
{
	//  ESP_LOGW(TAG,"stateScreen: %d, mTscreen: %d",stateScreen,mTscreen);
	if ((mTscreen != MTNODISPLAY) && (!itLcdOut))
	{
		switch (stateScreen)
		{
		case MainScreen: //
			drawFrame();
			break;
		case VolumeScreen:
			drawVolume();
			break;
		case StationScreen:
			drawStation();
			break;
		case TimeScreen:
			drawTime();
			break;
		case NumberScreen:
			drawNumber();
			break;
		default:
			Screen(defaultStateScreen);
			//	  drawFrame();
		}
		//	if (mTscreen == MTREFRESH)
		mTscreen = MTNODISPLAY;
	}
}

void stopStation()
{
	clientDisconnect("addon stop");
}
void startStation()
{
	playStationInt(StNum - 1);
}
void startStop()
{
	ESP_LOGD(TAG, "START/STOP State: %d", state);
	state ? stopStation() : startStation();
}
void stationOk()
{
	beep(10);
	ESP_LOGD(TAG, "STATION OK");
	if ((strlen(StNumStr) > 0) && (atoi(StNumStr) > 0))
	{
		StNum = atoi(StNumStr);
		playStationInt(StNum - 1);
	}
	else
	{
		startStop();
	}
	StNumStr[0] = 0;
}
void changeStation(int16_t value)
{
	currentValue = value;
	// ESP_LOGD(TAG, "changeStation val: %d, StNum: %d", value, StNum);
	if (value > 0)
	{
		StNum++;
		if (StNum > MainConfig->TotalStations)
			StNum = 1;
	}
	else if (value < 0)
	{
		StNum--;
		if (StNum < 1)
			StNum = MainConfig->TotalStations;
	}
	// ESP_LOGD(TAG, "StNum: %d", StNum);
	//else if (value != 0) mTscreen = MTREFRESH;
}
// IR
// a number of station in progress...
void nbStation(char nb)
{
	beep(10);
	if (strlen(StNumStr) >= 3)
		StNumStr[0] = 0;
	uint8_t id = strlen(StNumStr);
	StNumStr[id] = nb;
	StNumStr[id + 1] = 0;
	evtScreen(NumberScreen);
}

//
static void evtClearScreen()
{
	//	ucg_ClearScreen(&ucg);
	event_lcd_s evt;
	evt.lcmd = eclrs;
	evt.lline = NULL;
	//	xQueueSendToFront(event_lcd,&evt, 0);
	xQueueSend(event_lcd, &evt, 0);
}

static void evtScreen(lcd_cmd_e value)
{
	event_lcd_s evt;
	evt.lcmd = escreen;
	evt.lline = (char *)((uint32_t)value);
	xQueueSend(event_lcd, &evt, 0);
}

static void evtStation(int16_t value)
{ // value +1 or -1
	beep(10);
	event_lcd_s evt;
	evt.lcmd = estation;
	evt.lline = (char *)((uint32_t)value);
	xQueueSend(event_lcd, &evt, 0);
}

// toggle main / time
static void toggletime()
{
	beep(10);
	event_lcd_s evt;
	evt.lcmd = etoggle;
	evt.lline = NULL;
	xQueueSend(event_lcd, &evt, 0);
}

//-----------------------
// Compute the encoder
//----------------------

void encoderCompute(encoder_s *enc, bool role)
{
	int16_t newValue = -getValue(enc);
	if (newValue != 0)
		ESP_LOGD(TAG, "encoder value: %d, stateScreen: %d", newValue, stateScreen);
	button_e newButton = getButton(enc);
	type_screen_e estate;
	if (role)
		estate = StationScreen;
	else
		estate = VolumeScreen;
	// if an event on encoder switch
	if (newButton != Open)
	{
		if (newButton == Clicked)
		{
			startStop();
		}
		// double click = toggle time
		if (newButton == DoubleClicked)
		{
			toggletime();
		}
		// switch held and rotated then change station
		if ((newButton == Held) && (getPinState(enc) == getpinsActive(enc)))
		{
			if (stateScreen != (role ? StationScreen : VolumeScreen))
				role ? evtStation(newValue) : setRelVolume(newValue);
		}
	}
	else
	// no event on button switch
	{
		if ((stateScreen != estate) && (newValue != 0))
		{
			if (role)
				setRelVolume(newValue);
			else
				evtStation(newValue);
		}
		if ((stateScreen == estate) && (newValue != 0))
		{
			if (role)
				evtStation(newValue);
			else
				setRelVolume(newValue);
		}
	}
}

void encoderLoop()
{
	// encoder = volume control or station when pushed
	if (isEncoder)
		encoderCompute(encoder, VCTRL);
}

void set_ir_training(bool training)
{
	ir_training = training;
}

uint32_t get_ir_code()
{
	return ircode;
}
//-----------------------
// Compute the ir code
//----------------------

void irLoop()
{
	// IR
	event_ir_s evt;
	while (xQueueReceive(event_ir, &evt, 0))
	{
		wakeLcd();
		uint32_t evtir = ((evt.addr) << 8) | (evt.cmd & 0xFF);
		if (!ir_training)
		{ //
			int i;
			for (i = KEY_UP; i < KEY_MAX; i++)
			{
				if ((evtir == IR_Key[i][0]) || (evtir == IR_Key[i][1]))
					break;
			}
			if (i < KEY_MAX)
			{
				switch (i)
				{
				case KEY_UP:
					evtStation(+1);
					break;
				case KEY_LEFT:
					setRelVolume(-5);
					break;
				case KEY_OK:
					if (!evt.repeat_flag)
						stationOk();
					// stopStation();
					break;
				case KEY_RIGHT:
					setRelVolume(+5);
					break;
				case KEY_DOWN:
					evtStation(-1);
					break;
				case KEY_0:
					if (!evt.repeat_flag)
						nbStation('0');
					break;
				case KEY_1:
					if (!evt.repeat_flag)
						nbStation('1');
					break;
				case KEY_2:
					if (!evt.repeat_flag)
						nbStation('2');
					break;
				case KEY_3:
					if (!evt.repeat_flag)
						nbStation('3');
					break;
				case KEY_4:
					if (!evt.repeat_flag)
						nbStation('4');
					break;
				case KEY_5:
					if (!evt.repeat_flag)
						nbStation('5');
					break;
				case KEY_6:
					if (!evt.repeat_flag)
						nbStation('6');
					break;
				case KEY_7:
					if (!evt.repeat_flag)
						nbStation('7');
					break;
				case KEY_8:
					if (!evt.repeat_flag)
						nbStation('8');
					break;
				case KEY_9:
					if (!evt.repeat_flag)
						nbStation('9');
					break;
				case KEY_STAR:
					if (!evt.repeat_flag)
						playStationInt(StNum - 1);
					break;
				case KEY_DIESE:
					if (!evt.repeat_flag)
						// stopStation();
						toggletime();
					break;
				default:;
				}
				ESP_LOGV(TAG, "irCode success, evtir %x, i: %d", evtir, i);
			}
		}
		else
		{
			beep(30);
			ESP_LOGI(TAG, "IR training: Channel: %x, ADDR: %x, CMD: %x = %X, REPEAT: %d", evt.channel, evt.addr, evt.cmd, evtir, evt.repeat_flag);
			ircode = evtir;
			char IRcode[20];
			sprintf(IRcode, "{\"ircode\":\"%X\"}", ircode);
			websocketbroadcast(IRcode, strlen(IRcode));
		}
		ircode = 0;
	}
}

void initButtonDevices()
{
	//	radiola_config_s *device;
	gpio_num_t enca0;
	gpio_num_t encb0;
	gpio_num_t encbtn0;
	gpio_get_encoders(&enca0, &encb0, &encbtn0);
	if (enca0 == GPIO_NONE)
		isEncoder = false; //no encoder
	if (isEncoder)
		encoder = ClickEncoderInit(enca0, encb0, encbtn0, ((MainConfig->options & Y_ENC) == 0) ? false : true);
}

// custom ir code init from hardware nvs partition
esp_err_t ir_key_init()
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
	if (MainConfig->ir_mode == IR_DEFAULD)
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
		if (err != ESP_OK)
		{
			close_storage(handle);
			MainConfig->ir_mode = IR_DEFAULD;
			SaveConfig();
			return err;
		}
		for (indexKey = KEY_UP; indexKey < KEY_MAX; indexKey++)
		{
			// get the key in the nvs
			err |= nvs_get_ir_key(handle, klab[indexKey], (uint32_t *)&(IR_Key[indexKey][0]), (uint32_t *)&(IR_Key[indexKey][1]));
			taskYIELD();
		}
		close_storage(handle);
	}
	return err;
}

// touch loop

void touchLoop()
{
	int tx, ty;
	if (haveTouch())
	{
		if (xpt_read_touch(&tx, &ty, 0))
		{
			ESP_LOGD(TAG, "tx: %d, ty: %d", tx, ty);
			uint16_t width = GetWidth();
			uint16_t height = GetHeight();
			uint16_t xdiv2 = width / 2;
			uint16_t xdiv6 = width / 6;
			uint16_t ydiv2 = height / 2;
			uint16_t ydiv6 = height / 6;
			uint16_t xl = xdiv2 - xdiv6;
			uint16_t xh = xdiv2 + xdiv6;
			uint16_t yl = ydiv2 - ydiv6;
			uint16_t yh = ydiv2 + ydiv6;
			if ((ty > yl) && (ty < yh))
			{
				if ((tx > xl) && (tx < xh))
					startStop(); // center
				else
				{
					if (tx < xl)
						evtStation(-1); //
					else
						evtStation(+1); // evtStation(1);
				}
			}
			else if (ty < yl)
				setRelVolume(+5);
			else
				setRelVolume(-5);
		}
	}
}

// indirect call to service
IRAM_ATTR void multiService() // every 1ms
{
	if (isEncoder)
		service(encoder);
	ServiceAddon();
}
void adcLoop()
{
	if (ldr_channel != GPIO_NONE) //пин фоторезистора определён
	{

		if (MainConfig->backlight_mode == NOT_ADJUSTABLE)
		{
			MainConfig->backlight_level = 255;
		}
		else if (MainConfig->backlight_mode == BY_TIME)
		{
			// Уровень подсветки в зависимости от времени суток
		}
		else if (MainConfig->backlight_mode == BY_LIGHTING)
		{
			MainConfig->night_brightness = 55; // Временные установки
			MainConfig->day_brightness = 255;  // Временные установки
			adc1_config_width(ADC_WIDTH_9Bit);
			adc1_config_channel_atten(ldr_channel, ADC_ATTEN_11db);
			uint32_t voltage = 0;
			// Снимаем напряжение.
			voltage = (adc1_get_raw(ldr_channel));
			// vTaskDelay(1);
			MainConfig->backlight_level = map(voltage, 0, 511, MainConfig->day_brightness, MainConfig->night_brightness);
			// ESP_LOGD(TAG, "LDR voltage: %d\n", voltage);
			// ESP_LOGD(TAG, "backlight_level: %d\n", MainConfig->backlight_level);
		}
		else if (MainConfig->backlight_mode == BY_HAND)
		{
			// Ручная регулировка уровня подсветки
		}
		if (MainConfig->backlight_level != old_backlight_level) // Если уровень изменился, применяем к дисплею
		{
			SetLedBacklight();
			old_backlight_level = MainConfig->backlight_level; // Запоминаем текущий уровень подсветки
		}
	}
	if (kbd_channel == GPIO_NONE)
		return; // no gpio specified
	uint32_t voltage, voltage0, voltage1;
	bool wasVol = false;
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(kbd_channel, ADC_ATTEN_0db);
	voltage0 = (adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel)) / 4;
	vTaskDelay(1);
	voltage1 = (adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel) + adc1_get_raw(kbd_channel)) / 4;
	//	printf ("Volt0: %d, Volt1: %d\n",voltage0,voltage1);
	voltage = (voltage0 + voltage1) * 105 / (819);
	if (voltage < 100)
		return; // клавиатура не подключена...

	if (inside && (voltage0 > 3700))
	{
		inside = false;
		wasVol = false;
		return;
	}
	if (voltage0 > 3700)
	{
		wasVol = false;
	}
	if ((voltage0 > 3700) || (voltage1 > 3700))
		return; // must be two valid voltage

	if (voltage < 985)
		ESP_LOGD(TAG, "Voltage: %i", voltage);
	//		printf("VOLTAGE: %d\n",voltage);
	if ((voltage > 400) && (voltage < 590)) // volume +
	{
		setRelVolume(+5);
		wasVol = true;
		ESP_LOGD(TAG, "Volume+ : %i", voltage);
	}
	else if ((voltage > 730) && (voltage < 830)) // volume -
	{
		setRelVolume(-5);
		wasVol = true;
		ESP_LOGD(TAG, "Volume- : %i", voltage);
	}
	else if ((voltage > 900) && (voltage < 985)) // station+
	{
		if (!wasVol)
		{
			evtStation(1);
			ESP_LOGD(TAG, "station+: %i", voltage);
		}
	}
	else if ((voltage > 620) && (voltage < 710)) // station-
	{
		if (!wasVol)
		{
			evtStation(-1);
			ESP_LOGD(TAG, "station-: %i", voltage);
		}
	}
	if (!inside)
	{
		if ((voltage > 100) && (voltage < 220)) // toggle time/info  old stop
		{
			inside = true;
			toggletime();
			ESP_LOGD(TAG, "toggle time: %i", voltage);
		}
		else if ((voltage > 278) && (voltage < 380)) //start stop toggle   old start
		{
			inside = true;
			startStop();
			ESP_LOGD(TAG, "start stop: %i", voltage);
		}
	}
}
//--------------------
// LCD display task
//--------------------

void task_lcd(void *pvParams)
{
	event_lcd_s evt;  // lcd event
	event_lcd_s evt1; // lcd event
	ESP_LOGD(TAG, "task_lcd Started, LCD Type");
	defaultStateScreen = (MainConfig->options & Y_TOGGLETIME) ? TimeScreen : MainScreen;
	drawFrame();

	while (1)
	{
		if (itLcdOut == 1) // switch off the lcd
		{
			sleepLcd();
		}

		if (stateScreen == MainScreen)
		{
			scroll();
		}
		if ((stateScreen == TimeScreen) || (stateScreen == MainScreen))
		{
			mTscreen = MTREFRESH;
		} // display time

		drawScreen();
		if (event_lcd != NULL)
			while (xQueueReceive(event_lcd, &evt, 0))
			{
				if (evt.lcmd != lmeta)
					ESP_LOGV(TAG, "event_lcd: %x", (int)evt.lcmd);
				else
					ESP_LOGV(TAG, "event_lcd: %x  %s", (int)evt.lcmd, evt.lline);
				switch (evt.lcmd)
				{
				case lmeta:
					metaUcg(evt.lline);
					Screen(MainScreen);
					wakeLcd();
					break;
				case licy4:
					icy4Ucg(evt.lline);
					break;
				case licy0:
					icy0Ucg(evt.lline);
					break;
				case lstop:
					statusUcg(stopped);
					Screen(MainScreen);
					wakeLcd();
					break;
				case lnameset:
					namesetUcg(evt.lline);
					statusUcg(starting);
					Screen(MainScreen);
					wakeLcd();
					break;
				case lplay:
					playingUcg();
					break;
				case lvol:
					// ignore it if the next is a lvol
					if (xQueuePeek(event_lcd, &evt1, 0))
						if (evt1.lcmd == lvol)
							break;
					setVolumeUcg(volume);
					if (dvolume)
					{
						Screen(VolumeScreen);
						wakeLcd();
					}
					dvolume = true;
					break;
				case estation:
					wakeLcd();
					if (xQueuePeek(event_lcd, &evt1, 0))
						if (evt1.lcmd == estation)
						{
							evt.lline = NULL;
							break;
						}
					ESP_LOGD(TAG, "estation val: %d", (uint32_t)evt.lline);
					changeStation((uint32_t)evt.lline);
					Screen(StationScreen);
					evt.lline = NULL; // just a number
					break;
				case eclrs:
					ucg_ClearScreen(&ucg);
					break;
				case escreen:
					Screen((uint32_t)evt.lline);
					wakeLcd();
					evt.lline = NULL; // just a number Don't free
					break;
				case etoggle:
					defaultStateScreen = (stateScreen == MainScreen) ? TimeScreen : MainScreen;
					(stateScreen == MainScreen) ? Screen(TimeScreen) : Screen(MainScreen);
					MainConfig->options = (defaultStateScreen == MainScreen) ? MainConfig->options & N_TOGGLETIME : MainConfig->options | Y_TOGGLETIME;
					SaveConfig();
					break;
				default:;
				}
				if (evt.lline != NULL)
					free(evt.lline);
				vTaskDelay(1);
			}
		if ((event_lcd) && (!uxQueueMessagesWaiting(event_lcd)))
			vTaskDelay(4);
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}

//-------------------
// Main task of addon
//-------------------
extern void rmt_nec_rx_task();

void task_addon(void *pvParams)
{
	xTaskHandle pxCreatedTask;
	// Инициализация фоторезистора
	gpio_num_t ldr;
	gpio_get_ldr(&ldr);
	if (ldr != GPIO_NONE)
	{
		// вычисление канала по используемому пину
		switch (ldr)
		{
		case GPIO_NUM_36: //
			ldr_channel = ADC1_CHANNEL_0;
			break;
		case GPIO_NUM_37:
			ldr_channel = ADC1_CHANNEL_1;
			break;
		case GPIO_NUM_38:
			ldr_channel = ADC1_CHANNEL_2;
			break;
		case GPIO_NUM_39:
			ldr_channel = ADC1_CHANNEL_3;
			break;
		case GPIO_NUM_32:
			ldr_channel = ADC1_CHANNEL_4;
			break;
		case GPIO_NUM_33:
			ldr_channel = ADC1_CHANNEL_5;
			break;
		case GPIO_NUM_34:
			ESP_LOGE(TAG, "Channel reserved for keyboard!");
			break;
		case GPIO_NUM_35:
			ldr_channel = ADC1_CHANNEL_7;
			break;
		default:
			ESP_LOGD(TAG, "LDR Channel not defined");
		}
		if (ldr_channel != GPIO_NONE)
		{
			ESP_LOGD(TAG, "Channel for GPIO: %d defined, number: %i", ldr, ldr_channel);
			MainConfig->ldr = true;
			// adc1_config_width(ADC_WIDTH_9Bit);
			// adc1_config_channel_atten(ldr_channel, ADC_ATTEN_11db);
		}
		else
		{
			MainConfig->ldr = false;
		}
		SaveConfig();
	}
	if (PIN_NUM_KBD == GPIO_NUM_34) // Инициализация клавиатуры
	{
		kbd_channel = ADC1_CHANNEL_6;
		ESP_LOGD(TAG, "Channel for KBD defined, number: %i", kbd_channel);
		// adc1_config_width(ADC_WIDTH_12Bit);
		// adc1_config_channel_atten(kbd_channel, ADC_ATTEN_0db);
	}
	else
	{
		kbd_channel = GPIO_NONE;
		ESP_LOGD(TAG, "Channel for KBD not defined");
	}
	ir_key_init();
	initButtonDevices();

	serviceAddon = &multiService;
	; // connect the 1ms interruption
	StNum = getCurrentStation();
	StNum++;
	sprintf(StNumStr, "%u", StNum);
	//ir
	// queue for events of the IR nec rx
	event_ir = xQueueCreate(5, sizeof(event_ir_s));
	ESP_LOGD(TAG, "event_ir: %x", (int)event_ir);
	// queue for events of the lcd
	event_lcd = xQueueCreate(20, sizeof(event_lcd_s));
	ESP_LOGD(TAG, "event_lcd: %x", (int)event_lcd);

	xTaskCreatePinnedToCore(rmt_nec_rx_task, "rmt_nec_rx_task", 2148, NULL, PRIO_RMT, &pxCreatedTask, CPU_RMT);
	ESP_LOGI(TAG, "%s task: %x", "rmt_nec_rx_task", (unsigned int)pxCreatedTask);
	;
	xTaskCreatePinnedToCore(task_lcd, "task_lcd", 2200, NULL, PRIO_LCD, &pxTaskLcd, CPU_LCD);
	ESP_LOGI(TAG, "%s task: %x", "task_lcd", (unsigned int)pxTaskLcd);
	getTaskLcd(&pxTaskLcd); // give the handle to xpt
							//	vTaskDelay(1);
							//	wakeLcd();

	while (1)
	{
		adcLoop();			  // Опрос фоторезистора и клавиатуры
		encoderLoop();		  // Опрос энкодера
		irLoop();			  // Опрос ИК-пульта
		touchLoop();		  // Опрос тачскрина
		if (timerScreen >= 3) //  sec timeout transient screen
		{
			//			if ((stateScreen != MainScreen)&&(stateScreen != TimeScreen)&&(stateScreen != EmptyScreen))
			//printf("timerScreen: %d, stateScreen: %d, defaultStateScreen: %d\n",timerScreen,stateScreen,defaultStateScreen);
			timerScreen = 0;
			if ((stateScreen != defaultStateScreen) && (stateScreen != EmptyScreen))
			{
				// Play the changed station on return to main screen
				// if a number is entered, play it.
				if (strlen(StNumStr) > 0)
				{
					uint8_t old_num = StNum;
					StNum = atoi(StNumStr);
					if (StNum < 1)
					{
						StNum = old_num;
						playable = false;
					}
					else if (StNum > MainConfig->TotalStations)
					{
						StNum = old_num;
						playable = false;
					}
					else
					{
						playable = true;
					}
					// clear the number
					StNumStr[0] = 0;
				}
				if ((strlen(getNameNumUcg()) != 0) && playable && (StNum != atoi(getNameNumUcg())))
				{
					playStationInt(StNum - 1);
					vTaskDelay(10);
				}
				if (!itAskStime)
				{
					if ((defaultStateScreen == TimeScreen) && (stateScreen != MainScreen))
						evtScreen(MainScreen);
					else if ((defaultStateScreen == TimeScreen) && (stateScreen == MainScreen))
						evtScreen(TimeScreen);
					else if (stateScreen != defaultStateScreen)
						evtScreen(defaultStateScreen); //Back to the old screen
				}
			}
			if (itAskStime && (stateScreen != TimeScreen)) // time start the time display. Don't do that in interrupt.
				evtScreen(TimeScreen);
		}

		vTaskDelay(20);
	}
	vTaskDelete(NULL);
}

////////////////////////////////////////
// parse the esp32media received line and do the job
void addonParse(const char *fmt, ...)
{
	event_lcd_s evt;
	char *line = NULL;
	//	char* lfmt;
	int rlen;
	line = (char *)malloc(1024);
	if (line == NULL)
	{
		free(line);
		return;
	}
	line[0] = 0;
	strcpy(line, "ok\n");

	va_list ap;
	va_start(ap, fmt);
	rlen = vsprintf(line, fmt, ap);
	va_end(ap);
	line = realloc(line, rlen + 1);
	if (line == NULL)
	{
		free(line);
		return;
	}
	ESP_LOGV(TAG, "LINE: %s", line);
	evt.lcmd = -1;
	char *ici = strstr(line, "META#: ");
	if (ici != NULL)
	{ ////// Meta title  ##CLI.META#:
		evt.lcmd = lmeta;
		evt.lline = malloc(strlen(ici) + 1);
		//evt.lline = NULL;
		strcpy(evt.lline, ici);
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "ICY4#: ");
	if (ici != NULL)
	{ ////// ICY4 Description  ##CLI.ICY4#:
		evt.lcmd = licy4;
		evt.lline = malloc(strlen(ici) + 1);
		//evt.lline = NULL;
		strcpy(evt.lline, ici);
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "ICY0#: ");
	if (ici != NULL)
	{ ////// ICY0 station name   ##CLI.ICY0#:
		evt.lcmd = licy0;
		evt.lline = malloc(strlen(ici) + 1);
		//evt.lline = NULL;
		strcpy(evt.lline, ici);
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "STOPPED");
	if ((ici != NULL) && (strstr(line, "C_HDER") == NULL) && (strstr(line, "C_PLIST") == NULL))
	{ ////// STOPPED  ##CLI.STOPPED#
		state = false;
		evt.lcmd = lstop;
		evt.lline = NULL;
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "MESET#: ");
	if (ici != NULL)
	{ //////Nameset    ##CLI.NAMESET#:
		evt.lcmd = lnameset;
		evt.lline = malloc(strlen(ici) + 1);
		//evt.lline = NULL;
		strcpy(evt.lline, ici);
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "YING#");
	if (ici != NULL)
	{ //////Playing    ##CLI.PLAYING#
		state = true;
		itAskStime = false;
		evt.lcmd = lplay;
		evt.lline = NULL;
		//xQueueSend(event_lcd,&evt, 0);
	}
	ici = strstr(line, "VOL#:");
	if (ici != NULL)
	{						   //////Volume   ##CLI.VOL#:
		if (*(ici + 6) != 'x') // ignore help display.
		{
			volume = atoi(ici + 6);
			evt.lcmd = lvol;
			evt.lline = NULL; //atoi(ici+6);
							  //xQueueSend(event_lcd,&evt, 0);
		}
	}
	if (evt.lcmd != -1)
		xQueueSend(event_lcd, &evt, 0);
	free(line);
}
