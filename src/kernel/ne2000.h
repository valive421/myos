#pragma once
#include "stdint.h"

// NE2000 I/O ports
#define NE2000_BASE 0x300

// NE2000 registers
#define NE_CMD      0x00
#define NE_DATAPORT 0x10
#define NE_RESET    0x1F

// NE2000 commands
#define NE_CMD_STOP     0x01
#define NE_CMD_START    0x02
#define NE_CMD_TRANSMIT 0x04

// Function declarations
void ne2000_init(void);
int ne2000_send(uint8_t* data, uint16_t length);
int ne2000_receive(uint8_t* buffer, uint16_t max_length);
