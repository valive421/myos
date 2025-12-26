#pragma once
#include "http.h"

// Browser functions
void browser_init(void);
void browser_navigate(const char* url);
void browser_display_page(http_response_t* response);
void browser_render_html(const char* html);
void browser_run(void);
