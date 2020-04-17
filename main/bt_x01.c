#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "bt_x01.h"
#include "gpios.h"
#include "eeprom.h"
#include "interface.h"

#define TAG "BT201"
////////////////////////////////

#define PATTERN_CHR_NUM (1) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t bt_queue;
uint8_t *bt_data;
int rxBytes = 0;
char event_bt[BUF_SIZE] = {""};

struct bt_x01 bt;

uint8_t check_index(const char *find)
{
    // Поиск индекса начала параметра в строке
    uint8_t _index = 0;

    for (uint8_t i = 0; i < rxBytes; i++)
    {

        if (strncmp((char *)bt_data + i, find, strlen(find)) == 0)
        {
            // ESP_LOGD(TAG, "index found %d", i);
            _index = i;
            break;
        }
    }
    return _index;
}

void unicode_to_utf8(uint8_t event_index)
{
    if (event_index > 0)
    {
        uint8_t step = 1;
        uint8_t count = 0;

        if (rxBytes > 2)
        {
            event_index = event_index + 3;
            if (bt_data[event_index + 1] == 0 || bt_data[event_index + 1] == 4)
                step = 2;
        }

        for (int i = event_index; i < rxBytes; i = i + step)
        {
            if (bt_data[i + 1] == 4)
            {
                if (bt_data[i] == 0x51)
                {
                    event_bt[count] = (char)0xD1;
                    count++;
                    event_bt[count] = (char)0x91;
                    count++;
                }
                else
                {
                    if (bt_data[i] >= 0x40)
                    {
                        event_bt[count] = (char)0xD1;
                        count++;
                        event_bt[count] = (char)bt_data[i] + (char)0x40;
                    }
                    else
                    {
                        event_bt[count] = (char)0xD0;
                        count++;
                        event_bt[count] = (char)bt_data[i] + (char)0x80;
                    }
                    count++;
                }
            }
            else if (bt_data[i + 1] == 0)
            {
                if (bt_data[i] > 0x1F && bt_data[i] < 0x7F)
                {
                    event_bt[count] = (char)bt_data[i];
                    count++;
                }
            }
            else
            {
                if (bt_data[i] > 0x1F && bt_data[i] < 0x7F)
                {
                    event_bt[count] = (char)bt_data[i];
                    count++;
                }
            }
        }
        event_bt[count] = '\0';
        // ESP_LOGD(TAG, "Size str: %d\n", count);
        // ESP_LOGD(TAG, "Event str: %s\n", event_bt);
    }
}

