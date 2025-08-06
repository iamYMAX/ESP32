#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "CrankSignal.h"
#include "Display.h"
#include "Input.h"
#include "InjectorControl.h"

// --- Конфигурация и Пины ---
const uint8_t INJECTOR_PINS[NUM_INJECTORS] = {26, 27, 32, 33};
#define CRANK_SIGNAL_PIN 17
#define IGNITION_PIN 23
#define RELAY_PIN 25
#define BTN_UP_PIN 13
#define BTN_DOWN_PIN 14
#define BTN_SELECT_PIN 15
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// --- Глобальные объекты и состояния ---
GpioPin gpio_pins[] = {{2, "LED", false}, {18, "P18", false}, {19, "P19", false}, {12, "P12", false}};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);
enum RelayMode { RELAY_OFF, RELAY_ON, RELAY_PWM };
RelayMode current_relay_mode = RELAY_OFF;
int current_pwm_duty = 50;
AsyncWebServer *server = NULL;
AsyncWebSocket ws("/ws");

unsigned long last_display_update = 0;

// --- Logging Helper ---
void log_message(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.print(buf);
    display_add_log(String(buf));
}

void update_relay_state();

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    log_message("WebSocket client connected\n");
  } else if(type == WS_EVT_DISCONNECT){
    log_message("WebSocket client disconnected\n");
  }
}

void setup() {
  Serial.begin(115200);
  display_init();
  input_init(BTN_UP_PIN, BTN_DOWN_PIN, BTN_SELECT_PIN);
  for (int i = 0; i < num_gpio_pins; i++) { pinMode(gpio_pins[i].pin, OUTPUT); digitalWrite(gpio_pins[i].pin, gpio_pins[i].state); }
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(5, INPUT); // Set pin 5 as input for graphing
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(RELAY_PIN, PWM_CHANNEL);
  update_relay_state();
  engine_simulator_init(CRANK_SIGNAL_PIN, IGNITION_PIN);
  injector_control_init(INJECTOR_PINS); // Initialize injectors
  LittleFS.begin(true);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  // wm.resetSettings(); // Раскомментируйте для сброса настроек WiFi
  bool res = wm.autoConnect("ECU-Simulator-AP");
  if(!res) {
    log_message("Failed to connect\n");
    // ESP.restart();
  }
  else {
    log_message("WiFi Connected: %s\n", WiFi.localIP().toString().c_str());
  }

  server = new AsyncWebServer(80);
  ws.onEvent(onWsEvent);
  server->addHandler(&ws);

  // --- Веб-сервер ---
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/index.html", "text/html"); });
  server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/style.css", "text/css"); });
  server->on("/app.js", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/app.js", "text/javascript"); });

  server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"gpio\":{";
    for (int i = 0; i < num_gpio_pins; i++) {
        json += "\"" + String(gpio_pins[i].pin) + "\":" + (gpio_pins[i].state ? "true" : "false");
        if (i < num_gpio_pins - 1) json += ",";
    }
    json += "},\"generator\":{";
    json += "\"rpm\":" + String(engine_simulator_get_current_rpm()) + ",";
    json += "\"pattern\":\"" + String(engine_simulator_get_current_pattern_name()) + "\"";
    json += "},\"ignition\":{";
    json += "\"dwell\":" + String(engine_simulator_get_current_dwell_time_ms(), 1) + ",";
    json += "\"angle\":" + String(engine_simulator_get_current_ignition_angle_btdc());
    json += "},\"relay\":{";
    const char* mode_str = (current_relay_mode == RELAY_OFF) ? "off" : ((current_relay_mode == RELAY_ON) ? "on" : "pwm");
    json += "\"mode\":\"" + String(mode_str) + "\",";
    json += "\"pwm_duty\":" + String(current_pwm_duty);
    json += "},\"injectors\":[";
    const InjectorState* injectors = get_injector_states();
    for (int i = 0; i < NUM_INJECTORS; i++) {
        json += "{";
        json += "\"enabled\":" + String(injectors[i].enabled ? "true" : "false") + ",";
        json += "\"angle\":" + String(injectors[i].opening_angle_deg) + ",";
        json += "\"duration\":" + String(injectors[i].duration_ms, 2);
        json += "}";
        if (i < NUM_INJECTORS - 1) json += ",";
    }
    json += "]}";
    request->send(200, "application/json", json);
  });

  server->on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pin = request->getParam("pin")->value().toInt();
      bool state = request->getParam("state")->value().toInt() == 1;
      log_message("[WEB] Toggle GPIO %d -> %s\n", pin, state ? "ON" : "OFF");
      for (int i = 0; i < num_gpio_pins; i++) {
        if (gpio_pins[i].pin == pin) {
          gpio_pins[i].state = state; // <--- ВАЖНОЕ ИСПРАВЛЕНИЕ
          digitalWrite(gpio_pins[i].pin, state);
          request->send(200);
          return;
        }
      }
    }
    request->send(400);
  });

  server->on("/set_rpm", HTTP_GET, [](AsyncWebServerRequest *r){
    int rpm = r->getParam("value")->value().toInt();
    log_message("[WEB] Set RPM -> %d\n", rpm);
    engine_simulator_set_rpm(rpm);
    r->send(200);
  });

  server->on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *r){
    const char* p_name = r->getParam("pattern")->value().c_str();
    log_message("[WEB] Set Pattern -> %s\n", p_name);
    engine_simulator_set_pattern(p_name);
    r->send(200);
  });

  server->on("/set_ignition_params", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("dwell")) {
        float dwell = request->getParam("dwell")->value().toFloat();
        log_message("[WEB] Set Dwell -> %.1f ms\n", dwell);
        engine_simulator_set_dwell_time_ms(dwell);
    }
    if (request->hasParam("angle")) {
        int angle = request->getParam("angle")->value().toInt();
        log_message("[WEB] Set Angle -> %d deg\n", angle);
        engine_simulator_set_ignition_angle_btdc(angle);
    }
    request->send(200);
  });

  server->on("/set_relay_mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("mode")) {
        String mode = request->getParam("mode")->value();
        if (mode == "off") { current_relay_mode = RELAY_OFF; log_message("[WEB] Set Relay -> OFF\n"); }
        else if (mode == "on") { current_relay_mode = RELAY_ON; log_message("[WEB] Set Relay -> ON\n"); }
        else if (mode == "pwm") {
            current_relay_mode = RELAY_PWM;
            if (request->hasParam("value")) {
                current_pwm_duty = request->getParam("value")->value().toInt();
                log_message("[WEB] Set Relay -> PWM %d%%\n", current_pwm_duty);
            }
        }
    }
    update_relay_state();
    request->send(200);
  });

  server->on("/next_screen", HTTP_GET, [](AsyncWebServerRequest *request){
    log_message("[WEB] Next Screen\n");
    display_next_screen();
    request->send(200);
  });

  server->on("/set_injector_params", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("injector")) {
        uint8_t index = request->getParam("injector")->value().toInt();
        bool enabled = request->hasParam("enabled") ? request->getParam("enabled")->value() == "true" : false;
        uint16_t angle = request->hasParam("angle") ? request->getParam("angle")->value().toInt() : 0;
        float duration = request->hasParam("duration") ? request->getParam("duration")->value().toFloat() : 0;

        log_message("[WEB] Set Injector %d: enabled=%d, angle=%d, duration=%.2f\n", index, enabled, angle, duration);
        injector_control_set_params(index, enabled, angle, duration);
        engine_simulator_recalculate(); // Recalculate all engine params
    }
    request->send(200);
  });

  server->on("/start_injector_cleaning", HTTP_GET, [](AsyncWebServerRequest *request){
    log_message("[WEB] Start injector cleaning cycle\n");
    injector_control_start_cleaning_cycle();
    request->send(200);
  });

  server->begin();
  log_message("HTTP server started\n");
}

