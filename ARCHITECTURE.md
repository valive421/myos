# MyOS Network & Browser Architecture

## System Boot Flow

```
┌─────────────────────────────────────────────────────────────┐
│                         BOOT PROCESS                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  BIOS/MBR Boot   │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │ Stage 1 Loader   │
                    │   (boot.asm)     │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │ Stage 2 Loader   │
                    │   FAT12 Read     │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  Kernel Entry    │
                    │  (entry.asm)     │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │   Kernel Main    │
                    │   (kmain.c)      │
                    └──────────────────┘
                              │
                ┌─────────────┴─────────────┐
                ▼                           ▼
        ┌──────────────┐          ┌──────────────┐
        │   Network    │          │   Browser    │
        │   Init       │          │   Start      │
        └──────────────┘          └──────────────┘
```

## Network Stack Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      APPLICATION LAYER                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Browser (browser.c)                                  │  │
│  │  - HTML rendering                                     │  │
│  │  - Page display                                       │  │
│  │  - User interface                                     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      PROTOCOL LAYER                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  HTTP Client (http.c)                                 │  │
│  │  - URL parsing                                        │  │
│  │  - HTTP GET requests                                  │  │
│  │  - Response parsing                                   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      TRANSPORT LAYER                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  TCP/IP Stack (network.c)                             │  │
│  │  - TCP connections                                    │  │
│  │  - IP addressing                                      │  │
│  │  - Packet handling                                    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      HARDWARE LAYER                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  NE2000 Driver (ne2000.c)                             │  │
│  │  - Card initialization                                │  │
│  │  - Packet send/receive                                │  │
│  │  - I/O port operations                                │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  NE2000 Hardware │
                    │  (Emulated)      │
                    └──────────────────┘
```

## Browser Request Flow

```
User Request (URL)
       │
       ▼
┌─────────────────┐
│ Browser Init    │ browser_navigate()
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Parse URL       │ parse_url()
│ - Extract host  │
│ - Extract path  │
│ - Extract port  │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Build HTTP Req  │ http_get()
│ - GET method    │
│ - Headers       │
│ - User-Agent    │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Send via TCP    │ tcp_send()
│ (Mock for demo) │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Receive Data    │ tcp_receive()
│ (Mock response) │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Parse HTTP Resp │
│ - Status code   │
│ - Headers       │
│ - Content       │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Render HTML     │ browser_render_html()
│ - Parse tags    │
│ - Format text   │
│ - Display       │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Display Page    │ browser_display_page()
│ - Show status   │
│ - Show content  │
└─────────────────┘
       │
       ▼
    [Done]
```

## HTML Rendering Process

```
Raw HTML Input
       │
       ▼
┌──────────────────────────┐
│  Scan character by char  │
└──────────────────────────┘
       │
       ├─── Found '<' ──┐
       │                │
       ▼                ▼
┌──────────┐    ┌──────────────┐
│ Text     │    │ Tag Detection │
│ Output   │    └──────────────┘
└──────────┘            │
                        │
         ┌──────────────┼──────────────┬──────────────┐
         ▼              ▼              ▼              ▼
    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
    │  <h1>  │    │  <p>   │    │  <li>  │    │  Other │
    │ Header │    │ Para   │    │ List   │    │ Strip  │
    └────────┘    └────────┘    └────────┘    └────────┘
         │              │              │              │
         ▼              ▼              ▼              ▼
    "=== X ===="   "\nText\n"    "\n * Item"   Continue
```

## Component Interaction

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│ Browser  │────▶│  HTTP    │────▶│ Network  │────▶│  NE2000  │
└──────────┘     └──────────┘     └──────────┘     └──────────┘
     │                │                 │                 │
     │                │                 │                 │
     ▼                ▼                 ▼                 ▼
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  stdio   │     │  string  │     │   x86    │     │   I/O    │
│  .c/.h   │     │  .c/.h   │     │  .asm/.h │     │  Ports   │
└──────────┘     └──────────┘     └──────────┘     └──────────┘

Support Libraries            System Layer         Hardware Layer
```

## File Dependencies

```
kmain.c
├── stdio.h
│   └── x86.h
├── network.h
│   ├── ne2000.h
│   │   ├── stdio.h
│   │   └── x86.h
│   └── stdio.h
└── browser.h
    ├── http.h
    │   ├── network.h
    │   ├── stdio.h
    │   └── string.h
    ├── stdio.h
    └── string.h
```

## Memory Layout (Real Mode)

```
0x0000 ─────────────────────
       │ Interrupt Vectors │
0x0400 ├───────────────────┤
       │ BIOS Data Area    │
0x0500 ├───────────────────┤
       │ Free Space        │
0x7C00 ├───────────────────┤
       │ Stage 1 Loader    │
0x7E00 ├───────────────────┤
       │ Stack (grows dn)  │
       ├───────────────────┤
       │ Free Space        │
0x2000:0 ──────────────────┤
       │ Stage 2 / Kernel  │
       │ - Code            │
       │ - Data            │
       │ - BSS             │
       ├───────────────────┤
       │ Free Space        │
       │ (Heap, Buffers)   │
       └───────────────────┘
```

## Data Structures

### Network Packet
```
┌──────────────┬──────────────┬──────────────┬──────────────┐
│ Ethernet Hdr │   IP Header  │  TCP Header  │   Payload    │
│   14 bytes   │   20 bytes   │   20 bytes   │  Variable    │
└──────────────┴──────────────┴──────────────┴──────────────┘
```

### HTTP Response
```
┌──────────────────────────────────┐
│ http_response_t                  │
├──────────────────────────────────┤
│ int status_code                  │
│ char content[HTTP_MAX_RESPONSE]  │
│ uint16_t content_length          │
└──────────────────────────────────┘
```

This architecture provides:
- ✓ Modularity (each layer independent)
- ✓ Extensibility (easy to add features)
- ✓ Clarity (clean separation of concerns)
- ✓ Testability (components can be tested independently)
