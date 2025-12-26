#!/bin/bash
qemu-system-i386 -fda build/main_floppy.img -boot a -m 32M -nic user,model=ne2000