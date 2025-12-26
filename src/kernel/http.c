#include "http.h"
#include "network.h"
#include "stdio.h"
#include "string.h"

void parse_url(const char* url, char* host, char* path, uint16_t* port)
{
    const char* ptr = url;
    
    // Skip http:// or https://
    if(strncmp(ptr, "http://", 7) == 0) {
        ptr += 7;
        *port = 80;
    } else if(strncmp(ptr, "https://", 8) == 0) {
        ptr += 8;
        *port = 443;
    } else {
        *port = 80;
    }
    
    // Extract host
    int i = 0;
    while(*ptr && *ptr != '/' && *ptr != ':' && i < 255) {
        host[i++] = *ptr++;
    }
    host[i] = '\0';
    
    // Check for custom port
    if(*ptr == ':') {
        ptr++;
        *port = 0;
        while(*ptr >= '0' && *ptr <= '9') {
            *port = *port * 10 + (*ptr - '0');
            ptr++;
        }
    }
    
    // Extract path
    if(*ptr == '/') {
        i = 0;
        while(*ptr && i < 255) {
            path[i++] = *ptr++;
        }
        path[i] = '\0';
    } else {
        path[0] = '/';
        path[1] = '\0';
    }
}

int http_get(const char* url, http_response_t* response)
{
    char host[256];
    char path[256];
    uint16_t port;
    char request[512];
    int remaining;
    
    // Parse URL
    parse_url(url, host, path, &port);
    
    // Build HTTP GET request with bounds checking
    remaining = sizeof(request) - 1;
    request[0] = '\0';
    
    // Safely build request
    strncat(request, "GET ", remaining);
    remaining -= 4;
    if (remaining > 0) {
        strncat(request, path, remaining);
        remaining -= strlen(path);
    }
    if (remaining > 0) {
        strncat(request, " HTTP/1.0\r\n", remaining);
        remaining -= 12;
    }
    if (remaining > 0) {
        strncat(request, "Host: ", remaining);
        remaining -= 6;
    }
    if (remaining > 0) {
        strncat(request, host, remaining);
        remaining -= strlen(host);
    }
    if (remaining > 0) {
        strncat(request, "\r\n", remaining);
        remaining -= 2;
    }
    if (remaining > 0) {
        strncat(request, "User-Agent: MyOS-Browser/1.0\r\n", remaining);
        remaining -= 31;
    }
    if (remaining > 0) {
        strncat(request, "Connection: close\r\n", remaining);
        remaining -= 19;
    }
    if (remaining > 0) {
        strncat(request, "\r\n", remaining);
    }
    
    // For demonstration, create a mock response
    // In a real implementation, this would:
    // 1. Resolve DNS for host
    // 2. Connect via TCP
    // 3. Send HTTP request
    // 4. Receive and parse response
    
    response->status_code = 200;
    
    // Safely build response with bounds checking
    remaining = HTTP_MAX_RESPONSE - 1;
    response->content[0] = '\0';
    
    strncat(response->content, 
           "<html>\r\n"
           "<head><title>Welcome to MyOS Browser!</title></head>\r\n"
           "<body>\r\n"
           "<h1>MyOS Browser Works!</h1>\r\n"
           "<p>This is a simple text-based web browser.</p>\r\n"
           "<p>URL requested: ", remaining);
    remaining -= strlen(response->content);
    
    if (remaining > 0) {
        // Safely append URL with length limit
        int url_len = strlen(url);
        if (url_len > remaining - 100) {  // Reserve space for closing tags
            url_len = remaining - 100;
        }
        if (url_len > 0) {
            strncat(response->content, url, url_len);
            remaining -= url_len;
        }
    }
    
    if (remaining > 0) {
        strncat(response->content, "</p>\r\n", remaining);
        remaining -= 6;
    }
    
    if (remaining > 0) {
        strncat(response->content,
               "<p>Features:</p>\r\n"
               "<ul>\r\n"
               "<li>Network driver support (NE2000)</li>\r\n"
               "<li>Basic HTTP client</li>\r\n"
               "<li>Simple HTML rendering</li>\r\n"
               "</ul>\r\n"
               "</body>\r\n"
               "</html>\r\n", remaining);
    }
    
    response->content_length = strlen(response->content);
    
    return 0;
}
