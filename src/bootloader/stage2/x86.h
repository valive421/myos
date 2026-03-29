#pragma once
#include "stdint.h"


// Basic x86 functions for the bootloader stage 2, implemented in assembly
void x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotientOut, uint32_t* remainderOut);
// Writes a character to the screen using teletype mode (scrolling)
void x86_Video_WriteCharTeletype(char c, uint8_t page);

// BIOS serial I/O (INT 14h), COM1
void x86_Serial_Init(void);
void x86_Serial_WriteChar(char c);

// Disk I/O functions using BIOS interrupts
bool x86_Disk_Reset(uint8_t drive);
// Reads sectors from the disk using CHS addressing, with retries on failure
bool x86_Disk_Read(uint8_t drive,
                   uint16_t cylinder,
                   uint16_t sector,
                   uint16_t head,
                   uint8_t count,
                   uint16_t segment,
                   uint16_t offset);
// Gets the drive parameters (cylinders, sectors, heads) for the specified drive number
bool x86_Disk_GetDriveParams(uint8_t drive,
                             uint8_t* driveTypeOut,
                             uint16_t* cylindersOut,
                             uint16_t* sectorsOut,
                             uint16_t* headsOut);

/*
 * Performs a real-mode far jump to segment:offset and does not return.
 */
void x86_JumpToKernel(uint16_t segment, uint16_t offset);
