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

// --- Состояние симулятора ---
static volatile uint16_t current_rpm = 0;
static const CrankPattern* current_pattern = &patterns[0];
static float current_dwell_ms = 3.0;
static volatile uint16_t ignition_angle_btdc = 10;

// --- Переменные, вычисляемые ЗАРАНЕЕ ---
static volatile uint32_t angle_per_tick_res = 0;
static volatile uint32_t dwell_angle_res = 0;
static volatile uint32_t tooth_angle_res = 0;
static volatile uint32_t missing_teeth_start_angle_res = 0;
static volatile uint32_t ignition_angle_res = 0;
static volatile uint32_t dwell_start_angle_res = 0;

static volatile uint32_t current_angle_res = 0;
static volatile bool is_dwelling = false;

void IRAM_ATTR onEngineTimer();
void recalculate_params();

void engine_simulator_init(uint8_t _crank_pin, uint8_t _ignition_pin) {
    crank_pin = _crank_pin;
    ignition_pin = _ignition_pin;
    pinMode(crank_pin, OUTPUT);
    pinMode(ignition_pin, OUTPUT);

    engine_timer = timerBegin(1, 80, true);
    timerAttachInterrupt(engine_timer, &onEngineTimer, true);
    timerAlarmWrite(engine_timer, TIMER_MICROS, true);

    engine_simulator_set_pattern("60-2"); // Устанавливаем начальный паттерн
    engine_simulator_set_rpm(0);
}

void recalculate_params() {
    if (current_rpm == 0) {
        angle_per_tick_res = 0;
        return;
    }
    uint64_t total_angle_per_min = (uint64_t)current_rpm * ENG_DEGREES_PER_REV;
    uint64_t total_angle_per_sec = total_angle_per_min / 60;
    angle_per_tick_res = total_angle_per_sec / ISR_FREQUENCY_HZ;

    float dwell_sec = current_dwell_ms / 1000.0f;
    dwell_angle_res = (uint32_t)(dwell_sec * total_angle_per_sec);

    tooth_angle_res = ENG_DEGREES_PER_REV / current_pattern->total_teeth;
    missing_teeth_start_angle_res = (current_pattern->total_teeth - current_pattern->missing_teeth) * tooth_angle_res;

    ignition_angle_res = ignition_angle_btdc * ENG_RESOLUTION_MULT;
    dwell_start_angle_res = ignition_angle_res + dwell_angle_res;
}

void engine_simulator_set_rpm(uint16_t rpm) {
    timerAlarmDisable(engine_timer);
    current_rpm = rpm;
    recalculate_params();
    current_angle_res = 0;
    is_dwelling = false;
    if (rpm > 0) {
        timerAlarmEnable(engine_timer);
    } else {
        digitalWrite(crank_pin, LOW);
        digitalWrite(ignition_pin, LOW);
    }
}

bool engine_simulator_set_pattern(const char* name) {
    for (int i = 0; i < num_patterns; i++) {
        if (strcmp(patterns[i].name, name) == 0) {
            timerAlarmDisable(engine_timer);
            current_pattern = &patterns[i];
            recalculate_params();
            if (current_rpm > 0) timerAlarmEnable(engine_timer);
            return true;
        }
    }
    return false;
}

void engine_simulator_set_dwell_time_ms(float ms) { current_dwell_ms = ms; recalculate_params(); }
void engine_simulator_set_ignition_angle_btdc(uint16_t degrees) { ignition_angle_btdc = degrees; recalculate_params(); }

// ... функции get ...

void IRAM_ATTR onEngineTimer() {
    current_angle_res += angle_per_tick_res;
    if (current_angle_res >= ENG_DEGREES_PER_REV) {
        current_angle_res -= ENG_DEGREES_PER_REV;
        is_dwelling = false;
    }

    if (current_angle_res >= missing_teeth_start_angle_res) {
        digitalWrite(crank_pin, LOW);
    } else {
        digitalWrite(crank_pin, (current_angle_res % tooth_angle_res) < (tooth_angle_res / 2));
    }

    if (!is_dwelling && current_angle_res >= dwell_start_angle_res) {
        digitalWrite(ignition_pin, HIGH);
        is_dwelling = true;
    }
    if (is_dwelling && current_angle_res >= ignition_angle_res) {
        digitalWrite(ignition_pin, LOW);
        is_dwelling = false;
    }
}
// ... (getters omitted for brevity)
