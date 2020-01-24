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
  + ESP32-WROVER 16M flash, 8M SPIRAM - микроконтроллер, который рулит всем доступным железом в проекте ;-).

  **Внешний вид и схему подключения некоторых компонентов можно глянуть в каталоге [pictures](https://github.com/SinglWolf/ESP32-Radiola/tree/master/pictures).**


Проект делаю для себя. Никаких хроник изменений не веду. Как только реализую основные задачи, напишу развёрнутый мануал.
Проект на стадии Beta 1 :-)

**Вырезано из Ka-Radio32:**
  - Вырезаны все аудиокодеки, аудиодекодеры, кроме VS1053b
  - Вырезана поддержка всех дисплеев, кроме TFT ILI9341 18x240x320
  - Вырезаны все интерфейсы, кроме веб-интерфейса, тачскрина, энкодеров и ик-пульта
  - ...будет вырезано всё, что не используется :-)

**Реализовано сейчас:**
  + Переопределение GPIOs (пинов ESP32) через web-интерфейс. (Прошивать на чистую FLASH!!!)
  + Русифицированы сообщения, выводимые на дисплей.
  + Написана библиотека для TDA7313 (аудиопроцессор)
  + Тестовая поддержка TDA7313 в виде переключения каналов в зависимости от состояния проигывания станций радио. Play - вход 2 (VS1053), Stop - вход 1 (Компьютер).
  + Полное управление всеми функциями TDA7313 через web-интерфейс (в отдельной вкладке).
  + Датчик температуры DS18B20 для контроля температуры радиаторов усилителя.
  + Вывод показаний датчика температуры DS18B20 на странице веб-интерфейса.
  + Тахометр (счётчик оборотов) вентилятора охлаждения радиаторов усилителя.
  + Вывод показаний тахометра на странице веб-интерфейса.
  + Русифицирован веб-интерфейс.
  + Пин P_RST (ESP32) для управления VS1053 удалён из библиотеки без потери функциональности +освободился 1 пин. Пин XRESET (VS1053) нужно подключить к пину EN (RESET) ESP32.

**TODO (В ближайшее время):**
  + Реализовать другой механизм переназначения опций и кодов ик-пульта, а также возможность чтения\сохранения всех настроек в\из файл(а).
  + Добавить ручное управление оборотами вентилятора охлаждения радиаторов усилителя
  + Добавить автоматическое управление оборотами вентилятора охлаждения радиаторов усилителя в зависимости то температуры радиаторов
  + Добавить звуковую индикацию событий BUZZER (пищалки).
  + etc...

**Распиновка** 

| ESP32 | VS1053  | TFT  | XPT2046 | TDA7317  | DS1820  |  Fan  | LED  | DS3231SN | IRLED | BUZZER |
| :---: | :-----: | :--: | :-----: | :------: | :-----: | :---: | :--: | :------: | :---: | :----: |
|   19  |  MISO   | MISO | T_MISO  |          |         |       |      |          |       |        |
|      23       |  MOSI   | MOSI | T_MOSI  |          |         |       |      |          |       |        |
|      18       |  CLK    | CLK  | T_CLK   |          |         |       |      |          |       |        |
|       5       |  XCS    |      |         |          |         |       |      |          |       |        |
|      32       |  XDCS   |      |         |          |         |       |      |          |       |        |
|       4       |  DREQ   |      |         |          |         |       |      |          |       |        |
|      21       |         |      |         |    SCL   |         |       |      |   SCL    |       |        |
|      22       |         |      |         |    SDA   |         |       |      |   SDA    |       |        |
|      27       |         | CS   |         |          |         |       |      |          |       |        |
|       2       |         | DC   |         |          |         |       |      |          |       |        |
|      25       |         |      |         |          |         |       | +LED |          |       |        |
|      35       |         |      |         |          |         |       |      |          |+IRLED |        |
|      26       |         | LED  |         |          |         |       |      |          |       |        |
|      12       |         |      |         |          |         | TACH  |      |          |       |        |
|      13       |         |      |         |          |         | PWM   |      |          |       |        |
|      14       |         |      |         |          |  DS1820 |       |      |          |       |        |
|       0       |         |      | T_CS    |          |         |       |      |          |       |        |
|      33       |         |      |         |          |         |       |      |          |       |+BUZZER |
|    RES(EN)    |  RESET  | RES  |         |          |         |       |      |          |       |        |
|               |         |      |         |          |         |       |      |          |       |        |
|               |         |      |         |          |         |       |      |          |       |        |

![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/ESP32-Radiola.png)
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/amplifier.jpg)
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/ESP32WROVER.jpg)
![Image alt](https://github.com/SinglWolf/ESP32-Radiola/raw/master/pictures/display.jpg)
