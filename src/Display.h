#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// --- Типы данных ---
struct GpioPin {
    const uint8_t pin;
    const char* name;
    bool state;
};

enum InjectorMode { INJ_OFF, INJ_ON, INJ_AUTO, INJ_CLEAN, INJ_BOOST };

struct Injector {
  const uint8_t pin;
  bool state;
  InjectorMode mode;
  int frequency;
  int duty_cycle;
};

// --- Публичные функции ---

// Инициализация дисплея
void display_init();

// Главная функция обновления, вызывается в loop()
void display_update();

// Функции для передачи данных на дисплей
void display_set_rpm(int rpm);
void display_set_wifi_status(bool connected, String ip_addr);
void display_add_log(String message);
void display_next_screen();
void display_set_gpio_pins(const GpioPin* pins, int num_pins);
void display_set_injectors_state(const Injector* injectors, int num_injectors);
// ... другие сеттеры по мере необходимости ...

#endif // DISPLAY_H