void check_event()
{
    //

    uint8_t txt_index = check_index("OK"); // Успешное выполнение команды
    if (txt_index > 0)
    {
        if (bt.err == 0)
            checkCommand(2, "OK");
        else
            checkCommand(5, "ERROR");
    }
    bt.err = 0; // Сброс ошибок.
    txt_index = check_index("AT+VER");
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // if (strlen(g_device->bt_ver) != strlen(event_bt))
        if (strlen(event_bt) > 11)
        {
            bt.present = true;
            if (strcmp(event_bt, g_device->bt_ver) != 0)
            {
                strcpy(g_device->bt_ver, event_bt);
                saveDeviceSettings(g_device);
            }
        }
        else
        {
            bt.present = false;
            strcpy(g_device->bt_ver, "NOT PRESENT");
        }
    }
    txt_index = check_index("QA+"); // Текущая громкость
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_vol = atoi(event_bt);
        kprintf("#BT: Current Volume: %u\n", bt.current_vol);
        // ESP_LOGD(TAG, "bt.current_vol: %u\n", bt.current_vol);
    }
    txt_index = check_index("QM+"); // Текущий режим
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_mode = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.current_mode: %u\n", bt.current_mode);
    }
    txt_index = check_index("QM+"); // Текущий режим
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_mode = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.current_mode: %u\n", bt.current_mode);
    }
    txt_index = check_index("QN+"); // prompt sound
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.prompt_sound = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.prompt_sound: %d\n", bt.prompt_sound);
    }
    txt_index = check_index("QK+"); // automatically switch to Bluetooth
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.auto_bluetooth = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.auto_bluetooth: %d\n", bt.auto_bluetooth);
    }
    txt_index = check_index("QG+"); // Bluetooth Background
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.bluetooth_background = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.bluetooth_background: %d\n", bt.bluetooth_background);
    }
    txt_index = check_index("Q1+"); // AD button
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.ad_button = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.ad_button: %u\n", bt.ad_button);
    }
    txt_index = check_index("RB+"); //  Recording Bit Rate
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        uint8_t bit_rate = atoi(event_bt);
        switch (bit_rate)
        {
        case 0:
            bt.rec_bit_rate = 16;
            break;
        case 1:
            bt.rec_bit_rate = 24;
            break;
        case 2:
            bt.rec_bit_rate = 32;
            break;
        case 3:
            bt.rec_bit_rate = 48;
            break;
        case 4:
            bt.rec_bit_rate = 64;
            break;
        case 5:
            bt.rec_bit_rate = 96;
            break;
        case 6:
            bt.rec_bit_rate = 128;
            break;
        case 7:
            bt.rec_bit_rate = 144;
            break;
        case 8:
            bt.rec_bit_rate = 160;
            break;

        default:
            bt.rec_bit_rate = 0;
            break;
        }
        // ESP_LOGD(TAG, "bt.rec_bit_rate: %u\n", bt.rec_bit_rate);
    }
    txt_index = check_index("RV+"); //  gain of setting MIC
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.gain_mic = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.gain_mic: %u\n", bt.gain_mic);
    }
    txt_index = check_index("TS+"); //   status indicates that Bluetooth
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.status_bluetooth = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.status_bluetooth: %u\n", bt.status_bluetooth);
    }
    txt_index = check_index("TL+"); //    BLE Simple State
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.ble_state = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.ble_state: %u\n", bt.ble_state);
    }
    txt_index = check_index("T1+"); //    Represents the default need to enter a password of "0000"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.need_password = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.need_password: %d\n", bt.need_password);
    }
    txt_index = check_index("T2+"); //    Representative Chip Supports HFP
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_hfp = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.support_hfp: %d\n", bt.support_hfp);
    }
    txt_index = check_index("T3+"); //    Delegate chip support A2DP
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_a2dp = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.support_a2dp: %d\n", bt.support_a2dp);
    }
    txt_index = check_index("T4+"); //    Support for BLE on behalf of chip
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_ble = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.support_ble: %d\n", bt.support_ble);
    }
    txt_index = check_index("T5+"); //    Delegate chip supports EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_edr = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.support_edr: %d\n", bt.support_edr);
    }
    txt_index = check_index("TE+"); //    Return the password of the current Bluetooth connection to "0000"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.current_password, event_bt);
        // ESP_LOGD(TAG, "bt.current_password: %s\n", bt.current_password);
    }
    txt_index = check_index("TD+"); //    Returns the name of the current Bluetooth EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.edr_name, event_bt);
        // ESP_LOGD(TAG, "bt.edr_name: %s\n", bt.edr_name);
    }
    txt_index = check_index("TA+"); //    Returns the MAC address of the current Bluetooth EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.mac_edr, event_bt);
        // ESP_LOGD(TAG, "bt.mac_edr: %s\n", bt.mac_edr);
    }
    txt_index = check_index("TB+"); //    Returns the MAC address of the current Bluetooth BLE
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.mac_ble, event_bt);
        // ESP_LOGD(TAG, "bt.mac_ble: %s\n", bt.mac_ble);
    }
    txt_index = check_index("TM+"); //    Returns the name of the current Bluetooth BLE
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.ble_name, event_bt);
        // ESP_LOGD(TAG, "bt.ble_name: %s\n", bt.ble_name);
    }
    txt_index = check_index("T6+"); //    Specify service UUID as F000
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid0, event_bt);
        // ESP_LOGD(TAG, "bt.uuid0: %s\n", bt.uuid0);
    }
    txt_index = check_index("T7+"); //    Specify signature 1 as F001, and its feature is "write"+ "listen"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid1, event_bt);
        // ESP_LOGD(TAG, "bt.uuid1: %s\n", bt.uuid1);
    }
    txt_index = check_index("T8+"); //    Specify signature 2 as F002, which is read + listen
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid2, event_bt);
        // ESP_LOGD(TAG, "bt.uuid2: %s\n", bt.uuid2);
    }
    txt_index = check_index("T9+"); //    Specify signature 3 as F003 and its feature is "write"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid3, event_bt);
        // ESP_LOGD(TAG, "bt.uuid3: %s\n", bt.uuid3);
    }
    txt_index = check_index("TR+"); //    Query the broadcast package data of the chip, and the chip will return "TR+9988776655"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.boardcast, event_bt);
        // ESP_LOGD(TAG, "bt.boardcast: %s\n", bt.boardcast);
    }
    txt_index = check_index("M1+"); //    Play file physical number of current device
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.play_file_number = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.play_file_number: %u\n", bt.play_file_number);
    }
    txt_index = check_index("M2+"); //    Total number of files for current devices
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.total_files = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.total_files: %u\n", bt.total_files);
    }
    txt_index = check_index("MT+"); //    Total time for current file playback
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.total_time = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.total_time: %u\n", bt.total_time);
    }
    txt_index = check_index("MK+"); //    Time when the current file has been played
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.current_time = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.current_time: %u\n", bt.current_time);
    }
    txt_index = check_index("MF+"); //    Long File Name of Currently Played Files
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        // bt.current_time = atoi(event_bt);
        if (strlen(event_bt))
        {
            // ESP_LOGD(TAG, "Current Played File Name: %s\n", event_bt);
            kprintf("#BT: Played File Name:\n\"%s\"\n", event_bt);
        }
    }
    txt_index = check_index("MU+"); //     TFCard or U Disk - PC Sound Card - Insert and Pull Back Information
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.isert_info = atoi(event_bt);
        // ESP_LOGD(TAG, "bt.isert_info: %u\n", bt.isert_info);
        switch (bt.isert_info)
        {
        case 1:
            kprintf("#BT: USB disk insertion.\n");
            break;
        case 2:
            kprintf("#BT: USB disk pull-out.\n");
            break;
        case 3:
            kprintf("#BT: TF card insertion.\n");
            break;
        case 4:
            kprintf("#BT: TF card pull-out.\n");
            break;
        case 5:
            kprintf("#BT: Connecting PC.\n");
            break;
        case 6:
            kprintf("#BT: PC pull-out.\n");
            break;
        default:
            break;
        }
    }
    txt_index = check_index("ER+"); // Ошибки
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.err = atoi(event_bt);
        // ESP_LOGD(TAG, "ERROR: %u\n", bt.err);
        kprintf("#BT ERROR #%u :", bt.err);
        switch (bt.err)
        {
        case 1:
            // Полученный фрейм данных не корректен
            kprintf("The received data frame is not correct.\n");
            break;
        case 2:
            // Полученная команда не существует
            kprintf("The received command does not exist.\n");
            break;
        case 3:
            // При записи устройство не подключено или другие ошибки
            kprintf("When recording, the device is not online or other errors.\n");
            break;
        case 4:
            // Отправленные параметры выходят за пределы допустимого диапазона, либо формат команды неверен.
            kprintf("Sent parameters are out of range or the command format is incorrect.\n");
            break;
        case 5:
            // Указанное устройство [TF или U диск] не подключено или работает неправильно.
            kprintf("The specified device [TF or U disk] is not connected or is not working properly.\n");
            break;
        case 6:
            // Ошибка в указанном пути к устройству [диск TF или U].
            kprintf("Error in the specified device path [TF or U disk].\n");
            break;
        default:
            // Не определено.
            kprintf("Undetermined.\n");
            break;
        }
    }
}
void sendData(const char *at_data)
{
    char buf[strlen(at_data) + 3];
    sprintf(buf, "%s\r\n", at_data);
    const int len = strlen(buf);
    const int txBytes = uart_write_bytes(UART_NUM_1, buf, len);
    if (txBytes != -1)
    {
        // ESP_LOGD(TAG, "Wrote %d bytes", txBytes);
    }
    else
        ESP_LOGE(TAG, "Write to uart fail!!");
}

