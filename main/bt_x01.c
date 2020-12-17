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

uint8_t find_index(const char *find)
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
    }
}

void parse_event()
{
    //

    uint8_t txt_index = find_index("OK"); // Успешное выполнение команды
    if (txt_index > 0)
    {
        if (bt.err == 0)
            checkCommand(2, "OK");
        else
            checkCommand(5, "ERROR");
    }
    bt.err = 0; // Сброс ошибок.
    txt_index = find_index("AT+VER");
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
         if (strlen(event_bt) > 11)
        {
            bt.present = true;
            if (strcmp(event_bt, MainConfig->bt_ver) != 0)
            {
                strcpy(MainConfig->bt_ver, event_bt);
                SaveConfig();
            }
        }
        else
        {
            bt.present = false;
            strcpy(MainConfig->bt_ver, "NOT PRESENT");
        }
    }
    txt_index = find_index("QA+"); // Текущая громкость
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_vol = atoi(event_bt);
        ESP_LOGI(TAG, "Current Volume: %u\n", bt.current_vol);
    }
    txt_index = find_index("QM+"); // Текущий режим
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_mode = atoi(event_bt);
    }
    txt_index = find_index("QM+"); // Текущий режим
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.current_mode = atoi(event_bt);
    }
    txt_index = find_index("QN+"); // prompt sound
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.prompt_sound = atoi(event_bt);
    }
    txt_index = find_index("QK+"); // automatically switch to Bluetooth
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.auto_bluetooth = atoi(event_bt);
    }
    txt_index = find_index("QG+"); // Bluetooth Background
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.bluetooth_background = atoi(event_bt);
    }
    txt_index = find_index("Q1+"); // AD button
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.ad_button = atoi(event_bt);
    }
    txt_index = find_index("RB+"); //  Recording Bit Rate
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
    }
    txt_index = find_index("RV+"); //  gain of setting MIC
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.gain_mic = atoi(event_bt);
    }
    txt_index = find_index("TS+"); //   status indicates that Bluetooth
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.status_bluetooth = atoi(event_bt);
        switch (bt.status_bluetooth)
        {
        case 0: // Bluetooth не был успешно подключен и находится в состоянии ожидания сопряжения
            ESP_LOGW(TAG, "Bluetooth has not been connected successfully and is in a waiting pairing state");
            break;
        case 1: // Bluetooth был успешно подключен, но музыка еще не воспроизводилась. Свободно.
            ESP_LOGI(TAG, "Bluetooth has been successfully connected, but no music has been played yet. Free.");
            break;
        case 2: // Музыка играет по Bluetooth.
            ESP_LOGI(TAG, "Music playing from the Bluetooth.");
            break;
        case 3: // Разговор по телефону.
            ESP_LOGI(TAG, "Talking on the phone.");
            break;
        default:
            break;
        }
    }
    txt_index = find_index("TL+"); //    BLE Simple State
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.ble_state = atoi(event_bt);
        switch (bt.ble_state)
        {
        case 0: // BLE пуст.
            ESP_LOGI(TAG, "BLE is empty.");
            break;
        case 1: // BLE простаивает.
            ESP_LOGI(TAG, "BLE is idle.");
            break;
        case 2: // BLE вещает.
            ESP_LOGI(TAG, "BLE is broadcasting.");
            break;
        case 3: // BLE соединение успешно.
            ESP_LOGI(TAG, "BLE Connection Successful.");
            break;
        case 4: // BLE разъединение.
            ESP_LOGI(TAG, "BLE disconnect.");
            break;
        case 5: // BLE открывает состояние прослушивания.
            ESP_LOGI(TAG, "BLE Opens Listening State.");
            break;
        case 6: // BLE в состоянии сканирования - хост.
            ESP_LOGI(TAG, "BLE in Scanning State - Host.");
            break;
        case 7: // BLE Завершение поиска - Хост.
            ESP_LOGI(TAG, "BLE Search Completion - Host.");
            break;
        default:
            break;
        }
    }
    txt_index = find_index("T1+"); //    Represents the default need to enter a password of "0000"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.need_password = atoi(event_bt);
    }
    txt_index = find_index("T2+"); //    Representative Chip Supports HFP
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_hfp = atoi(event_bt);
    }
    txt_index = find_index("T3+"); //    Delegate chip support A2DP
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_a2dp = atoi(event_bt);
    }
    txt_index = find_index("T4+"); //    Support for BLE on behalf of chip
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_ble = atoi(event_bt);
    }
    txt_index = find_index("T5+"); //    Delegate chip supports EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.support_edr = atoi(event_bt);
    }
    txt_index = find_index("TE+"); //    Return the password of the current Bluetooth connection to "0000"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.current_password, event_bt);
    }
    txt_index = find_index("TD+"); //    Returns the name of the current Bluetooth EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.edr_name, event_bt);
    }
    txt_index = find_index("TA+"); //    Returns the MAC address of the current Bluetooth EDR
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.mac_edr, event_bt);
    }
    txt_index = find_index("TB+"); //    Returns the MAC address of the current Bluetooth BLE
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.mac_ble, event_bt);
    }
    txt_index = find_index("TM+"); //    Returns the name of the current Bluetooth BLE
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.ble_name, event_bt);
    }
    txt_index = find_index("T6+"); //    Specify service UUID as F000
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid0, event_bt);
    }
    txt_index = find_index("T7+"); //    Specify signature 1 as F001, and its feature is "write"+ "listen"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid1, event_bt);
    }
    txt_index = find_index("T8+"); //    Specify signature 2 as F002, which is read + listen
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid2, event_bt);
    }
    txt_index = find_index("T9+"); //    Specify signature 3 as F003 and its feature is "write"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.uuid3, event_bt);
    }
    txt_index = find_index("TR+"); //    Query the broadcast package data of the chip, and the chip will return "TR+9988776655"
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        strcpy(bt.boardcast, event_bt);
    }
    txt_index = find_index("M1+"); //    Play file physical number of current device
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.play_file_number = atoi(event_bt);
    }
    txt_index = find_index("M2+"); //    Total number of files for current devices
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.total_files = atoi(event_bt);
    }
    txt_index = find_index("MT+"); //    Total time for current file playback
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.total_time = atoi(event_bt);
    }
    txt_index = find_index("MK+"); //    Time when the current file has been played
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.current_time = atoi(event_bt);
    }
    txt_index = find_index("MF+"); //    Long File Name of Currently Played Files
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        // bt.current_time = atoi(event_bt);
        if (strlen(event_bt))
        {
            ESP_LOGI(TAG, "Played File Name:\n%s\n", event_bt);
        }
    }
    txt_index = find_index("MU+"); //     TFCard or U Disk - PC Sound Card - Insert and Pull Back Information
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        // strcpy(bt.boardcast, event_bt);
        bt.isert_info = atoi(event_bt);
        switch (bt.isert_info)
        {
        case 1:
            ESP_LOGI(TAG, "USB disk insertion.");
            break;
        case 2:
            ESP_LOGI(TAG, "USB disk pull-out.");
            break;
        case 3:
            ESP_LOGI(TAG, "TF card insertion.");
            break;
        case 4:
            ESP_LOGI(TAG, "TF card pull-out.");
            break;
        case 5:
            ESP_LOGI(TAG, "Connecting PC.");
            break;
        case 6:
            ESP_LOGI(TAG, "PC pull-out.");
            break;
        default:
            break;
        }
    }
    txt_index = find_index("ER+"); // Ошибки
    if (txt_index > 0)
    {
        unicode_to_utf8(txt_index);
        bt.err = atoi(event_bt);
        ESP_LOGE(TAG, "#BT ERROR #%u :", bt.err);
        switch (bt.err)
        {
        case 1:
            // Полученный фрейм данных не корректен
            ESP_LOGE(TAG, "The received data frame is not correct.");
            break;
        case 2:
            // Полученная команда не существует
            ESP_LOGE(TAG, "The received command does not exist.");
            break;
        case 3:
            // При записи устройство не подключено или другие ошибки
            ESP_LOGE(TAG, "When recording, the device is not online or other errors.");
            break;
        case 4:
            // Отправленные параметры выходят за пределы допустимого диапазона, либо формат команды неверен.
            ESP_LOGE(TAG, "Sent parameters are out of range or the command format is incorrect.");
            break;
        case 5:
            // Указанное устройство [TF или U диск] не подключено или работает неправильно.
            ESP_LOGE(TAG, "The specified device [TF or U disk] is not connected or is not working properly.");
            break;
        case 6:
            // Ошибка в указанном пути к устройству [диск TF или U].
            ESP_LOGE(TAG, "Error in the specified device path [TF or U disk].");
            break;
        default:
            // Не определено.
            ESP_LOGE(TAG, "Undetermined.");
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
    if (txBytes == -1)
        ESP_LOGE(TAG, "Write to uart fail!!");
}

static void uartBtTask()
{
    uart_event_t event;
    size_t buffered_size;
    bt_data = (uint8_t *)malloc(RD_BUF_SIZE + 1);
    bt.debug = false;
    while (1)
    {
        //Waiting for UART event.
        if (xQueueReceive(bt_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            bzero(bt_data, RD_BUF_SIZE);
            switch (event.type)
            {
            case UART_DATA:
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
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                break;
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_NUM_1, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_NUM_1);
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
                        // int uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                        if (bt.debug)
                        {
                            // ESP_LOGI("uart1Task", "watermark: %d  heap: %d", uxHighWaterMark, xPortGetFreeHeapSize());
                            ESP_LOGI(TAG, "read data:\"%s\"\nsize: %d", bt_data, rxBytes);
                            ESP_LOG_BUFFER_HEXDUMP(TAG, bt_data, pos, ESP_LOG_INFO);
                        }
                        parse_event();
                    }
                }
                break;
            default:
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
    uint32_t uspeed = MainConfig->uartspeed;
    gpio_num_t bt_rxd, bt_txd;
    esp_err_t err = gpio_get_uart(&bt_rxd, &bt_txd);
    if (bt_rxd != GPIO_NONE && bt_txd != GPIO_NONE)
    {
        uart_config_t uart_config1 = {
            .baud_rate = uspeed,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
        };
        err |= uart_param_config(UART_NUM_1, &uart_config1);

        err |= uart_set_pin(UART_NUM_1, bt_txd, bt_rxd, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        //Install UART driver, and get the queue.
        err |= uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 20, &bt_queue, 0);

        //Set uart pattern detect function.
        err |= uart_enable_pattern_det_baud_intr(UART_NUM_1, '\r', PATTERN_CHR_NUM, 9, 0, 0);
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
                ESP_LOGI(TAG, "%s", MainConfig->bt_ver);
            }
            else
            {
                vTaskDelete(xBT_Handdle);
                uart_driver_delete(UART_NUM_1);
                ESP_LOGE(TAG, "%s!", MainConfig->bt_ver);
            }
        }
        else
        {
            ESP_LOGE(TAG, "UART1 err: %X", err);
        }
    }
}