# ESP32-Radiola
Форк проекта https://github.com/karawin/Ka-Radio32

**Немного о проекте:**

Проект **ESP32-Radiola** кросс-платформенный, т.е. компиляция и сборка будет работать как на **Linux**-, так **Windows**- совместимых **OS**.
Для этого нужно скачать и установить для Вашей ОС среду разработки **VS Code** от MicroSoft по этой ссылке https://code.visualstudio.com/download

![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/vscode.png)

Затем в **VS Code** нужно установить необходимые компоненты: **Russian Language Pack for Visual Studio Code** и **PlatformIO IDE**.

**Инструкция:** [Установка и настройка IDE для программирования ESP32](https://serverdoma.ru/viewtopic.php?f=70&t=1179)

**Для полноценной работы необходима подержка Java**. Скачать можно по ссылке https://www.java.com/ru/download/

**Все вопросы по проекту пишите в** [ESP32-Radiola - Музыкальный центр с управлением на ESP32 своими руками](https://serverdoma.ru/viewtopic.php?f=70&t=1178).

**[демо веб-интерфейса Радиолы](https://serverdoma.ru/esp32/newWeb/index.html)**

**Используемое железо (HARDWARE):**
  + Корпус - старый системный блок компьютера.
  + Силовой трансформатор - от усилителя Вега 50У-122С, с дополнительной обмоткой.
  + Радиаторы - от усилителя Вега 50У-122С.
  + Аудиоколонки - от усилителя Вега 50У-122С.
  + 2 платы усилителя на TDA7293.
  + \***ESP32-WROVER 4M flash, 4M SPIRAM** - микроконтроллер, который рулит всем доступным железом в проекте ;-).<br/>
       *(Возможно использовать ESP32-WROOM 4M flash без SPIRAM, буфер под поток радио будет меньше)*.
  + TDA7313 - аудиопроцессор.
  + \***VS1053B** (MP3-декодер для web-радио).
  + UPC1237 - защита колонок от щелчков при включении/выключении и появлении постоянного напряжения.
  + BA3121 - изолирующий усилитель для подавления помех (шумов) при подключении VS1053B к TDA7313.
  + DS3231SN - часы реального времени. (Если нет интернета, короче :-)).
  + Вентилятор охлаждения радиаторов усилителя - 4-х пиновый (с ШИМ) от какой-то материнской платы компьютера.
  + DS1820 - датчик температуры для контроля нагрева радиаторов усилителя. (Можно мерить температуру в комнате :-))
  + \***Цветной TFT дисплей 18x320x240. Драйвер SPI на ILI9341.** (У меня размер экрана 3,2 дюйма.)
  + Тачскрин на XPT2046.
  + IR-модуль с простым пультом.
  + BTx01 - блютуз-модуль ([ссылка в магазин на Ali](https://aliexpress.ru/item/4000099619425.html))
  
  *\* Жирным шрифтом отмечена минимальная рабочая конфигурация*

  **Внешний вид и схему подключения некоторых компонентов можно глянуть в каталоге [pictures](https://github.com/SinglWolf/ESP32-Radiola/tree/master/pictures).**

Проект делаю для себя. Никаких хроник изменений не веду. Как только реализую основные задачи, напишу развёрнутый мануал.

**(Прошивать на чистую FLASH!!!)**

**ИНФОРМАЦИЯ**
  + У Радиолы две прошивки.<br/>
    [**Релизная**](https://github.com/SinglWolf/ESP32-Radiola/tree/master/binaries/release) - для постоянного пользования.<br/>
    [**Отладочная**](https://github.com/SinglWolf/ESP32-Radiola/tree/master/binaries/debug) - для анализа поведения, отадки и поиска возможных ошибок.<br/>
  + Вся забота об отслеживании изменений в файлах, необходимых для работы веб-интерфейса, теперь лежит на PlatformIO. При любом изменени в этих файлах, автоматически пересоздаются заголовочные файлы с массивами данных в формате языка С.
  + Проект Радиола собран на **framework-espidf 4.1.0**.
  + Проект Радиола можно запускать как на **Linux**, так и на **Windows**.
  + По-умолчанию в **VS Code** формат конца сток **CRLF**. В **Linux** же **LF**. Если Вы будете работать проекте под управлением ОС **Windows**, **обязательно настройте Linux-совместимый  формат конца строк**. Для этого зайдите в настройки параметров, наберите в строке поиска "***eol***" и установите формат строк как на картинке:
  
    ![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/eol.png)
  + **Перед первой настройкой проекта командой** `Run Menuconfig`**, обязательно сделать копию файла** `defconfig` **с именем** `sdkconfig` **в корне проекта. В файле** `defconfig` **записана необходимая первоначальная настройка ESP32!**

**ПРОШИВКА собрана без поддержки тачскрина!!! Если Вы хотите подключить дисплей с тачскрином:**

  В файле `.platformio\packages\framework-espidf\components\driver\spi_master.c`
  
  Строка 147, заменить
  
  ```cpp
  #define NO_CS 3     //Number of CS pins per SPI host
  ```
    
  на

  ```cpp
  #define NO_CS 5     //Number of CS pins per SPI host
  ```
  и определить пин тачскрина через задачу `Run Menuconfig`

**Реализовано сейчас:**
  + Радиола поддерживает загрузку и выгрузку плейлистов радиостанций в стандартном формате **M3U**.
  + Максимальное количество радиостанций в Радиоле ограничено до **128**.
  + Индикация выхода нового релиза Радиолы. Цвет логотипа Радиолы в верхнем левом углу на странице веб-интерфейса меняется с зелёного на красный.
  + Полностью реструктурирована разметка FLASH-памяти.
  + Настройка конфигурации Радиолы и переопределение пинов ESP32 производится через через задачу `Run Menuconfig` из среды PlatformIO.
  + Реализован сброс Радиолы до заводских настроек через кнопку в вебинтерфейсе на вкладке опций.
  + Исправлена ошибка в принципиальной схеме. В схему добавлена разводка питания для BTx01 
  + Реализована тестовая поддержка модуля BTx01, через консоль UART и TELNET. Доступные команды для модуля BTx01 можно посмотреть, набрав в консоли "help".
  + Функция регулировки яркости подсветки дисплея реализована в тестовом режиме, без настройки в интерфейсах.
  + Реализован обучаемый ИК-пульт с сохранением кодов в NVS.
  + 4 сервера точного времени (NTP). Системное время Радиолы синхронизируется с NTP каждый час. Можно прописать свои сервера NTP. Все поля должны быть заполнены. Сервер с номером **0:** имеет высший приоритет. Часовые пояса настроены с учётом наличия летнего времени в регионе. Переход на летнее время происходит автоматически. **Если Вашего города нет в списке часовых поясов Радиолы, пожалуйста, сообщите мне свой город и я в ближайшей версии Радиолы его добавлю.**
  + При отсутствии TDA7313 в веб-интерфейсе Радиолы отключается всё, что касается TDA7313.
  + Обновление через OTA.
  + Реализовано управление громкостью воспроизведения радиостанций/выбор станций через энкодер.
  + Анонсы проекта можно смотреть в веб-интерфейсе.
  + Переопределение GPIOs (пинов ESP32) через web-интерфейс.
  + Полное управление всеми функциями TDA7313 через web-интерфейс (в отдельной вкладке).
  + Датчик температуры DS18B20 для контроля температуры радиаторов усилителя.
  + Вывод показаний датчика температуры DS18B20 на странице веб-интерфейса.
  + Тахометр (счётчик оборотов) вентилятора охлаждения радиаторов усилителя.
  + Вывод показаний тахометра на странице веб-интерфейса.
  + Реализована звуковая индикация (пищалка) нажатий кнопок пульта (энкодера).
  + Функции: управление яркостью дисплея, управление оборотами вентилятора - ПОКА НЕ РЕАЛИЗОВАНЫ  из-за отсутствия необходимых компонентов.

   
**TODO (приоритет):**
  + Реализовать в веб-интерфейсе настройку энкодера.
  + Реализовать в веб-интерфейсе кнопку "Вернуть заводские настройки".
  + Реализовать в веб-интерфейсе кнопку "Отображать часы во время проигрывания станций".
  + Реализовать ручное/автоматическое управление оборотами вентилятора охлаждения радиаторов усилителя в зависимости то температуры радиаторов.
  + Реализовать регулировку яркости подсветки дисплея (ручная, в зависимости от времени суток, в зависимости от уровня освещённости).
  + Добавить поддержку RTC DS3231 (часы).
  + Реализовать возможность сборки прошивки с поддержкой тачскрина без правки в системных библиотеках
  
 **TODO (перспектива):**
  + Переделать функцию **decodeHttpMessage()**.
  + Сделть 1 кнопку для вкл\выкл звука и совместить с функцией, чтобы при удержании кнопки при включении сбрасывались все настройки.
  + Реализовать обнвление прошивки через OTA с локального сервера.
  + Добавить поддержку эквалайзера TDA7317.
  + Реализовать выборку и воспроизведение станций из основного списка станций в отдельный список-категорию "Избранное".
  + etc...

**Распиновка по-умолчанию** 

| ESP   | VS1053<br/>TFT<br/>TOUCH  | TDA7313<br/>TDA7317<br/>DS3231 | DS1820 |  FAN  | IR | BUZZ | STAND<br/>BY | BTx01 | ENC | KBD | LDR |
| :---: | :-----: | :-----: | :----: | :---: | :---: | :----: |:----:|:----:|:----:|:----:|:----:|
|   19  |  MISO   |         |        |       |       |        |      |      |      |      |      |
|   23  |  MOSI   |         |        |       |       |        |      |      |      |      |      |
|   18  |  CLK    |         |        |       |       |        |      |      |      |      |      |
|   5   |  XCS    |         |        |       |       |        |      |      |      |      |      |
|   32  |  XDCS   |         |        |       |       |        |      |      |      |      |      |
|   4   |  DREQ   |         |        |       |       |        |      |      |      |      |      |
|   21  |         | \*\*SDA |        |       |       |        |      |      |      |      |      |
|   22  |         | \*\*SCL |        |       |       |        |      |      |      |      |      |
|   27  |   CS    |         |        |       |       |        |      |      |      |      |      |
|   2   |   DC    |         |        |       |       |        |      |      |      |      |      |
|   25  |         |         |        |       |       |        | STB  |      |      |      |      |
|   35  |         |         |        |       |  +IR  |        |      |      |      |      |      |
|   26  |   LED   |         |        |       |       |        |      |      |      |      |      |
|   12  |         |         |        | TACH  |       |        |      |      |      |      |      |
|   13  |         |         |        | \*PWM |       |        |      |      | \*DT |      |      |
|   14  |         |         |  \*DS  |       |       |        |      |      | \*CLK|      |      |
|   0   |  T_CS   |         |        |       |       |        |      |      |      |      |      |
|   33  |         |         |        |       |       |  +BUZ  |      |      |      |      |      |
|   36  |         |         |        |       |       |        |      |  RX  |      |      |      |
|   15  |         |         |        |       |       |        |      |  TX  |      |      |      |
|  (EN) |  RESET  |         |        |       |       |        |      |      |      |      |      |
|   34  |         |         |        |       |       |        |      |      | \*SW | \*KEY|      |
|   39  |         |         |        |       |       |        |      |      |      |      | LDR  |

\* - отмечены пины, которые можно использовать для подключения только ОДНОГО интерфейса!!!

Например, если вы не желаете иметь термометр и управление оборотами вентилятора, к GPIO13(PWM) можно подключить вывод (DT) энкодера.
И, наоборот, если вы желаете подключить энкодер, увы, придётся отказаться от управления оборотами вентилятора и термометра.

**!!! ВНИМАНИЕ НЕ ПОДКЛЮЧАЙТЕ ОДНОВРЕМЕННО КЛАВИАТУРУ И ЭНКОДЕР !!!**

\*\* - отмечены пины, которые можно использовать для подключения другого интерфейса!!!

Например, если у Вас не подключены TDA7313, TDA7317 и DS3231, к этим GPIO можно подключить термометр, если у Вас подключен энкодер.


**[flash_download_tools_v3.6.8](https://www.espressif.com/sites/default/files/tools/flash_download_tools_v3.6.8.zip)**

**!!!!Перед прошивкой обязательно проверьте контрольные суммы всех трёх бинарных файлов!!!!**

Удобная программа для проверки контрольной суммы файла [#HashTab](https://hashtab.ru/).<br>
(После установки увидите инструкцию в картинках)
 
 *адреса:*
  + bootloader.bin - 0x1000
  + partitions.bin - 0x8000
  + ESP32Radiola(-**release** или -**debug**).bin - 0x10000 и 0x1D0000

![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/flash_download_tools.png)

**[ESP32-Radiola - если что-то пошло не так...](https://serverdoma.ru/viewtopic.php?f=70&t=1183)**

**[демо веб-интерфейса](https://serverdoma.ru/esp32/newWeb/index.html)**

**Принципиальная схема Радиолы**
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/ESP32-Radiola.jpg)

**Принципиальная схема периферийных устройств Радиолы**
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/Peripherals-Radoila.jpg)


**Прототип**
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/amplifier.jpg)
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/ESP32WROVER.jpg)
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/display.jpg)
