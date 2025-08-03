#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "CrankSignal.h" // Assuming this is the fixed-point version
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

// --- Глобальные объекты и состояния ---
struct GpioPin { const uint8_t pin; const char* name; bool state; };
GpioPin gpio_pins[] = {{2, "LED", false}, {18, "P18", false}, {19, "P19", false}, {21, "P21", false}};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);
enum RelayMode { RELAY_OFF, RELAY_ON, RELAY_PWM };
RelayMode current_relay_mode = RELAY_OFF;
int current_pwm_duty = 50;
AsyncWebServer server(80);

void update_relay_state();

void setup() {
  Serial.begin(115200);
  display_init();
  input_init(BTN_UP_PIN, BTN_DOWN_PIN, BTN_SELECT_PIN);
  for (int i = 0; i < num_gpio_pins; i++) { pinMode(gpio_pins[i].pin, OUTPUT); }
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

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){ /* Manual JSON building */ });

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

void loop() { /* ... */ }
void update_relay_state() { /* ... */ }
// Full bodies for loop, update_relay_state, and status handler are omitted for brevity
// They are the same as the previous correct versions.
