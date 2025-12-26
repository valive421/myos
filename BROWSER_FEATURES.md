# MyOS Browser - Feature Overview

## Browser Capabilities

The MyOS browser is a text-based web browser integrated into the kernel. Here's what it can do:

### 1. Network Initialization
```
[Boot] → [Network Driver Init] → [NE2000 Ready]
```

The browser initializes the NE2000 network card driver on boot, making the system network-capable.

### 2. HTTP Request Flow
```
User URL → Parse URL → Build HTTP Request → Send via Network → Receive Response → Parse HTML → Display
```

**Example:**
- Input: `http://example.com/page.html`
- Output: Formatted text content of the page

### 3. HTML Rendering

The browser supports basic HTML tags and converts them to text:

| HTML Tag | Rendered As |
|----------|-------------|
| `<h1>Title</h1>` | `=== Title ===` |
| `<p>Text</p>` | Text with line breaks |
| `<li>Item</li>` | `* Item` |
| `<br>` | Line break |
| Other tags | Stripped (content shown) |

**Example Rendering:**

Input HTML:
```html
<html>
<head><title>Welcome</title></head>
<body>
<h1>MyOS Browser</h1>
<p>This is a paragraph.</p>
<ul>
<li>Feature 1</li>
<li>Feature 2</li>
</ul>
</body>
</html>
```

Rendered Output:
```
=== MyOS Browser ===

This is a paragraph.

  * Feature 1
  * Feature 2
```

### 4. Browser Session Example

When you boot MyOS, you'll see:

```
========================================
       MyOS - Operating System          
    With Networking & Browser Support   
========================================

Initializing network...
NE2000 network card initialized
Network initialized!

Starting web browser...

=====================================
    MyOS Web Browser v1.0
=====================================

Demo mode: Browsing example pages...

Navigating to: http://example.com/

Fetching page...

-------------------------------------
Page loaded successfully!
Status: 200 OK
-------------------------------------

=== MyOS Browser Works! ===

This is a simple text-based web browser.

URL requested: http://example.com/

Features:

  * Network driver support (NE2000)
  * Basic HTTP client
  * Simple HTML rendering

-------------------------------------

Press any key to browse another page...

[... continues with more demo pages ...]
```

## Technical Details

### Network Stack
- **Driver**: NE2000 (industry-standard, emulator-compatible)
- **Protocol**: TCP/IP (simplified for demonstration)
- **MAC Address**: 52:54:00:12:34:56
- **IP Address**: 10.0.2.2 (configurable)

### HTTP Client
- **Method**: GET
- **Version**: HTTP/1.0
- **Headers**: Host, User-Agent, Connection
- **User-Agent**: MyOS-Browser/1.0

### Supported Operations

1. **URL Parsing**
   - Protocol detection (http:// / https://)
   - Host extraction
   - Port parsing (default 80)
   - Path extraction

2. **Request Building**
   - HTTP GET method
   - Standard headers
   - Proper formatting

3. **Response Handling**
   - Status code parsing
   - Content extraction
   - HTML processing

4. **HTML Rendering**
   - Tag detection
   - Tag stripping
   - Text formatting
   - Structure preservation

## Demo Mode

The browser includes a demo mode that automatically:
1. Shows the browser banner
2. Navigates to example.com
3. Displays the rendered page
4. Navigates to another demo page
5. Shows available features

This demonstrates all browser capabilities without requiring user input.

## Architecture Benefits

### Modular Design
Each component is separate:
- `ne2000.c` - Hardware driver
- `network.c` - Protocol stack
- `http.c` - Application protocol
- `browser.c` - User interface

### Extensibility
Easy to add:
- New network drivers
- Additional protocols
- More HTML tags
- CSS support
- JavaScript (in future)

### Educational Value
Demonstrates:
- OS-level networking
- Driver development
- Protocol implementation
- Text processing
- User interface design

## Future Enhancements

The modular architecture allows for:
- Real network communication (currently mocked)
- DNS resolution
- HTTPS/TLS support
- More HTML tags
- CSS styling with colors
- Graphics mode rendering
- Interactive input
- Multiple tabs
- Bookmarks
- Download manager

## Testing in Emulator

To see the browser in action:

```bash
# Build the OS
make clean && make

# Run in QEMU with networking
qemu-system-i386 -fda build/main_floppy.img \
                 -boot a \
                 -m 32M \
                 -nic user,model=ne2000

# Or run in Bochs
bochs -f bochs_config
```

The browser will automatically start and show demo content!
