#ifndef INJECTOR_CONTROL_H
#define INJECTOR_CONTROL_H

#include <Arduino.h>
#include "CrankSignal.h"

#define NUM_INJECTORS 4

struct InjectorState {
    uint8_t pin;
    bool enabled;
    uint16_t opening_angle_deg; // Degrees before TDC
    float duration_ms;
    // Internal state variables for the ISR
    volatile bool is_injecting;
    volatile uint32_t start_angle_res;
    volatile uint32_t end_angle_res;
    volatile uint32_t duration_angle_res;
};

void injector_control_init(const uint8_t injector_pins[NUM_INJECTORS]);
void injector_control_recalculate(uint32_t total_angle_per_sec);
void injector_control_update(uint32_t current_angle_res);
void injector_control_set_params(uint8_t injector_index, bool enabled, uint16_t angle_deg, float duration_ms);
void injector_control_start_cleaning_cycle();
const InjectorState* get_injector_states();

#endif // INJECTOR_CONTROL_H
