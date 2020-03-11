/******************************************************************************
 * 
 * Copyright 2017 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
*******************************************************************************/
#ifndef __have__addon_h__
#define __have__addon_h__

// Never change this file. See gpio.h to adapt to your pcb.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ucg.h"

// custom ir code init from hardware nvs
typedef enum {
	KEY_UP,
	KEY_LEFT,
	KEY_OK,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_STAR,
	KEY_DIESE,
	KEY_MAX
} ir_key_t;

typedef struct
{
    int channel;   /*!< event type */
    uint16_t addr; /*!< nec addr */
    uint16_t cmd;  /*!< nec cmdr */
    bool repeat_flag;
} event_ir_t;

typedef enum typelcmd
{
    lstop,
    lplay,
    lmeta,
    licy0,
    licy4,
    lnameset,
    lvol,
    estation,
    eclrs,
    edraw,
    etoggle,
    escreen,
    erefresh
} typelcmd;

typedef struct
{
    typelcmd lcmd; /*!< For what ?*/
    char *lline;   /*!< string of command */
} event_lcd_t;

#define MTNODISPLAY 0
#define MTNEW 1
#define MTREFRESH 2

#define VCTRL false

extern xQueueHandle event_ir;
extern ucg_t ucg;
void task_addon(void *pvParams);
void task_lcd(void *pvParams);
void lcd_init();
void (*serviceAddon)();
void addonParse(const char *fmt, ...);
void lcd_welcome(const char *ip, const char *state);
void setFuturNum(int16_t new);
int16_t getFuturNum();
void wakeLcd();
void *getEncoder();
uint32_t get_ir_code();
void set_ir_training(bool training);
#endif