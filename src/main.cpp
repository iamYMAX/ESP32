#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "CrankSignal.h"

// --- Конфигурация ---
// ЗАМЕНИТЕ НА ВАШИ ДАННЫЕ
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Пины ---
// Управление GPIO
#define LED_BUILTIN 2
#define PIN1 18
#define PIN2 19
#define PIN3 21
// Выход сигнала ДПКВ
#define CRANK_SIGNAL_PIN 22


// --- Глобальные объекты ---
struct GpioPin {
  const uint8_t pin;
  const char* name;
  bool state;
};

GpioPin gpio_pins[] = {
  {LED_BUILTIN, "Built-in LED", false},
  {PIN1, "GPIO 18", false},
  {PIN2, "GPIO 19", false},
  {PIN3, "GPIO 21", false}
};
const int num_gpio_pins = sizeof(gpio_pins) / sizeof(gpio_pins[0]);

AsyncWebServer server(80);


// --- Функции инициализации ---
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("FATAL: LittleFS Mount Failed. Check partition scheme.");
    return;
  }
  Serial.println("LittleFS mounted successfully");
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void updateGpioPin(int pinIndex) {
    digitalWrite(gpio_pins[pinIndex].pin, gpio_pins[pinIndex].state);
}


void setup() {
  Serial.begin(115200);

  // Инициализация GPIO
  for (int i = 0; i < num_gpio_pins; i++) {
    pinMode(gpio_pins[i].pin, OUTPUT);
    updateGpioPin(i);
  }

  // Инициализация генератора ДПКВ
  crank_signal_init(CRANK_SIGNAL_PIN);

  initLittleFS();
  initWiFi();

  // --- Настройка веб-сервера ---

  // Раздача статичных файлов
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/app.js", "text/javascript");
  });

  // API: Получение полного статуса устройства
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<512> doc;
    // Статус GPIO
    JsonObject gpio = doc.createNestedObject("gpio");
    for (int i = 0; i < num_gpio_pins; i++) {
      gpio[String(gpio_pins[i].pin)] = gpio_pins[i].state;
    }
    // Статус генератора
    JsonObject generator = doc.createNestedObject("generator");
    generator["rpm"] = crank_signal_get_current_rpm();
    generator["pattern"] = crank_signal_get_current_pattern_name();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // API: Переключение GPIO
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pinNumber = request->getParam("pin")->value().toInt();
      bool pinState = request->getParam("state")->value().toInt() == 1;
      for (int i = 0; i < num_gpio_pins; i++) {
        if (gpio_pins[i].pin == pinNumber) {
          gpio_pins[i].state = pinState;
          updateGpioPin(i);
          request->send(200, "text/plain", "OK");
          return;
        }
      }
    }
    request->send(400, "text/plain", "Bad Request");
  });

  // API: Установка оборотов
  server.on("/set_rpm", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      int rpm = request->getParam("value")->value().toInt();
      crank_signal_set_rpm(rpm);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing value");
    }
  });

  // API: Установка паттерна
  server.on("/set_pattern", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pattern")) {
      const char* pattern_name = request->getParam("pattern")->value().c_str();
      if (crank_signal_set_pattern(pattern_name)) {
        request->send(200, "text/plain", "OK");
      } else {
        request->send(400, "text/plain", "Pattern not found");
      }
    } else {
      request->send(400, "text/plain", "Missing pattern");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Пусто, все работает на прерываниях и асинхронных задачах
}
