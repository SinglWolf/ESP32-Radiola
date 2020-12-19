/*
 * Copyright 2016 karawin (http://www.karawin.fr)
 * Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
*/
#pragma once

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "websocket.h"

extern xSemaphoreHandle semclient;
extern xSemaphoreHandle semfile;

void serverclientTask(void *pvParams);
void playStationInt(uint8_t id);
void websockethandle(int socket, wsopcode_e opcode, uint8_t * payload, size_t length);
uint16_t getVolume(void);
void setVolume(char* vol);
void setVolumei(int8_t vol);
void setRelVolume(int8_t vol);

#endif