#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "CrankSignal.h"
#include "Display.h"
#include "Input.h"

// --- Конфигурация ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Пины ---
#define CRANK_SIGNAL_PIN 22
#define IGNITION_PIN 23
#define RELAY_PIN 25
#define BTN_UP_PIN 13
#define BTN_DOWN_PIN 14
#define BTN_SELECT_PIN 15

// --- Конфигурация PWM ---
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

  // --- Настройка веб-сервера ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/index.html", "text/html"); });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/style.css", "text/css"); });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(LittleFS, "/app.js", "text/javascript"); });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Собираем JSON вручную, чтобы избежать переполнения стека
    String json = "{";
    json += "\"gpio\":{";
    for (int i = 0; i < num_gpio_pins; i++) {
        json += "\"" + String(gpio_pins[i].pin) + "\":" + (gpio_pins[i].state ? "true" : "false");
        if (i < num_gpio_pins - 1) json += ",";
    }
    json += "},";
    json += "\"generator\":{";
    json += "\"rpm\":" + String(engine_simulator_get_current_rpm()) + ",";
    json += "\"pattern\":\"" + String(engine_simulator_get_current_pattern_name()) + "\"";
    json += "},";
    json += "\"ignition\":{";
    json += "\"dwell\":" + String(engine_simulator_get_current_dwell_time_ms(), 1) + ",";
    json += "\"angle\":" + String(engine_simulator_get_current_ignition_angle_btdc());
    json += "},";
    json += "\"relay\":{";
    const char* mode_str = (current_relay_mode == RELAY_OFF) ? "off" : ((current_relay_mode == RELAY_ON) ? "on" : "pwm");
    json += "\"mode\":\"" + String(mode_str) + "\",";
    json += "\"pwm_duty\":" + String(current_pwm_duty);
    json += "}";
    json += "}";
    request->send(200, "application/json", json);
  });

  // ... (остальные обработчики без изменений) ...
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });
  server.on("/set_rpm", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });
  server.on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });
  server.on("/set_ignition_params", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });
  server.on("/set_relay_mode", HTTP_GET, [](AsyncWebServerRequest *r){ /* ... */ });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() { /* ... */ }
void update_relay_state() { /* ... */ }