#define SAMPLE_BUFFER_SIZE 20
uint8_t sample_buffer[SAMPLE_BUFFER_SIZE];
int sample_index = 0;
unsigned long last_sample_time = 0;
unsigned long last_send_time = 0;

void loop() {
  ButtonAction action = input_check();
  if (action == ACTION_SELECT) {
    log_message("Button: Select\n");
    display_next_screen();
  } else if (action != ACTION_NONE) {
    log_message("Button Action: %d\n", action);
  }

  if (millis() - last_sample_time > 1) {
    sample_buffer[sample_index++] = digitalRead(5);
    last_sample_time = millis();
  }

  if (sample_index >= SAMPLE_BUFFER_SIZE) {
    if (ws.count() > 0) {
      ws.binaryAll(sample_buffer, SAMPLE_BUFFER_SIZE);
    }
    sample_index = 0;
  }

  if (millis() - last_display_update > 100) {
    display_set_rpm(engine_simulator_get_current_rpm());
    display_set_wifi_status(WiFi.status() == WL_CONNECTED, WiFi.localIP().toString());
    display_set_gpio_pins(gpio_pins, num_gpio_pins);
    display_update();
    last_display_update = millis();
  }
}

void update_relay_state() {
    switch(current_relay_mode) {
        case RELAY_OFF: ledcWrite(PWM_CHANNEL, 0); break;
        case RELAY_ON: ledcWrite(PWM_CHANNEL, 255); break;
        case RELAY_PWM: ledcWrite(PWM_CHANNEL, map(current_pwm_duty, 0, 100, 0, 255)); break;
    }
}
