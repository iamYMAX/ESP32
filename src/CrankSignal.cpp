#include "CrankSignal.h"

// --- Конфигурация и переменные модуля ---
#define ISR_FREQUENCY_HZ 100000
#define TIMER_MICROS (1000000 / ISR_FREQUENCY_HZ)

static hw_timer_t *engine_timer = NULL;
static uint8_t crank_pin = -1;
static uint8_t ignition_pin = -1;

const CrankPattern patterns[] = {
    {"60-2", 60, 2}, {"36-1", 36, 1}, {"36-2", 36, 2}, {"12-1", 12, 1}
};
const int num_patterns = sizeof(patterns) / sizeof(patterns[0]);

// --- Состояние симулятора (целочисленное) ---
static volatile uint16_t current_rpm = 0;
static const CrankPattern* current_pattern = &patterns[0];
static float current_dwell_ms = 3.0;
static volatile uint32_t dwell_angle_res = 0;
static volatile uint16_t ignition_angle_btdc = 10;

static volatile uint32_t current_angle_res = 0;
static volatile uint32_t angle_per_tick_res = 0;
static volatile bool is_dwelling = false;

void IRAM_ATTR onEngineTimer();

// --- Реализация функций ---
void engine_simulator_init(uint8_t _crank_pin, uint8_t _ignition_pin) {
    crank_pin = _crank_pin;
    ignition_pin = _ignition_pin;
    pinMode(crank_pin, OUTPUT);
    pinMode(ignition_pin, OUTPUT);

    engine_timer = timerBegin(1, 80, true);
    timerAttachInterrupt(engine_timer, &onEngineTimer, true);
    timerAlarmWrite(engine_timer, TIMER_MICROS, true);

    engine_simulator_set_rpm(0);
}

void engine_simulator_set_rpm(uint16_t rpm) {
    // Атомарно обновляем параметры, отключая прерывание на время расчетов
    timerAlarmDisable(engine_timer);

    current_rpm = rpm;
    if (rpm == 0) {
        digitalWrite(crank_pin, LOW);
        digitalWrite(ignition_pin, LOW);
        return; // Оставляем таймер выключенным
    }

    uint64_t total_angle_per_min = (uint64_t)rpm * ENG_DEGREES_PER_REV;
    uint64_t total_angle_per_sec = total_angle_per_min / 60;
    angle_per_tick_res = total_angle_per_sec / ISR_FREQUENCY_HZ;

    float dwell_sec = current_dwell_ms / 1000.0f;
    dwell_angle_res = (uint32_t)(dwell_sec * total_angle_per_sec);

    // Сбрасываем угол и состояние, чтобы избежать глитчей при смене оборотов
    current_angle_res = 0;
    is_dwelling = false;

    timerAlarmEnable(engine_timer);
}

// --- Функции установки параметров (выполняются в основном потоке) ---
bool engine_simulator_set_pattern(const char* name) { /* ... */ }
void engine_simulator_set_dwell_time_ms(float ms) { current_dwell_ms = ms; engine_simulator_set_rpm(current_rpm); }
void engine_simulator_set_ignition_angle_btdc(uint16_t degrees) { ignition_angle_btdc = degrees; }

// --- Функции получения статуса ---
const char* engine_simulator_get_current_pattern_name() { return current_pattern->name; }
uint16_t engine_simulator_get_current_rpm() { return current_rpm; }
float engine_simulator_get_current_dwell_time_ms() { return current_dwell_ms; }
uint16_t engine_simulator_get_current_ignition_angle_btdc() { return ignition_angle_btdc; }

// --- Главная функция прерывания (ISR) ---
void IRAM_ATTR onEngineTimer() {
    current_angle_res += angle_per_tick_res;
    if (current_angle_res >= ENG_DEGREES_PER_REV) {
        current_angle_res -= ENG_DEGREES_PER_REV;
        is_dwelling = false; // Prevent stuck dwell on rollover
    }

    // 1. Сигнал ДПКВ
    uint32_t tooth_angle_res = ENG_DEGREES_PER_REV / current_pattern->total_teeth;
    uint32_t missing_teeth_start_angle = (current_pattern->total_teeth - current_pattern->missing_teeth) * tooth_angle_res;

    if (current_angle_res >= missing_teeth_start_angle) {
        digitalWrite(crank_pin, LOW);
    } else {
        digitalWrite(crank_pin, (current_angle_res % tooth_angle_res) < (tooth_angle_res / 2));
    }

    // 2. Зажигание
    uint32_t ignition_angle_res = ignition_angle_btdc * ENG_RESOLUTION_MULT;
    uint32_t dwell_start_angle_res = ignition_angle_res + dwell_angle_res;

    if (!is_dwelling && current_angle_res >= dwell_start_angle_res && current_angle_res < (dwell_start_angle_res + angle_per_tick_res)) {
        digitalWrite(ignition_pin, HIGH);
        is_dwelling = true;
    }
    if (is_dwelling && current_angle_res >= ignition_angle_res && current_angle_res < (ignition_angle_res + angle_per_tick_res)) {
        digitalWrite(ignition_pin, LOW);
        is_dwelling = false;
    }
}

// Тело функции set_pattern не изменилось
// bool engine_simulator_set_pattern(const char* name) {
//     for (int i = 0; i < num_patterns; i++) {
//         if (strcmp(patterns[i].name, name) == 0) {
//             current_pattern = &patterns[i];
//             engine_simulator_set_rpm(current_rpm);
//             return true;
//         }
//     }
//     return false;
// }
