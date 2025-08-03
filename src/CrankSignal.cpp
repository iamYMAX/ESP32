#include "CrankSignal.h"

// --- Переменные модуля ---

// Пин для вывода сигнала
static uint8_t signal_pin = -1;

// Указатель на таймер
static hw_timer_t *crank_timer = NULL;

// Текущие параметры работы
static volatile int current_rpm = 0;
static const CrankPattern* current_pattern = &patterns[0]; // По умолчанию 60-2
static volatile int tooth_counter = 0;

// --- Прототип ISR ---
void IRAM_ATTR onCrankTimer();

// --- Реализация функций ---

void crank_signal_init(uint8_t output_pin) {
    signal_pin = output_pin;
    pinMode(signal_pin, OUTPUT);
    digitalWrite(signal_pin, LOW);

    // Создаем и настраиваем таймер (используем таймер 0)
    // Таймер ESP32 работает на 80МГц. Делитель 80 дает нам 1МГц, т.е. 1 микросекунду на тик.
    crank_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(crank_timer, &onCrankTimer, true);

    // Начальная установка RPM (0 - выключено)
    crank_signal_set_rpm(0);
}

bool crank_signal_set_pattern(const char* pattern_name) {
    for (int i = 0; i < num_patterns; i++) {
        if (strcmp(patterns[i].name, pattern_name) == 0) {
            current_pattern = &patterns[i];
            // При смене паттерна, пересчитываем таймер с текущим RPM
            crank_signal_set_rpm(current_rpm);
            return true;
        }
    }
    return false;
}

void crank_signal_set_rpm(int rpm) {
    current_rpm = rpm;

    // Если обороты 0, выключаем таймер
    if (rpm == 0) {
        timerAlarmDisable(crank_timer);
        digitalWrite(signal_pin, LOW);
        return;
    }

    // Расчет времени одного "зуба" в микросекундах
    // (60 секунд * 1,000,000 микросекунд) / (обороты * количество зубьев)
    long us_per_tooth = (60 * 1000000L) / (rpm * current_pattern->total_teeth);

    // Время одного импульса (50% high, 50% low)
    long alarm_value = us_per_tooth / 2;

    // Устанавливаем новое значение для срабатывания таймера
    timerAlarmWrite(crank_timer, alarm_value, true); // true = auto-reload
    timerAlarmEnable(crank_timer);

    // Сбрасываем счетчик зубьев
    tooth_counter = 0;
}

const char* crank_signal_get_current_pattern_name() {
    return current_pattern->name;
}

int crank_signal_get_current_rpm() {
    return current_rpm;
}


// --- Interrupt Service Routine (ISR) ---
// Эта функция будет вызываться с высокой точностью по таймеру
void IRAM_ATTR onCrankTimer() {
    // Проверяем, не находимся ли мы в "пропущенной" зоне
    if (tooth_counter < (current_pattern->total_teeth - current_pattern->missing_teeth) * 2) {
        // Если нет, просто переключаем пин (HIGH/LOW)
        digitalWrite(signal_pin, !digitalRead(signal_pin));
    } else {
        // Если мы в зоне пропущенных зубьев, держим пин в LOW
        digitalWrite(signal_pin, LOW);
    }

    // Увеличиваем счетчик импульсов (один зуб = 2 импульса, HIGH и LOW)
    tooth_counter++;

    // Если мы завершили полный оборот, сбрасываем счетчик
    if (tooth_counter >= current_pattern->total_teeth * 2) {
        tooth_counter = 0;
    }
}
