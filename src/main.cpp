#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "CrankSignal.h"
#include "Display.h"
#include "Input.h"

// --- Конфигурация и Пины ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
#define CRANK_SIGNAL_PIN 22
#define IGNITION_PIN 23
#define RELAY_PIN 25
#define BTN_UP_PIN 13
#define BTN_DOWN_PIN 14
#define BTN_SELECT_PIN 15
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define INJECTOR_1_PIN 4
#define INJECTOR_2_PIN 26
#define INJECTOR_3_PIN 27
#define INJECTOR_4_PIN 32

// --- Глобальные объекты и состояния ---
Injector injectors[] = {
  {INJECTOR_1_PIN, false, INJ_OFF, 10, 50},
  {INJECTOR_2_PIN, false, INJ_OFF, 10, 50},
  {INJECTOR_3_PIN, false, INJ_OFF, 10, 50},
  {INJECTOR_4_PIN, false, INJ_OFF, 10, 50}
};
const int num_injectors = sizeof(injectors) / sizeof(injectors[0]);
GpioPin gpio_pins[] = {{2, "LED", false}, {18, "P18", false}, {19, "P19", false}, {21, "P21", false}};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);
enum RelayMode { RELAY_OFF, RELAY_ON, RELAY_PWM };
RelayMode current_relay_mode = RELAY_OFF;
int current_pwm_duty = 50;
AsyncWebServer server(80);

unsigned long last_display_update = 0;

void update_relay_state();

void setup() {
  Serial.begin(115200);
  display_init();
  input_init(BTN_UP_PIN, BTN_DOWN_PIN, BTN_SELECT_PIN);
  for (int i = 0; i < num_gpio_pins; i++) { pinMode(gpio_pins[i].pin, OUTPUT); digitalWrite(gpio_pins[i].pin, gpio_pins[i].state); }
  for (int i = 0; i < num_injectors; i++) { pinMode(injectors[i].pin, OUTPUT); }
  pinMode(RELAY_PIN, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(RELAY_PIN, PWM_CHANNEL);
  update_relay_state();
  engine_simulator_init(CRANK_SIGNAL_PIN, IGNITION_PIN);
  LittleFS.begin(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());

  // --- Веб-сервер ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/index.html", "text/html"); });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/style.css", "text/css"); });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/app.js", "text/javascript"); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
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
    json += "}}";
    request->send(200, "application/json", json);
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pin = request->getParam("pin")->value().toInt();
      bool state = request->getParam("state")->value().toInt() == 1;
      Serial.printf("[WEB] Toggle GPIO %d -> %s\n", pin, state ? "ON" : "OFF");
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

  server.on("/set_rpm", HTTP_GET, [](AsyncWebServerRequest *r){
    int rpm = r->getParam("value")->value().toInt();
    Serial.printf("[WEB] Set RPM -> %d\n", rpm);
    engine_simulator_set_rpm(rpm);
    r->send(200);
  });

  server.on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *r){
    const char* p_name = r->getParam("pattern")->value().c_str();
    Serial.printf("[WEB] Set Pattern -> %s\n", p_name);
    engine_simulator_set_pattern(p_name);
    r->send(200);
  });

  server.on("/set_ignition_params", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("dwell")) {
        float dwell = request->getParam("dwell")->value().toFloat();
        Serial.printf("[WEB] Set Dwell -> %.1f ms\n", dwell);
        engine_simulator_set_dwell_time_ms(dwell);
    }
    if (request->hasParam("angle")) {
        int angle = request->getParam("angle")->value().toInt();
        Serial.printf("[WEB] Set Angle -> %d deg\n", angle);
        engine_simulator_set_ignition_angle_btdc(angle);
    }
    request->send(200);
  });

  server.on("/set_relay_mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("mode")) {
        String mode = request->getParam("mode")->value();
        if (mode == "off") { current_relay_mode = RELAY_OFF; Serial.println("[WEB] Set Relay -> OFF"); }
        else if (mode == "on") { current_relay_mode = RELAY_ON; Serial.println("[WEB] Set Relay -> ON"); }
        else if (mode == "pwm") {
            current_relay_mode = RELAY_PWM;
            if (request->hasParam("value")) {
                current_pwm_duty = request->getParam("value")->value().toInt();
                Serial.printf("[WEB] Set Relay -> PWM %d%%\n", current_pwm_duty);
            }
        }
    }
    update_relay_state();
    request->send(200);
  });

  server.on("/set_injector_state", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      int id = request->getParam("id")->value().toInt();
      if (id >= 0 && id < num_injectors) {
        if (request->hasParam("state")) {
          injectors[id].state = request->getParam("state")->value().toInt() == 1;
          digitalWrite(injectors[id].pin, injectors[id].state);
        }
        // Add other parameter handling here in the future (mode, freq, etc.)
        request->send(200);
        return;
      }
    }
    request->send(400);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  ButtonAction action = input_check();
  if (action != ACTION_NONE) {
    Serial.printf("Button Action: %d\n", action);
  }
  if (millis() - last_display_update > 100) {
    display_set_rpm(engine_simulator_get_current_rpm());
    display_set_wifi_status(WiFi.status() == WL_CONNECTED, WiFi.localIP().toString());
    display_set_injectors_state(injectors, num_injectors);
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
