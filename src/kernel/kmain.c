#include "stdint.h"
#include "stdio.h"
#include "network.h"
#include "browser.h"

void _cdecl cstart_(void)
{
    // Clear screen would go here in a real implementation
    
    // Print banner
    puts("\r\n");
    puts("========================================\r\n");
    puts("       MyOS - Operating System          \r\n");
    puts("    With Networking & Browser Support   \r\n");
    puts("========================================\r\n");
    puts("\r\n");
    
    // Initialize networking
    network_init();
    
    puts("\r\n");
    puts("Starting web browser...\r\n");
    
    // Run the browser
    browser_run();
    
    puts("\r\n");
    puts("========================================\r\n");
    puts("System ready. Halting...\r\n");
    puts("========================================\r\n");
    
    // Halt the system
    for (;;);
}
