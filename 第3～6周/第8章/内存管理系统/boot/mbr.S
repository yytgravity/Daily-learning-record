%include "boot.inc"
SECTION MBR vstart=0x7c00
    
 mov ax,cs
 mov ds,cs
 mov es,ax
 mov ss,ax
 mov fs,ax
 mov sp,0x7c00
 mov ax,0xb800
 mov gs,ax

 mov  ax, 0x600
 mov  bx, 0x700
