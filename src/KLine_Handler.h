#ifndef KLINE_HANDLER_H
#define KLINE_HANDLER_H

#include <Arduino.h>

#define KLINE_MAX_MSG_LEN 16

void kline_handler_init();
int kline_handler_read(uint8_t* buffer, int max_len);
void kline_handler_write(const uint8_t* buffer, int len);

#endif // KLINE_HANDLER_H
