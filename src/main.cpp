#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "CrankSignal.h"

// --- Конфигурация ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Пины ---
#define LED_BUILTIN 2
#define PIN1 18
#define PIN2 19
#define PIN3 21
#define CRANK_SIGNAL_PIN 22
#define IGNITION_PIN 23
#define RELAY_PIN 25

// --- Конфигурация PWM ---
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// --- Глобальные объекты и состояния ---
struct GpioPin { const uint8_t pin; const char* name; bool state; };
GpioPin gpio_pins[] = {
  {LED_BUILTIN, "Built-in LED", false}, {PIN1, "GPIO 18", false},
  {PIN2, "GPIO 19", false}, {PIN3, "GPIO 21", false}
};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

enum RelayMode { RELAY_OFF, RELAY_ON, RELAY_PWM };
RelayMode current_relay_mode = RELAY_OFF;
int current_pwm_duty = 50;

AsyncWebServer server(80);

void update_relay_state();

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < num_gpio_pins; i++) {
    pinMode(gpio_pins[i].pin, OUTPUT);
    digitalWrite(gpio_pins[i].pin, LOW);
  }
  pinMode(RELAY_PIN, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(RELAY_PIN, PWM_CHANNEL);
  update_relay_state();
  engine_simulator_init(CRANK_SIGNAL_PIN, IGNITION_PIN);
  if (LittleFS.begin(true)) { Serial.println("LittleFS OK"); } else { Serial.println("LittleFS FAIL"); }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());

  // --- Настройка веб-сервера ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/index.html", "text/html"); });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/style.css", "text/css"); });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/app.js", "text/javascript"); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){ /* ... Omitted for brevity ... */ });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pin = request->getParam("pin")->value().toInt();
      bool state = request->getParam("state")->value().toInt() == 1;
      Serial.printf("[WEB] Toggle GPIO %d -> %s\n", pin, state ? "ON" : "OFF");
      for (int i = 0; i < num_gpio_pins; i++) {
        if (gpio_pins[i].pin == pin) {
          gpio_pins[i].state = state;
          digitalWrite(pin, state);
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
        engine_simulator_set_dwell_time(dwell);
    }
    if (request->hasParam("angle")) {
        int angle = request->getParam("angle")->value().toInt();
        Serial.printf("[WEB] Set Angle -> %d deg\n", angle);
        engine_simulator_set_ignition_angle(angle);
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
            } else {
                Serial.printf("[WEB] Set Relay -> PWM %d%% (no value)\n", current_pwm_duty);
            }
        }
    }
    update_relay_state();
    request->send(200);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {}

void update_relay_state() {
    switch(current_relay_mode) {
        case RELAY_OFF: ledcWrite(PWM_CHANNEL, 0); break;
        case RELAY_ON: ledcWrite(PWM_CHANNEL, 255); break;
        case RELAY_PWM: ledcWrite(PWM_CHANNEL, map(current_pwm_duty, 0, 100, 0, 255)); break;
    }
}
// Body of /status is omitted for brevity, it's unchanged.
// StaticJsonDocument<1024> doc; ... etc.
