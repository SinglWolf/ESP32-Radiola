// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Long Click
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
//
// Modified for ESP32 IDF
// (c) 2017 KaRadio
// Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
// ----------------------------------------------------------------------------

#ifndef __have__ClickEncoder_h__
#define __have__ClickEncoder_h__

#include "driver/gpio.h"
// ---Button defaults-------------------------------------------------------------
#define ENC_BUTTONINTERVAL 10   // check enc->button every x milliseconds, also debouce time
#define BTN_DOUBLECLICKTIME 600 // second click within 400ms
#define BTN_HOLDTIME 1000       // report held button after 1s

// ----------------------------------------------------------------------------
// #undef INPUT
// #define INPUT GPIO_MODE_INPUT
// #undef INPUT_PULLUP
// #define INPUT_PULLUP GPIO_MODE_INPUT
// #undef LOW
#define LOW 0
// #undef HIGH
#define HIGH 1
#define digitalRead(x) gpio_get_level((gpio_num_t)x)
#ifndef __have__ClickButton_h__
typedef enum button
{
  Open = 0,
  Closed,
  Pressed,
  Held,
  Released,
  Clicked,
  DoubleClicked
} button_e;
#endif

typedef struct encoder
{
  int8_t pinA;
  int8_t pinB;
  int8_t pinBTN;
  bool pinsActive;
  volatile int16_t delta;
  volatile int16_t last;
  volatile uint8_t steps;
  volatile uint8_t accel_inc;
  volatile int16_t acceleration;
  bool accelerationEnabled;
  volatile button_e button;
  bool doubleClickEnabled;
  bool buttonHeldEnabled;
  uint16_t keyDownTicks;
  uint16_t doubleClickTicks;
  unsigned long lastButtonCheck;
} encoder_s;

encoder_s *ClickEncoderInit(int8_t A, int8_t B, int8_t BTN, bool half);
void setHalfStep(encoder_s *enc, bool value);
bool getHalfStep(encoder_s *enc);
void service(encoder_s *enc);
int16_t getValue(encoder_s *enc);
button_e getButton(encoder_s *enc);
bool getPinState(encoder_s *enc);
bool getpinsActive(encoder_s *enc);

#endif // __have__ClickEncoder_h__
