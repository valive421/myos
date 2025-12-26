#include "ne2000.h"
#include "stdio.h"
#include "x86.h"

static uint8_t mac_address[6];

void ne2000_init(void)
{
    // Reset the card
    uint8_t val = x86_inb(NE2000_BASE + NE_RESET);
    x86_outb(NE2000_BASE + NE_RESET, val);
    
    // Wait a bit
    for(volatile int i = 0; i < 1000; i++);
    
    // Stop the NIC
    x86_outb(NE2000_BASE + NE_CMD, NE_CMD_STOP);
    
    // Initialize MAC address (hardcoded for simplicity)
    mac_address[0] = 0x52;
    mac_address[1] = 0x54;
    mac_address[2] = 0x00;
    mac_address[3] = 0x12;
    mac_address[4] = 0x34;
    mac_address[5] = 0x56;
    
    // Start the NIC
    x86_outb(NE2000_BASE + NE_CMD, NE_CMD_START);
    
    puts("NE2000 network card initialized\r\n");
}

int ne2000_send(uint8_t* data, uint16_t length)
{
    // Simplified send - in real implementation would:
    // 1. Set up DMA
    // 2. Copy data to NIC memory
    // 3. Trigger transmission
    // For now, just return success
    return length;
}

int ne2000_receive(uint8_t* buffer, uint16_t max_length)
{
    // Simplified receive - in real implementation would:
    // 1. Check for received packets
    // 2. Copy from NIC memory via DMA
    // 3. Return packet data
    // For now, return 0 (no packets)
    return 0;
}
