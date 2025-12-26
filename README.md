# MyOS - Operating System with Networking and Browser Support

A simple x86 operating system with networking capabilities and a text-based web browser.

## Features

### Core System
- **Bootloader**: Two-stage bootloader with FAT12 filesystem support
- **Kernel**: C-based kernel with assembly support
- **Real Mode**: 16-bit real mode operation

### Networking
- **Network Driver**: NE2000 network card driver support (compatible with QEMU/Bochs emulators)
- **Network Stack**: Basic networking infrastructure with:
  - Ethernet frame handling
  - IP packet support
  - TCP connection management
  - Packet send/receive functions

### Web Browser
- **HTTP Client**: Simple HTTP/1.0 client implementation
- **URL Parser**: Parses HTTP/HTTPS URLs
- **HTML Renderer**: Text-based HTML rendering engine
  - Supports basic tags: `<h1>`, `<p>`, `<li>`, `<br>`
  - Strips HTML tags for clean text output
  - Formats content for terminal display
- **Browser UI**: Simple text-based browser interface

## Architecture

```
MyOS
├── Bootloader (Stage 1) - MBR bootloader, loads stage 2
├── Bootloader (Stage 2) - Loads kernel from FAT12 filesystem
└── Kernel
    ├── Network Layer
    │   ├── NE2000 Driver (ne2000.c/h)
    │   ├── Network Stack (network.c/h)
    │   └── TCP/IP Support
    ├── HTTP Layer (http.c/h)
    │   ├── HTTP GET requests
    │   └── Response parsing
    ├── Browser (browser.c/h)
    │   ├── HTML rendering
    │   └── Page display
    └── System Libraries
        ├── stdio (stdio.c/h)
        ├── string (string.c/h)
        └── x86 utilities (x86.asm/h)
```

## Building

### Prerequisites

You need the following tools to build MyOS:

1. **NASM** (Netwide Assembler)
   ```bash
   sudo apt-get install nasm
   ```

2. **Watcom C Compiler** (for 16-bit code)
   - Download from: https://github.com/open-watcom/open-watcom-v2/releases
   - Install to `/usr/bin/watcom/` or update paths in makefiles
   - Alternative: Use the official package or build from source

3. **Standard utilities**
   ```bash
   sudo apt-get install mtools dosfstools
   ```

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/valive421/myos.git
   cd myos
   ```

2. Build the OS:
   ```bash
   make clean
   make
   ```

   This will create `build/main_floppy.img` - a bootable floppy disk image.

## Running

### Using QEMU

```bash
qemu-system-i386 -fda build/main_floppy.img -boot a -m 32M -nic user,model=ne2000
```

### Using Bochs

```bash
bochs -f bochs_config
```

Or use the provided run script:
```bash
./run.sh
```

## Usage

When you boot MyOS, it will:

1. Display the MyOS banner
2. Initialize the network driver (NE2000)
3. Automatically launch the web browser in demo mode
4. Display sample web pages with rendered HTML content

The browser demonstrates:
- Network initialization
- HTTP request handling
- HTML parsing and rendering
- Text-based web page display

## Components

### Network Driver (NE2000)
Located in `src/kernel/ne2000.c`:
- Initializes the NE2000 network interface card
- Provides packet send/receive functions
- Compatible with QEMU and Bochs emulators

### Network Stack
Located in `src/kernel/network.c`:
- Basic TCP/IP implementation
- Socket-like interface for connections
- IP address and MAC address handling

### HTTP Client
Located in `src/kernel/http.c`:
- Parses URLs (http:// and https://)
- Builds HTTP GET requests
- Parses HTTP responses
- Mock implementation for demonstration

### Web Browser
Located in `src/kernel/browser.c`:
- Text-based HTML renderer
- Removes HTML tags and formats content
- Displays web pages in terminal
- Demo mode with sample pages

## Development

### File Structure

```
myos/
├── src/
│   ├── bootloader/
│   │   ├── stage1/          # MBR bootloader
│   │   └── stage2/          # Second stage loader
│   └── kernel/
│       ├── kmain.c          # Kernel entry point
│       ├── entry.asm        # Assembly entry
│       ├── network.c/h      # Network stack
│       ├── ne2000.c/h       # NE2000 driver
│       ├── http.c/h         # HTTP client
│       ├── browser.c/h      # Web browser
│       ├── stdio.c/h        # Standard I/O
│       ├── string.c/h       # String utilities
│       └── x86.asm/h        # x86 utilities
├── build/                   # Build output (ignored)
└── makefile                 # Main build file
```

### Adding Features

To extend the browser or networking:

1. **Network protocols**: Add to `src/kernel/network.c`
2. **HTML tags**: Update `browser_render_html()` in `src/kernel/browser.c`
3. **Network drivers**: Create new driver files following `ne2000.c` pattern
4. **HTTP methods**: Extend `src/kernel/http.c`

## Limitations

This is a demonstration/educational OS with the following limitations:

- 16-bit real mode only (no protected mode)
- Text mode display only
- Simplified network stack (mock TCP/IP for demo)
- Limited HTML rendering (text tags only)
- Single-tasking
- No filesystem writes (read-only FAT12)
- No security features

## Future Enhancements

Potential improvements:

- [ ] Full TCP/IP stack implementation
- [ ] Real DNS resolution
- [ ] Actual HTTP networking (currently mocked)
- [ ] More HTML tags support
- [ ] CSS styling (color support)
- [ ] Image rendering (in graphics mode)
- [ ] Keyboard input for URL entry
- [ ] Bookmarks and history
- [ ] Multiple tabs
- [ ] HTTPS/TLS support

## License

This project is provided as-is for educational purposes.

## Credits

Developed as a demonstration of OS development concepts including:
- Bootloader development
- Real-mode x86 programming
- Network driver implementation
- Protocol stack development
- Application development in constrained environments
