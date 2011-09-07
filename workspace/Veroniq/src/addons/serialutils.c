#include <stdio.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "serial.h"

xComPortHandle serialPortHdlr;

int __attribute__((weak)) _write (int file, char * ptr, int len) { 
    int i;
    for ( i = 0; i < len; i++ ){
       xUsartPutChar(serialPortHdlr, ptr[i], 200);
    }
    return len;
}

size_t __attribute__((weak)) _write_r (struct _reent *ptr, int fd, const void *buf, size_t cnt )
{
    unsigned short remaining = usUsartPutString(serialPortHdlr, buf, cnt);
    return cnt - remaining;
}
/*
int serial_print(const char* format, ... ) {
    va_list args, o_args;
    int status = 0;
    
    va_start(args, format);
    va_copy(o_args, args); // Make copy of args for later use
    
    / * Count memory for new string * /
    
    
    status = vsnprintf(buf, len, format, o_args);
    
    va_end(args);
}
*/
