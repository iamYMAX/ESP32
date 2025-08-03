#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// --- Публичные функции ---

// Инициализация дисплея
void display_init();

// Главная функция обновления, вызывается в loop()
void display_update();

// Функции для передачи данных на дисплей
void display_set_rpm(int rpm);
void display_set_wifi_status(bool connected, String ip_addr);
// ... другие сеттеры по мере необходимости ...

#endif // DISPLAY_H
