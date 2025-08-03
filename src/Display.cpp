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

// Перечисление для экранов меню
enum MenuScreen {
    SCREEN_BOOT,
    SCREEN_MAIN_STATUS,
    SCREEN_GPIO_STATUS
};
static MenuScreen current_screen = SCREEN_BOOT;

// --- Прототипы функций отрисовки ---
void draw_boot_screen();
void draw_main_status_screen();
void draw_gpio_status_screen();


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


// --- Функции отрисовки экранов ---

void draw_boot_screen() {
    u8g2.drawStr(10, 22, "ECU Simulator");
    u8g2.drawStr(45, 31, "v2.0");
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
    u8g2.drawStr(0, 32, "-> GPIO Status");
}

void draw_gpio_status_screen() {
    u8g2.setFont(u8g2_font_profont11_tf);
    u8g2.drawStr(0, 10, "GPIOs: [1] [2] [3] [4]");
    // Здесь будет логика отображения состояния пинов

    // Подсказка
    u8g2.drawStr(0, 32, "-> Main Status");
}
