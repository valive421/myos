#pragma once
#include "stdint.h"

#define HTTP_MAX_RESPONSE 4096

typedef struct {
    int status_code;
    char content[HTTP_MAX_RESPONSE];
    uint16_t content_length;
} http_response_t;

// HTTP client functions
int http_get(const char* url, http_response_t* response);
void parse_url(const char* url, char* host, char* path, uint16_t* port);
