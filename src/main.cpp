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
struct GpioPin {
  const uint8_t pin;
  const char* name;
  bool state;
};
GpioPin gpio_pins[] = {
  {LED_BUILTIN, "Built-in LED", false}, {PIN1, "GPIO 18", false},
  {PIN2, "GPIO 19", false}, {PIN3, "GPIO 21", false}
};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

enum RelayMode { RELAY_OFF, RELAY_ON, RELAY_PWM };
RelayMode current_relay_mode = RELAY_OFF;
int current_pwm_duty = 50;

AsyncWebServer server(80);

// --- Прототипы ---
void update_relay_state();

// --- Инициализация ---
void setup() {
  Serial.begin(115200);

  // GPIO
  for (int i = 0; i < num_gpio_pins; i++) {
    pinMode(gpio_pins[i].pin, OUTPUT);
    digitalWrite(gpio_pins[i].pin, LOW);
  }

  // Реле
  pinMode(RELAY_PIN, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(RELAY_PIN, PWM_CHANNEL);
  update_relay_state();

  // Симулятор двигателя
  engine_simulator_init(CRANK_SIGNAL_PIN, IGNITION_PIN);

  // WiFi и FS
  if (LittleFS.begin(true)) { Serial.println("LittleFS OK"); } else { Serial.println("LittleFS FAIL"); }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());

  // --- Настройка веб-сервера ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/index.html", "text/html"); });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/style.css", "text/css"); });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/app.js", "text/javascript"); });

  // API: Статус
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<1024> doc;
    JsonObject gpio = doc.createNestedObject("gpio");
    for (int i = 0; i < num_gpio_pins; i++) gpio[String(gpio_pins[i].pin)] = gpio_pins[i].state;

    JsonObject generator = doc.createNestedObject("generator");
    generator["rpm"] = engine_simulator_get_current_rpm();
    generator["pattern"] = engine_simulator_get_current_pattern_name();

    JsonObject ignition = doc.createNestedObject("ignition");
    ignition["dwell"] = engine_simulator_get_current_dwell_time();
    ignition["angle"] = engine_simulator_get_current_ignition_angle();

    JsonObject relay = doc.createNestedObject("relay");
    relay["mode"] = (current_relay_mode == RELAY_OFF) ? "off" : ((current_relay_mode == RELAY_ON) ? "on" : "pwm");
    relay["pwm_duty"] = current_pwm_duty;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // API: Управление
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });
  server.on("/set_rpm", HTTP_GET, [](AsyncWebServerRequest *r){ engine_simulator_set_rpm(r->getParam("value")->value().toInt()); r->send(200); });
  server.on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *r){ engine_simulator_set_pattern(r->getParam("pattern")->value().c_str()); r->send(200); });

  server.on("/set_ignition_params", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("dwell")) engine_simulator_set_dwell_time(request->getParam("dwell")->value().toFloat());
    if (request->hasParam("angle")) engine_simulator_set_ignition_angle(request->getParam("angle")->value().toInt());
    request->send(200);
  });

  server.on("/set_relay_mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("mode")) {
        String mode = request->getParam("mode")->value();
        if (mode == "off") current_relay_mode = RELAY_OFF;
        else if (mode == "on") current_relay_mode = RELAY_ON;
        else if (mode == "pwm") {
            current_relay_mode = RELAY_PWM;
            if (request->hasParam("value")) current_pwm_duty = request->getParam("value")->value().toInt();
        }
    }
    update_relay_state();
    request->send(200);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {}

// --- Вспомогательные функции ---
void update_relay_state() {
    switch(current_relay_mode) {
        case RELAY_OFF:
            ledcWrite(PWM_CHANNEL, 0);
            break;
        case RELAY_ON:
            ledcWrite(PWM_CHANNEL, 255); // 100% duty cycle
            break;
        case RELAY_PWM:
            int duty = map(current_pwm_duty, 0, 100, 0, 255);
            ledcWrite(PWM_CHANNEL, duty);
            break;
    }
}

// Тела обработчиков /toggle опущены для краткости, они остаются как в прошлых версиях.
// Реализация в /toggle:
// if (request->hasParam("pin") && request->hasParam("state")) {
//   int pinNumber = request->getParam("pin")->value().toInt();
//   bool pinState = request->getParam("state")->value().toInt() == 1;
//   for (int i = 0; i < num_gpio_pins; i++) {
//     if (gpio_pins[i].pin == pinNumber) {
//       gpio_pins[i].state = pinState;
//       digitalWrite(gpio_pins[i].pin, pinState);
//       request->send(200);
//       return;
//     }
//   }
// }
// request->send(400);