static void uartBtTask()
{
    uart_event_t event;
    size_t buffered_size;
    bt_data = (uint8_t *)malloc(RD_BUF_SIZE + 1);
    while (1)
    {
        //Waiting for UART event.
        if (xQueueReceive(bt_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            bzero(bt_data, RD_BUF_SIZE);
            // ESP_LOGI(TAG, "uart[%d] event:", UART_NUM_1);
            switch (event.type)
            {
            case UART_DATA:
                // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                // uart_read_bytes(UART_NUM_1, bt_data, event.size, portMAX_DELAY);
                // ESP_LOGI(TAG, "[DATA EVT]:");
                // uart_write_bytes(UART_NUM_1, (const char *)bt_data, event.size);
                break;
            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "hw fifo overflow");
                uart_flush_input(UART_NUM_1);
                xQueueReset(bt_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGE(TAG, "ring buffer full");
                uart_flush_input(UART_NUM_1);
                xQueueReset(bt_queue);
                break;
            case UART_BREAK:
                // ESP_LOGE(TAG, "uart rx break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                // ESP_LOGE(TAG, "uart frame error");
                break;
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_NUM_1, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_NUM_1);
                // ESP_LOGD(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    uart_flush_input(UART_NUM_1);
                }
                else
                {
                    rxBytes = uart_read_bytes(UART_NUM_1, bt_data, pos, 100 / portTICK_PERIOD_MS);
                    if (rxBytes > 0)
                    {
                        bt_data[rxBytes] = 0;
                        int uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                        ESP_LOGD("uart1Task", "watermark: %d  heap: %d", uxHighWaterMark, xPortGetFreeHeapSize());
                        // ESP_LOGD(TAG, "read data: %s size: %d", bt_data, rxBytes);
                        // ESP_LOG_BUFFER_HEXDUMP(TAG, bt_data, pos, ESP_LOG_INFO);
                        check_event();
                    }
                }
                break;
            //Others
            default:
                // ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(bt_data);
    bt_data = NULL;
    vTaskDelete(NULL);
}

////////////////////////////////

void uartBtInit()
{
    uint32_t uspeed = g_device->uartspeed;
    gpio_num_t bt_rxd, bt_txd;
    esp_err_t err = gpio_get_uart(&bt_rxd, &bt_txd);
    if (bt_rxd != GPIO_NONE && bt_txd != GPIO_NONE)
    {
        uart_config_t uart_config1 = {
            .baud_rate = uspeed,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART_HW_FLOWCTRL_CTS_RTS,
            .rx_flow_ctrl_thresh = 0,
        };
        err |= uart_param_config(UART_NUM_1, &uart_config1);

        err |= uart_set_pin(UART_NUM_1, bt_txd, bt_rxd, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        //Install UART driver, and get the queue.
        err |= uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 20, &bt_queue, 0);

        //Set uart pattern detect function.
        err |= uart_enable_pattern_det_intr(UART_NUM_1, '\r', PATTERN_CHR_NUM, 10000, 10, 10);
        //Reset the pattern queue length to record at most 20 pattern positions.
        err |= uart_pattern_queue_reset(UART_NUM_1, 20);

        if (err == ESP_OK)
        {
            TaskHandle_t xBT_Handdle;
            xTaskCreatePinnedToCore(uartBtTask, "uartBtTask", BUF_SIZE * 3, NULL, 12, &xBT_Handdle, 1);
            // ESP_LOGD(TAG, "%s task: %x", "uartBtTask", (unsigned int)xBT_Handdle);
            vTaskDelay(1);
            sendData("AT+CZ");
            vTaskDelay(150);
            if (bt.present)
            {
                ESP_LOGI(TAG, "%s", g_device->bt_ver);
            }
            else
            {
                vTaskDelete(xBT_Handdle);
                uart_driver_delete(UART_NUM_1);
                ESP_LOGE(TAG, "%s!", g_device->bt_ver);
            }
        }
        else
        {
            ESP_LOGE(TAG, "UART1 err: %X", err);
        }
    }
}