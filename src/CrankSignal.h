#ifndef CRANK_SIGNAL_H
#define CRANK_SIGNAL_H

#include <Arduino.h>

// --- Типы и константы ---

// Структура для хранения параметров паттерна коленвала
struct CrankPattern {
    const char* name;
    int total_teeth;
    int missing_teeth;
};

// --- Публичные функции ---

// Инициализация модуля
void engine_simulator_init(uint8_t crank_pin, uint8_t ignition_pin);

// Управление генератором ДПКВ
bool engine_simulator_set_pattern(const char* pattern_name);
void engine_simulator_set_rpm(int rpm);

// Управление зажиганием
void engine_simulator_set_dwell_time(float milliseconds);
void engine_simulator_set_ignition_angle(int degrees_btdc);

// Функции для получения текущего статуса
const char* engine_simulator_get_current_pattern_name();
int engine_simulator_get_current_rpm();
float engine_simulator_get_current_dwell_time();
int engine_simulator_get_current_ignition_angle();

#endif // CRANK_SIGNAL_H
