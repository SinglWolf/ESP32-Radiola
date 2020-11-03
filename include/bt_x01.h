/******************************************************************************
 * 
 * Copyright ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
 *
 *******************************************************************************/
#ifndef BT_X01_H_
#define BT_X01_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "string.h"

struct bt_x01
{
    bool present;
    uint8_t current_vol;
    uint8_t current_mode;
    bool prompt_sound;
    bool auto_bluetooth;
    bool bluetooth_background;
    uint8_t ad_button;
    uint8_t rec_bit_rate;
    uint8_t gain_mic;
    uint8_t status_bluetooth;
    uint8_t ble_state;
    bool debug;
    bool need_password;
    bool support_hfp;
    bool support_a2dp;
    bool support_ble;
    bool support_edr;
    char current_password[4];
    char uuid0[4];
    char uuid1[4];
    char uuid2[4];
    char uuid3[4];
    char edr_name[30];
    char ble_name[30];
    char mac_edr[6];
    char mac_ble[6];
    char boardcast[10];
    uint16_t play_file_number;
    uint16_t total_files;
    uint16_t total_time;
    uint16_t current_time;
    uint8_t isert_info;
    uint8_t err;
};
extern struct bt_x01 bt;

void uartBtInit();
void sendData(const char *at_data);

#endif /* BT_X01_H_ */