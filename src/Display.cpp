#include "Display.h"
#include <U8g2lib.h>

// --- Конфигурация ---
// Используем стандартные пины I2C для ESP32: SDA=21, SCL=22
// Если у вас другие, их нужно будет указать в конструкторе.
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

// --- Переменные состояния дисплея ---
static int current_rpm = 0;
static bool wifi_connected = false;
static String ip_address = "N/A";
static const GpioPin* gpio_pins_ptr = NULL;
static int num_gpio_pins = 0;

// --- Log Buffer ---
#define LOG_BUFFER_SIZE 4
static String log_buffer[LOG_BUFFER_SIZE];
static int log_buffer_index = 0;
static bool log_buffer_full = false;

// Перечисление для экранов меню
enum MenuScreen {
    SCREEN_BOOT,
    SCREEN_MAIN_STATUS,
    SCREEN_GPIO_STATUS,
    SCREEN_LOG
};
static MenuScreen current_screen = SCREEN_BOOT;

// --- Прототипы функций отрисовки ---
void draw_boot_screen();
void draw_main_status_screen();
void draw_gpio_status_screen();
void draw_log_screen();


// --- Реализация публичных функций ---

void display_init() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr); // Выбираем шрифт
}

void display_update() {
    // Неблокирующая отрисовка. U8g2 буферизован, поэтому это быстро.
    u8g2.firstPage();
    do {
        switch (current_screen) {
            case SCREEN_BOOT:
                draw_boot_screen();
                break;
            case SCREEN_MAIN_STATUS:
                draw_main_status_screen();
                break;
            case SCREEN_GPIO_STATUS:
                draw_gpio_status_screen();
                break;
            case SCREEN_LOG:
                draw_log_screen();
                break;
        }
    } while (u8g2.nextPage());

    // Временно: после загрузочного экрана переходим на главный
    if (current_screen == SCREEN_BOOT) {
        delay(2000); // Показываем сплэш-скрин 2 секунды
        current_screen = SCREEN_MAIN_STATUS;
    }
}

void display_set_rpm(int rpm) {
    current_rpm = rpm;
}

void display_set_wifi_status(bool connected, String ip_addr) {
    wifi_connected = connected;
    ip_address = ip_addr;
}

void display_add_log(String message) {
    log_buffer[log_buffer_index] = message;
    log_buffer_index++;
    if (log_buffer_index >= LOG_BUFFER_SIZE) {
        log_buffer_index = 0;
        log_buffer_full = true;
    }
}

void display_set_gpio_pins(const GpioPin* pins, int count) {
    gpio_pins_ptr = pins;
    num_gpio_pins = count;
}


// --- Функции отрисовки экранов ---

void draw_boot_screen() {
    u8g2.drawStr(10, 22, "ECU Simulator");
    u8g2.drawStr(45, 28, "v2.0");
}

void draw_main_status_screen() {
    u8g2.setFont(u8g2_font_profont12_tf);

    // RPM
    char rpm_buf[16];
    sprintf(rpm_buf, "RPM: %d", current_rpm);
    u8g2.drawStr(0, 10, rpm_buf);

    // Wi-Fi Status
    if (wifi_connected) {
        u8g2.drawStr(0, 22, ip_address.c_str());
    } else {
        u8g2.drawStr(0, 22, "WiFi: Disconnected");
    }

    // Подсказка
    u8g2.drawStr(0, 28, "-> Next");
}

void draw_gpio_status_screen() {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.drawStr(0, 10, "GPIO Status:");

    if (gpio_pins_ptr != NULL) {
        char pin_buf[32];
        for (int i = 0; i < num_gpio_pins; i++) {
            sprintf(pin_buf, "%s: %s", gpio_pins_ptr[i].name, gpio_pins_ptr[i].state ? "ON" : "OFF");
            // Display 2 pins per line
            u8g2.drawStr((i % 2) * 64, 20 + (i / 2) * 10, pin_buf);
        }
    } else {
        u8g2.drawStr(0, 20, "No data");
    }

    // Подсказка
    u8g2.drawStr(0, 28, "-> Next");
}

void draw_log_screen() {
    u8g2.setFont(u8g2_font_profont10_tf);
    u8g2.drawStr(0, 8, "--- Log ---");

    int start_index = log_buffer_full ? log_buffer_index : 0;
    int count = log_buffer_full ? LOG_BUFFER_SIZE : log_buffer_index;

    for (int i = 0; i < count; i++) {
        int buffer_i = (start_index + i) % LOG_BUFFER_SIZE;
        u8g2.drawStr(0, 18 + i * 8, log_buffer[buffer_i].c_str());
    }

    u8g2.drawStr(0, 28, "-> Next");
}

void display_next_screen() {
    if (current_screen == SCREEN_MAIN_STATUS) {
        current_screen = SCREEN_GPIO_STATUS;
    } else if (current_screen == SCREEN_GPIO_STATUS) {
        current_screen = SCREEN_LOG;
    } else if (current_screen == SCREEN_LOG) {
        current_screen = SCREEN_MAIN_STATUS;
    }
}
