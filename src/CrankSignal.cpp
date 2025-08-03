#include "CrankSignal.h"

// --- Конфигурация и переменные модуля ---
#define DEGREES_PER_REVOLUTION 360
#define TIMER_RESOLUTION_HZ 1000000 // 1MHz, 1 тик = 1 микросекунда

// Пины
static uint8_t crank_pin = -1;
static uint8_t ignition_pin = -1;

// Аппаратный таймер
static hw_timer_t *engine_timer = NULL;

// Поддерживаемые паттерны
const CrankPattern patterns[] = {
    {"60-2", 60, 2}, {"36-1", 36, 1}, {"36-2", 36, 2}, {"12-1", 12, 1}
};
const int num_patterns = sizeof(patterns) / sizeof(patterns[0]);

// --- Состояние симулятора (volatile, так как используется в ISR) ---
static volatile int current_rpm = 0;
static const CrankPattern* current_pattern = &patterns[0];
static volatile float dwell_time_ms = 3.0;
static volatile int ignition_angle_btdc = 10;

// Угловое состояние
static volatile float current_angle_deg = 0.0;
static volatile float angle_per_tick = 0.0;

// Состояние зажигания
static volatile bool is_dwelling = false;
static volatile float dwell_start_angle = 0.0;

void IRAM_ATTR onEngineTimer(); // Прототип ISR

// --- Реализация публичных функций ---

void engine_simulator_init(uint8_t _crank_pin, uint8_t _ignition_pin) {
    crank_pin = _crank_pin;
    ignition_pin = _ignition_pin;
    pinMode(crank_pin, OUTPUT);
    pinMode(ignition_pin, OUTPUT);
    digitalWrite(crank_pin, LOW);
    digitalWrite(ignition_pin, LOW);

    engine_timer = timerBegin(0, 80, true); // Таймер 0, делитель 80 -> 1МГц
    timerAttachInterrupt(engine_timer, &onEngineTimer, true);

    engine_simulator_set_rpm(0); // По умолчанию выключен
}

void engine_simulator_set_rpm(int rpm) {
    current_rpm = rpm;
    if (rpm <= 0) {
        timerAlarmDisable(engine_timer);
        digitalWrite(crank_pin, LOW);
        digitalWrite(ignition_pin, LOW);
        return;
    }

    // Расчет изменения угла за 1 микросекунду
    float deg_per_second = rpm * (DEGREES_PER_REVOLUTION / 60.0);
    angle_per_tick = deg_per_second / TIMER_RESOLUTION_HZ;

    // Расчет времени накопления в градусах
    float dwell_time_sec = dwell_time_ms / 1000.0;
    float dwell_angle = dwell_time_sec * deg_per_second;
    dwell_start_angle = ignition_angle_btdc + dwell_angle;

    timerAlarmWrite(engine_timer, 1, true); // Прерывание каждую микросекунду
    timerAlarmEnable(engine_timer);
}

bool engine_simulator_set_pattern(const char* name) {
    for (int i = 0; i < num_patterns; i++) {
        if (strcmp(patterns[i].name, name) == 0) {
            current_pattern = &patterns[i];
            engine_simulator_set_rpm(current_rpm); // Пересчитать тайминги
            return true;
        }
    }
    return false;
}

void engine_simulator_set_dwell_time(float ms) {
    dwell_time_ms = ms;
    engine_simulator_set_rpm(current_rpm); // Пересчитать тайминги
}

void engine_simulator_set_ignition_angle(int degrees) {
    ignition_angle_btdc = degrees;
    engine_simulator_set_rpm(current_rpm); // Пересчитать тайминги
}

// --- Функции получения статуса ---
const char* engine_simulator_get_current_pattern_name() { return current_pattern->name; }
int engine_simulator_get_current_rpm() { return current_rpm; }
float engine_simulator_get_current_dwell_time() { return dwell_time_ms; }
int engine_simulator_get_current_ignition_angle() { return ignition_angle_btdc; }


// --- Главная функция прерывания (ISR) ---
void IRAM_ATTR onEngineTimer() {
    // 1. Обновляем текущий угол
    current_angle_deg += angle_per_tick;
    if (current_angle_deg >= DEGREES_PER_REVOLUTION) {
        current_angle_deg -= DEGREES_PER_REVOLUTION;
    }

    // 2. Управляем сигналом ДПКВ
    float tooth_angle = (float)DEGREES_PER_REVOLUTION / current_pattern->total_teeth;
    float current_tooth_index = current_angle_deg / tooth_angle;

    // Зона пропущенных зубьев
    float missing_teeth_start_index = current_pattern->total_teeth - current_pattern->missing_teeth;

    if (current_tooth_index >= missing_teeth_start_index) {
        digitalWrite(crank_pin, LOW);
    } else {
        // Сигнал 50/50 - безопасная версия без fmod()
        int current_tooth_int = (int)current_tooth_index;
        float start_of_this_tooth_angle = current_tooth_int * tooth_angle;
        float phase_in_tooth = current_angle_deg - start_of_this_tooth_angle;
        digitalWrite(crank_pin, phase_in_tooth < (tooth_angle / 2.0f));
    }

    // 3. Управляем зажиганием
    // Угол ВМТ (TDC) для первого цилиндра условно на 0/360 градусах
    // Начало накопления (Dwell Start)
    if (current_angle_deg >= dwell_start_angle && current_angle_deg < dwell_start_angle + angle_per_tick && !is_dwelling) {
        digitalWrite(ignition_pin, HIGH);
        is_dwelling = true;
    }
    // Конец накопления (Искра)
    if (current_angle_deg >= ignition_angle_btdc && current_angle_deg < ignition_angle_btdc + angle_per_tick && is_dwelling) {
        digitalWrite(ignition_pin, LOW);
        is_dwelling = false;
    }
}
