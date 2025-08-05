#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// --- Типы данных ---
struct GpioPin {
    const uint8_t pin;
    const char* name;
    bool state;
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
// ... другие сеттеры по мере необходимости ...

#endif // DISPLAY_H
