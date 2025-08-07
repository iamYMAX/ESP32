#ifndef ENGINE_SIMULATOR_H
#define ENGINE_SIMULATOR_H

#include <Arduino.h>

// --- Типы и константы ---
#define ENG_RESOLUTION_MULT 100 // Умножаем градусы на 100 для точности
#define ENG_DEGREES_PER_REV (360 * ENG_RESOLUTION_MULT)

struct CrankPattern {
    const char* name;
    const uint16_t total_teeth;
    const uint16_t missing_teeth;
};

// --- Публичные функции ---

void engine_simulator_init(uint8_t crank_pin, uint8_t ignition_pin);

// Управление симулятором
void engine_simulator_set_rpm(uint16_t rpm);
bool engine_simulator_set_pattern(const char* pattern_name);

// Управление зажиганием
void engine_simulator_set_dwell_time_ms(float milliseconds);
void engine_simulator_set_ignition_angle_btdc(uint16_t degrees);

// Функции для получения статуса
const char* engine_simulator_get_current_pattern_name();
uint16_t engine_simulator_get_current_rpm();
float engine_simulator_get_current_dwell_time_ms();
uint16_t engine_simulator_get_current_ignition_angle_btdc();

#endif // ENGINE_SIMULATOR_H
