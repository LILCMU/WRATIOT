#include "LILCustomizeCC2530Board.h"
#include "stdio.h"
#include "MT_UART.h"

#ifdef LIL_HOPHER

void beepLogoChip(void){

    uint8 Beep_Gekko_Packet[7];
    sprintf(Beep_Gekko_Packet,"%c%c%c%c%c%c",0x54,0xfe,0x03,0x00,0x0b,0x0b);
    HalUARTWrite(MT_UART_DEFAULT_PORT, Beep_Gekko_Packet, 6);


}

#endif