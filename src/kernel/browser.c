#include "browser.h"
#include "http.h"
#include "stdio.h"
#include "string.h"

static char current_url[256] = "";

void browser_init(void)
{
    puts("\r\n");
    puts("=====================================\r\n");
    puts("    MyOS Web Browser v1.0\r\n");
    puts("=====================================\r\n");
    puts("\r\n");
}

void browser_render_html(const char* html)
{
    const char* ptr = html;
    int in_tag = 0;
    int in_head = 0;
    
    while(*ptr) {
        if(*ptr == '<') {
            in_tag = 1;
            // Check for special tags
            if(strncmp(ptr, "<head", 5) == 0) {
                in_head = 1;
            } else if(strncmp(ptr, "</head>", 7) == 0) {
                in_head = 0;
            } else if(strncmp(ptr, "<h1>", 4) == 0) {
                puts("\r\n=== ");
            } else if(strncmp(ptr, "</h1>", 5) == 0) {
                puts(" ===\r\n");
            } else if(strncmp(ptr, "<p>", 3) == 0) {
                puts("\r\n");
            } else if(strncmp(ptr, "</p>", 4) == 0) {
                puts("\r\n");
            } else if(strncmp(ptr, "<li>", 4) == 0) {
                puts("\r\n  * ");
            } else if(strncmp(ptr, "<br", 3) == 0) {
                puts("\r\n");
            }
        } else if(*ptr == '>') {
            in_tag = 0;
        } else if(!in_tag && !in_head) {
            putc(*ptr);
        }
        ptr++;
    }
    puts("\r\n");
}

void browser_display_page(http_response_t* response)
{
    puts("\r\n");
    puts("-------------------------------------\r\n");
    puts("Page loaded successfully!\r\n");
    puts("Status: ");
    
    // Print status code
    if(response->status_code == 200) {
        puts("200 OK\r\n");
    } else {
        puts("Error\r\n");
    }
    
    puts("-------------------------------------\r\n");
    puts("\r\n");
    
    // Render the HTML content
    browser_render_html(response->content);
    
    puts("\r\n");
    puts("-------------------------------------\r\n");
}

void browser_navigate(const char* url)
{
    http_response_t response;
    
    puts("\r\nNavigating to: ");
    puts(url);
    puts("\r\n");
    puts("Fetching page...\r\n");
    
    // Fetch the page
    if(http_get(url, &response) == 0) {
        browser_display_page(&response);
        strcpy(current_url, url);
    } else {
        puts("Failed to load page!\r\n");
    }
}

void browser_run(void)
{
    browser_init();
    
    // For demonstration, navigate to a few URLs automatically
    puts("Demo mode: Browsing example pages...\r\n\r\n");
    
    browser_navigate("http://example.com/");
    
    puts("\r\nPress any key to browse another page...\r\n");
    // In a real implementation, this would wait for keyboard input
    
    browser_navigate("http://myos.local/welcome");
    
    puts("\r\n\r\nBrowser demo complete!\r\n");
    puts("In a full implementation, you could:\r\n");
    puts("  - Enter URLs manually\r\n");
    puts("  - Click links\r\n");
    puts("  - View images (in graphics mode)\r\n");
    puts("  - Use bookmarks\r\n");
    puts("\r\n");
}
