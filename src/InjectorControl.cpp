#include "InjectorControl.h"

static InjectorState injector_states[NUM_INJECTORS];
static bool cleaning_cycle_active = false;
static unsigned long cleaning_cycle_end_time = 0;

void injector_control_init(const uint8_t injector_pins[NUM_INJECTORS]) {
    for (int i = 0; i < NUM_INJECTORS; ++i) {
        injector_states[i].pin = injector_pins[i];
        injector_states[i].enabled = false;
        injector_states[i].opening_angle_deg = 0;
        injector_states[i].duration_ms = 0;
        injector_states[i].is_injecting = false;
        injector_states[i].start_angle_res = 0;
        injector_states[i].end_angle_res = 0;
        injector_states[i].duration_angle_res = 0;
        pinMode(injector_states[i].pin, OUTPUT);
        digitalWrite(injector_states[i].pin, LOW);
    }
}

void injector_control_recalculate(uint32_t total_angle_per_sec) {
    if (cleaning_cycle_active) return; // Don't recalculate during cleaning

    for (int i = 0; i < NUM_INJECTORS; ++i) {
        if (!injector_states[i].enabled) continue;

        // Calculate duration in terms of angle resolution
        float duration_sec = injector_states[i].duration_ms / 1000.0f;
        injector_states[i].duration_angle_res = (uint32_t)(duration_sec * total_angle_per_sec);

        // Calculate start angle
        injector_states[i].start_angle_res = ENG_DEGREES_PER_REV - (injector_states[i].opening_angle_deg * ENG_RESOLUTION_MULT);

        // Calculate end angle
        injector_states[i].end_angle_res = injector_states[i].start_angle_res + injector_states[i].duration_angle_res;

        // Handle wrap-around for end angle
        if (injector_states[i].end_angle_res >= ENG_DEGREES_PER_REV) {
            injector_states[i].end_angle_res -= ENG_DEGREES_PER_REV;
        }
    }
}


void IRAM_ATTR injector_control_update(uint32_t current_angle_res) {
    if (cleaning_cycle_active) {
        // Simple cleaning logic: toggle injectors every 100ms
        bool state = (millis() / 100) % 2;
        for (int i = 0; i < NUM_INJECTORS; ++i) {
            digitalWrite(injector_states[i].pin, state);
        }
        if (millis() > cleaning_cycle_end_time) {
            cleaning_cycle_active = false;
            // Ensure injectors are off after cleaning
            for (int i = 0; i < NUM_INJECTORS; ++i) {
                digitalWrite(injector_states[i].pin, LOW);
            }
        }
        return;
    }

    for (int i = 0; i < NUM_INJECTORS; ++i) {
        if (!injector_states[i].enabled) {
            digitalWrite(injector_states[i].pin, LOW);
            continue;
        }

        bool should_be_injecting = false;
        // Check if current angle is within the injection window
        if (injector_states[i].start_angle_res < injector_states[i].end_angle_res) { // No wrap-around
            if (current_angle_res >= injector_states[i].start_angle_res && current_angle_res < injector_states[i].end_angle_res) {
                should_be_injecting = true;
            }
        } else { // Wrap-around case
            if (current_angle_res >= injector_states[i].start_angle_res || current_angle_res < injector_states[i].end_angle_res) {
                should_be_injecting = true;
            }
        }
        digitalWrite(injector_states[i].pin, should_be_injecting);
    }
}

void injector_control_set_params(uint8_t injector_index, bool enabled, uint16_t angle_deg, float duration_ms) {
    if (injector_index >= NUM_INJECTORS) return;

    injector_states[injector_index].enabled = enabled;
    injector_states[injector_index].opening_angle_deg = angle_deg;
    injector_states[injector_index].duration_ms = duration_ms;

    // Note: Recalculation is not done here to avoid calling RPM-dependent functions from outside the main thread.
    // The main loop should call injector_control_recalculate() after this.
}

void injector_control_start_cleaning_cycle() {
    cleaning_cycle_active = true;
    cleaning_cycle_end_time = millis() + 5000; // Run for 5 seconds
}

const InjectorState* get_injector_states() {
    return injector_states;
}
