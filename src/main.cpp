#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

// ЗАМЕНИТЕ НА ВАШИ ДАННЫЕ
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Пины для управления
#define LED_BUILTIN 2
#define PIN1 18
#define PIN2 19
#define PIN3 21

// Создаем объекты пинов для удобства
struct GpioPin {
  const uint8_t pin;
  const char* name;
  bool state;
};

GpioPin pins[] = {
  {LED_BUILTIN, "Built-in LED", false},
  {PIN1, "GPIO 18", false},
  {PIN2, "GPIO 19", false},
  {PIN3, "GPIO 21", false}
};

const int numPins = sizeof(pins) / sizeof(pins[0]);

// Создаем асинхронный веб-сервер на порту 80
AsyncWebServer server(80);

// Инициализация файловой системы LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Инициализация Wi-Fi
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

// Обновление состояния пина
void updatePin(int pinIndex) {
    digitalWrite(pins[pinIndex].pin, pins[pinIndex].state);
}

void setup() {
  Serial.begin(115200);

  // Инициализация пинов
  for (int i = 0; i < numPins; i++) {
    pinMode(pins[i].pin, OUTPUT);
    updatePin(i);
  }

  initLittleFS();
  initWiFi();

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

  // API для получения статуса всех пинов
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    JSONVar json;
    for (int i = 0; i < numPins; i++) {
      json[String(pins[i].pin)] = pins[i].state;
    }
    request->send(200, "application/json", JSON.stringify(json));
  });

  // API для переключения пинов
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pinNumber = request->getParam("pin")->value().toInt();
      bool pinState = request->getParam("state")->value().toInt() == 1;

      bool pinFound = false;
      for (int i = 0; i < numPins; i++) {
        if (pins[i].pin == pinNumber) {
          pins[i].state = pinState;
          updatePin(i);
          pinFound = true;
          break;
        }
      }

      if (pinFound) {
        request->send(200, "text/plain", "OK");
      } else {
        request->send(400, "text/plain", "Pin not found");
      }
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  // Запуск сервера
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Пустой цикл, так как сервер асинхронный
}
