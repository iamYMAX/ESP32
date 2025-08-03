#ifndef CRANK_SIGNAL_H
#define CRANK_SIGNAL_H

#include <Arduino.h>

// Структура для хранения параметров паттерна коленвала
struct CrankPattern {
    const char* name;
    int total_teeth;
    int missing_teeth;
};

// Поддерживаемые паттерны
const CrankPattern patterns[] = {
    {"60-2", 60, 2},
    {"36-1", 36, 1},
    {"36-2", 36, 2},
    {"12-1", 12, 1}
};
const int num_patterns = sizeof(patterns) / sizeof(patterns[0]);

// --- Публичные функции ---

// Инициализация генератора
void crank_signal_init(uint8_t output_pin);

// Установка нового паттерна по имени
bool crank_signal_set_pattern(const char* pattern_name);

// Установка оборотов в минуту
void crank_signal_set_rpm(int rpm);

// Получение текущих параметров
const char* crank_signal_get_current_pattern_name();
int crank_signal_get_current_rpm();


#endif // CRANK_SIGNAL_H
