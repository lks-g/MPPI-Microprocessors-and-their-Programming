//********************************************************************
// Kniznica funkcii pre modul LCD displeja s radicom HD44780
//
// prepojenie pinov procesora s pinmi modulu LCD displeja:
// P1.0 => LCD_RS - vstup signalu Register Select, pin 4 modulu LCD
// P1.1 => LCD_EN - vstup signalu ENable, pin 6 modulu LCD
// P1.4 => DB4 - datovy vodic 4, Data Bus 4, pin 11 modulu LCD
// P1.5 => DB5 - datovy vodic 5, Data Bus 5, pin 12 modulu LCD
// P1.6 => DB6 - datovy vodic 6, Data Bus 6, pin 13 modulu LCD
// P1.7 => DB7 - datovy vodic 7, Data Bus 7, pin 14 modulu LCD
//********************************************************************

#include <msp430g2231.h>
#include "evb2.h"

void evb2_io_init(void)
{
	WDTCTL = WDTPW + WDTHOLD;   // Zastav casovac watchdogu
	P1DIR |= 0xF3;				// P1.0, P1.1 a P1.4 az 7 do vystupnej funkcie
	P1OUT = 0x00;				// inicializacia P1
}

void lcd_strobe(void)
{
	LCD_EN_H;
	asm("	nop");
	LCD_EN_L;
}

/* Zapis jeden byte do modulu LCD, 4-bitova zbernica */
void lcd_write(unsigned char byte)
{
	P1OUT = (P1OUT & 0x03) | (byte & 0xF0);
	lcd_strobe();
	__delay_cycles(3000);

	P1OUT = (P1OUT & 0x03) | ((byte << 4) & 0xF0);
	lcd_strobe();
	__delay_cycles(3000);
}

/* Vymaz displej a nastav pointer na zaciatok (home) */
void lcd_clear(void)
{
	LCD_RS_L;
	lcd_write(0x1);
	__delay_cycles(5000);
}

/* Zapis jeden znak do displeja */
void lcd_putch(char c)
{
	LCD_RS_H;
	lcd_write(c);
}

/* Zapis retazec znakov do displeja (write a string of chars to the LCD) */
void lcd_puts(const unsigned char *s)
{
	LCD_RS_H;
	while(*s)
		lcd_write(*s++);
}

/* Nastav smernik na zelanu poziciu (Go to the specified position) */
void lcd_goto(unsigned char pos)
{
	LCD_RS_L;
	lcd_write(0x80 + pos);
}

/* Nastavenie kurzora na zadanu poziciu */
void lcd_gotoCG(unsigned char pos)
{
    LCD_RS_L;
    lcd_write(0x40 + pos); // 0x40 - Nastavenie adresy CG RAM
    // K dispozicii mame 8 adries od 0000 po 0111.
}

/* Inicializacia modulu, nastav 4 -bitovu komunikaciu */
/* (initialization of LCD - put into 4 bit mode */
void lcd_init(void)
{
	LCD_RS_L;				// Budu sa zapisovat riadiace bajty (write control bytes)
	__delay_cycles(50000);	// Cakaj po zapnuti displeja (power on delay)

	P1OUT = 0x30;			// Nastav datove bity na 0011

	lcd_strobe();           // Zapis ich
    __delay_cycles(15000);

	lcd_strobe();           // Po pauze zapis to iste
	__delay_cycles(500);

	lcd_strobe();           // Po pauze zapis to iste
	__delay_cycles(500);

	P1OUT = 0x20;			// Nastavenie 4 / bitoveho modu (set 4 bit mode)

	lcd_strobe();           // Zapis
	__delay_cycles(500);

	lcd_write(0x28);		// Volame uz funkciu zapisu 2x po4bity, 4bitovy mod, 2 riadky, matica 5x7
	                        // (4 bit mode, 2 lines, 5x7 font)
	lcd_write(0x08);		// Zhasni displej (display off)
	lcd_write(0x0F);		// Zapni zobrazovanie, zapni kurzor, blikaj kurzorom
	                        // (display on, cursor on, blink cursor on)
	lcd_write(0x01);		// Vynuluj DDRAM (clear display)
	lcd_write(0x06);		// Spravanie sa displeja po zapise znaku, pohne sa kurzor
	                        // (entry mode set)
}
