#pragma once
#include "stdint.h"

// Ethernet frame
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t payload[1500];
} __attribute__((packed)) ethernet_frame_t;

// IP header
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

// TCP header
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// Network functions
void network_init(void);
int network_send_packet(uint8_t* data, uint16_t length);
int network_receive_packet(uint8_t* buffer, uint16_t max_length);
int tcp_connect(uint32_t ip, uint16_t port);
int tcp_send(int socket, const char* data, uint16_t length);
int tcp_receive(int socket, char* buffer, uint16_t max_length);
void tcp_close(int socket);
