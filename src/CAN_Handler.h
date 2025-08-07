#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>

struct CAN_Message {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
};

void can_handler_init();
bool can_handler_read(CAN_Message &msg);
void can_handler_write(const CAN_Message &msg);

#endif // CAN_HANDLER_H
