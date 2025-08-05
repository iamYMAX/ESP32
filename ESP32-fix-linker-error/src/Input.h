#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>

// Перечисление для действий кнопок
enum ButtonAction {
    ACTION_NONE,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_SELECT
};

// Инициализация пинов для кнопок
void input_init(uint8_t pin_up, uint8_t pin_down, uint8_t pin_select);

// Проверка состояния кнопок, вызывается в loop()
// Возвращает действие, если кнопка была нажата
ButtonAction input_check();


#endif // INPUT_H
