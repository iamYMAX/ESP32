#include "Input.h"

// --- Переменные модуля ---
static uint8_t pin_up, pin_down, pin_select;

// Для антидребезга (debounce)
static unsigned long last_debounce_time = 0;
static unsigned long debounce_delay = 50; // 50 мс

// --- Реализация функций ---

void input_init(uint8_t _pin_up, uint8_t _pin_down, uint8_t _pin_select) {
    pin_up = _pin_up;
    pin_down = _pin_down;
    pin_select = _pin_select;

    pinMode(pin_up, INPUT_PULLUP);
    pinMode(pin_down, INPUT_PULLUP);
    pinMode(pin_select, INPUT_PULLUP);
}

ButtonAction input_check() {
    // Проверяем, прошло ли достаточно времени с последнего нажатия
    if ((millis() - last_debounce_time) < debounce_delay) {
        return ACTION_NONE;
    }

    ButtonAction action = ACTION_NONE;

    // Кнопки подключены к земле через INPUT_PULLUP, поэтому LOW = нажата
    if (digitalRead(pin_up) == LOW) {
        action = ACTION_UP;
    } else if (digitalRead(pin_down) == LOW) {
        action = ACTION_DOWN;
    } else if (digitalRead(pin_select) == LOW) {
        action = ACTION_SELECT;
    }

    if (action != ACTION_NONE) {
        last_debounce_time = millis(); // Сбрасываем таймер антидребезга
    }

    return action;
}
