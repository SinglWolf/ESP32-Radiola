# ESP32-Radiola
Форк проекта https://github.com/karawin/Ka-Radio32 на VSCode+PlatformIO

**Все вопросы по проекту пишите [ТУТ](https://serverdoma.ru/viewtopic.php?f=70&t=1178).**

**Используемое железо (HARDWARE):**
  + Корпус - старый системный блок компьютера.
  + Силовой трансформатор - от УЛПЦТ, перемотанный на несколько напряжений.
  + Радиаторы - от усилителя Вега 50У-122С.
  + Аудиоколонки - от усилителя Вега 50У-122С.
  + 2 платы усилителя на TDA7293.
  + TDA7313 - аудиопроцессор.
  + VS1053B (MP3-декодер для web-радио).
  + UPC1237 - защита колонок от щелчков при включении/выключении и появлении постоянного напряжения.
  + BA3121 - изолирующий усилитель для подавления помех (шумов) при подключении VS1053B к TDA7313.
  + DS3231SN - часы реального времени. (Если нет интернета, короче :-)).
  + Вентилятор охлаждения радиаторов усилителя - 4-х пиновый (с ШИМ) от какой-то материнской платы компьютера.
  + DS1820 - датчик температуры для контроля нагрева радиаторов усилителя.
  + Цветной TFT дисплей 18x320x240 3,2 дюйма. Драйвер SPI на ILI9341.
  + Тачскрин на XPT2046.
  + IR-модуль с простым пультом.
  + ESP32-WROVER 4M flash, 4M SPIRAM - микроконтроллер, который рулит всем доступным железом в проекте ;-).
  + **(Возможно использовать ESP32-WROOM 4M flash без SPIRAM, буфер под поток радио будет меньше)**.

  **Внешний вид и схему подключения некоторых компонентов можно глянуть в каталоге [pictures](https://github.com/SinglWolf/ESP32-Radiola/tree/master/pictures).**


Проект делаю для себя. Никаких хроник изменений не веду. Как только реализую основные задачи, напишу развёрнутый мануал.

**(Прошивать на чистую FLASH!!!)**

**[демо веб-интерфейса](https://serverdoma.ru/esp32/newWeb/index.html)**

**Реализовано сейчас:**
  + Радиола переехала на <strong>framework-espidf 4.0.1</strong>.
  + Реализована тестовая поддержка модуля BTX01, через консоль UART и TELNET.
  + Доступные команды для модуля BTX01 можно посмотреть, набрав в консоли "help".
  + Функция регулировки яркости подсветки дисплея реализована в тестовом режиме, без настройки в интерфейсах.
  + Реализован обучаемый ИК-пульт с сохранением кодов в NVS.
  + 4 сервера точного времени (NTP). Системное время Радиолы синхронизируется с NTP каждый час.
  + Можно прописать свои сервера NTP. Все поля должны быть заполнены. Сервер с номером **0:** имеет высший приоритет.
  + Часовые пояса настроены с учётом наличия летнего времени в регионе. Переход на летнее время происходит автоматически.
  + **Если Вашего города нет в списке часовых поясов Радиолы, пожалуйста, сообщите мне свой город и я в ближайшей версии Радиолы его добавлю.**
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
  + Реализовать функцию чтения\сохранения всех настроек в\из файл(а).
  + Реализовать ручное/автоматическое управление оборотами вентилятора охлаждения радиаторов усилителя в зависимости то температуры радиаторов.
  + Реализовать регулировку яркости подсветки дисплея (ручная, в зависимости от времени суток, в зависимости от уровня освещённости).
  + Добавить поддержку RTC DS3231 (часы).
 
 **TODO (перспектива):**
  + Переделать функцию **decodeHttpMessage()**.
  + Сделть 1 кнопку для вкл\выкл звука и совместить с функцией, чтобы при удержании кнопки при включении сбрасывались все настройки.
  + Реализовать обнвление прошивки через OTA с локального сервера.
  + Добавить поддержку эквалайзера TDA7317.
  + Реализовать выборку и воспроизведение станций из основного списка станций в отдельный список-категорию "Избранное".
  + etc...

**Распиновка по-умолчанию** 

| ESP   | VS1053<br/>TFT<br/>TOUCH  | TDA7313<br/>TDA7317<br/>DS3231 | DS1820 |  FAN  | IR | BUZZ | STAND<br/>BY | BTX01 | ENC | KBD | LDR |
| :---: | :-----: | :-----: | :----: | :---: | :---: | :----: |:----:|:----:|:----:|:----:|:----:|
|   19  |  MISO   |         |        |       |       |        |      |      |      |      |      |
|   23  |  MOSI   |         |        |       |       |        |      |      |      |      |      |
|   18  |  CLK    |         |        |       |       |        |      |      |      |      |      |
|   5   |  XCS    |         |        |       |       |        |      |      |      |      |      |
|   32  |  XDCS   |         |        |       |       |        |      |      |      |      |      |
|   4   |  DREQ   |         |        |       |       |        |      |      |      |      |      |
|   21  |         |(##)SDA  |        |       |       |        |      |      |      |      |      |
|   22  |         |(##)SCL  |        |       |       |        |      |      |      |      |      |
|   27  |   CS    |         |        |       |       |        |      |      |      |      |      |
|   2   |   DC    |         |        |       |       |        |      |      |      |      |      |
|   25  |         |         |        |       |       |        | STB  |      |      |      |      |
|   35  |         |         |        |       |+IR    |        |      |      |      |      |      |
|   26  |  LED    |         |        |       |       |        |      |      |      |      |      |
|   12  |         |         |        | TACH  |       |        |      |      |      |      |      |
|   13  |         |         |        |(#)PWM |       |        |      |      |(#)DT |      |      |
|   14  |         |         |(#)DS   |       |       |        |      |      |(#)CLK|      |      |
|   0   |  T_CS   |         |        |       |       |        |      |      |      |      |      |
|   33  |         |         |        |       |       |+BUZ    |      |      |      |      |      |
|   36  |         |         |        |       |       |        |      |  RX  |      |      |      |
|   15  |         |         |        |       |       |        |      |  TX  |      |      |      |
|  (EN) |RESET    |         |        |       |       |        |      |      |      |      |      |
|   34  |         |         |        |       |       |        |      |      |(#)SW |(#)KEY|      |
|   39  |         |         |        |       |       |        |      |      |      |      | LDR  |

(#) - отмечены пины, которые можно использовать для подключения только ОДНОГО интерфейса!!!

Например, если вы не желаете иметь термометр и управление оборотами вентилятора, к GPIO13(PWM) можно подключить вывод (DT) энкодера.
И, наоборот, если вы желаете подключить энкодер, увы, придётся отказаться от управления оборотами вентилятора и термометра.

**!!! ВНИМАНИЕ НЕ ПОДКЛЮЧАЙТЕ ОДНОВРЕМЕННО КЛАВИАТУРУ И ЭНКОДЕР !!!**

(##) - отмечены пины, которые можно использовать для подключения другого интерфейса!!!

Например, если у Вас не подключены TDA7313, TDA7317 и DS3231, к этим GPIO можно подключить термометр, если у Вас подключен энкодер.


**[flash_download_tools_v3.6.8](https://www.espressif.com/sites/default/files/tools/flash_download_tools_v3.6.8.zip)**

**!!!!Перед прошивкой обязательно проверьте контрольные суммы всех трёх бинарных файлов!!!!**

Удобная программа для проверки контрольной суммы файла [#HashTab](https://hashtab.ru/).<br>
(После установки увидите инструкцию в картинках)
 
 *адреса:*
  + bootloader.bin - 0x1000
  + partitions.bin - 0x8000
  + ESP32Radiola.bin - 0x10000 и 0x1D0000

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
