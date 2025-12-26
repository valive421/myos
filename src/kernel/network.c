#include "network.h"
#include "stdio.h"
#include "ne2000.h"

static uint8_t my_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint32_t my_ip = 0x0A000202; // 10.0.2.2

void network_init(void)
{
    puts("Initializing network...\r\n");
    ne2000_init();
    puts("Network initialized!\r\n");
}

int network_send_packet(uint8_t* data, uint16_t length)
{
    return ne2000_send(data, length);
}

int network_receive_packet(uint8_t* buffer, uint16_t max_length)
{
    return ne2000_receive(buffer, max_length);
}

// Simplified TCP - just mock implementation for demonstration
static int next_socket = 1;

int tcp_connect(uint32_t ip, uint16_t port)
{
    // In a real implementation, this would:
    // 1. Create TCP SYN packet
    // 2. Send it via network
    // 3. Wait for SYN-ACK
    // 4. Send ACK
    // For now, just return a socket descriptor
    return next_socket++;
}

int tcp_send(int socket, const char* data, uint16_t length)
{
    // In a real implementation, this would:
    // 1. Create TCP packet with data
    // 2. Send it via network
    // 3. Handle ACKs
    // For now, just mock
    return length;
}

int tcp_receive(int socket, char* buffer, uint16_t max_length)
{
    // In a real implementation, this would:
    // 1. Wait for incoming packets
    // 2. Reassemble TCP stream
    // 3. Return data
    // For now, just mock
    return 0;
}

void tcp_close(int socket)
{
    // In a real implementation, this would:
    // 1. Send FIN packet
    // 2. Wait for FIN-ACK
    // 3. Clean up socket
}
