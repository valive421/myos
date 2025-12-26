# MyOS Quick Start Guide

## What is MyOS?

MyOS is an educational operating system featuring:
- **Networking**: NE2000 network card support
- **Web Browser**: Text-based browser with HTML rendering
- **Real Mode x86**: Classic 16-bit OS development

## Quick Start (5 minutes)

### 1. Prerequisites

Install required tools:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install nasm mtools dosfstools qemu-system-x86

# For full build support, also install Watcom C Compiler
# Download from: https://github.com/open-watcom/open-watcom-v2/releases
```

### 2. Clone & Build

```bash
# Clone the repository
git clone https://github.com/valive421/myos.git
cd myos

# Build the OS (if you have Watcom installed)
make clean
make
```

### 3. Run the Browser

```bash
# Run in QEMU with network support
./run.sh

# Or manually:
qemu-system-i386 -fda build/main_floppy.img -boot a -m 32M -nic user,model=ne2000
```

### 4. What You'll See

The OS will automatically:
1. Boot from floppy disk image
2. Load the kernel
3. Initialize the NE2000 network driver
4. Start the web browser
5. Display demo web pages with rendered HTML

## Current Status

### ✅ Implemented Features

- **Bootloader**: 2-stage bootloader with FAT12 support
- **Network Driver**: NE2000 driver initialization
- **Network Stack**: TCP/IP infrastructure (demo mode)
- **HTTP Client**: URL parsing and request building
- **HTML Renderer**: Text-based rendering of basic HTML tags
- **Browser UI**: Clean text interface with formatted output

### 📝 Demo Mode

The browser currently runs in demonstration mode:
- Automatically navigates to example URLs
- Shows mock HTTP responses
- Demonstrates HTML rendering
- Displays all browser features

### 🚧 Future Work (Real Implementation)

For a fully functional browser, these would be needed:
- Real network packet transmission
- Actual DNS resolution
- True TCP/IP handshaking
- Live HTTP communication
- Interactive URL input

## File Structure

```
myos/
├── src/
│   ├── bootloader/
│   │   ├── stage1/boot.asm       # MBR bootloader
│   │   └── stage2/main.c         # FAT12 kernel loader
│   └── kernel/
│       ├── kmain.c               # Kernel entry point
│       ├── browser.c/h           # Web browser
│       ├── http.c/h              # HTTP client
│       ├── network.c/h           # Network stack
│       ├── ne2000.c/h            # NIC driver
│       └── [support files]       # stdio, string, x86
├── build/                        # Build output (created)
├── README.md                     # Full documentation
├── ARCHITECTURE.md               # System diagrams
├── BROWSER_FEATURES.md           # Browser details
├── EXPECTED_OUTPUT.md            # What you'll see
└── test_structure.sh             # Verify structure
```

## Testing

### Verify File Structure

```bash
./test_structure.sh
```

Should output: `✓ All files present!`

### Build Status

If you have all build tools:
```bash
make clean && make
```

Creates: `build/main_floppy.img`

### Run Tests

```bash
# Test in QEMU (recommended)
./run.sh

# Test in Bochs (if installed)
bochs -f bochs_config
```

## Understanding the Code

### Key Components

1. **Network Initialization** (`src/kernel/network.c`):
   ```c
   void network_init(void)
   {
       puts("Initializing network...\r\n");
       ne2000_init();
       puts("Network initialized!\r\n");
   }
   ```

2. **Browser Navigation** (`src/kernel/browser.c`):
   ```c
   void browser_navigate(const char* url)
   {
       http_response_t response;
       http_get(url, &response);
       browser_display_page(&response);
   }
   ```

3. **HTML Rendering** (`src/kernel/browser.c`):
   ```c
   void browser_render_html(const char* html)
   {
       // Parses HTML and converts to text
       // <h1> → === Text ===
       // <p>  → New paragraph
       // <li> → * List item
   }
   ```

### Extending the Browser

To add a new HTML tag:

Edit `src/kernel/browser.c`:
```c
} else if(strncmp(ptr, "<b>", 3) == 0) {
    puts("[BOLD]");
} else if(strncmp(ptr, "</b>", 4) == 0) {
    puts("[/BOLD]");
}
```

Rebuild and test!

## Learning Resources

### Read These Files

1. `README.md` - Complete project documentation
2. `ARCHITECTURE.md` - System design and flow charts
3. `BROWSER_FEATURES.md` - Browser capabilities
4. `EXPECTED_OUTPUT.md` - What the browser displays

### Explore the Code

Start with these files in order:
1. `src/kernel/kmain.c` - See how it all starts
2. `src/kernel/browser.c` - Understand the browser
3. `src/kernel/http.c` - See URL and HTTP handling
4. `src/kernel/network.c` - Network stack overview
5. `src/kernel/ne2000.c` - Hardware driver

## Troubleshooting

### Build Fails - "nasm: command not found"

```bash
sudo apt-get install nasm
```

### Build Fails - "wcc: command not found"

You need Watcom C Compiler. Download from:
https://github.com/open-watcom/open-watcom-v2/releases

Install to `/usr/bin/watcom/` or update paths in makefiles.

### QEMU Not Found

```bash
sudo apt-get install qemu-system-x86
```

### Bochs Not Found

```bash
sudo apt-get install bochs bochs-x
```

## Example Session

```bash
# Start fresh
cd myos
make clean

# Build (if tools available)
make

# Run the browser
./run.sh
```

Expected output:
```
========================================
       MyOS - Operating System          
    With Networking & Browser Support   
========================================

Initializing network...
NE2000 network card initialized
Network initialized!

Starting web browser...

[Browser displays demo pages...]
```

## Next Steps

1. **Read the docs**: Check out all the .md files
2. **Explore the code**: Start with kmain.c
3. **Modify something**: Add a new HTML tag
4. **Rebuild and test**: See your changes
5. **Learn more**: Study OS development concepts

## Contributing

This is an educational project. Feel free to:
- Add more HTML tags
- Implement real networking
- Add graphics mode support
- Create new features

## Getting Help

Check these resources:
- `README.md` - Full documentation
- `ARCHITECTURE.md` - System design
- Source code comments

## Have Fun!

This OS demonstrates:
- Bootloader development
- Driver programming
- Network protocols
- Application development
- Low-level systems programming

Explore, learn, and enjoy! 🚀
