/*
    Cvicenie 11 - 12
    Meno: Lukas Grulik
*/

//***********************************************************
// Mikropocitace a ich programovanie
// Cvicenie c. 11 a 12
// Riadenie inteligentneho alfanumerickeho displeja DEM16217 - dvojriadkovy
// MSP-EXP430G2 _LaunchPad_Extended_Board_2
//***********************************************************

#include <msp430.h>
#include "evb2.h"

//const unsigned char text1[] = "Mikropocitace a ich"; //"Mikropoc" "\xa0" "tace a ich";
const unsigned char text1[] = "Mikropo" "\x02" "\x01" "ta" "\x02" "e a ich";
const unsigned char text2[] = "   programovanie   ";

void main(void)
{
	char i = 0;
	
	evb2_io_init();         // Inicializacia P1 a Watchdog
	
	lcd_init();
	lcd_clear();            // Vymazanie znakov na displeji
	lcd_puts(&text1[0]);    // Zapis textoveho pola do displeja
	lcd_goto(0x40);         // Nastavenie polohy kurzoru
	lcd_puts(&text2[0]);    // Odoslanie druheho textoveho pola
	LCD_RS_L;               // 0 - koniec zapisu
    lcd_write(0x0C);        // Zapis do modulu LCD

    lcd_gotoCG(0b001000);   // Prvy riadok znaku 01, dlhe i
    LCD_RS_H;               // 1 - zapis do CG alebo DD RAM
    lcd_write(0b00010);     // Zapis do modulu LCD
    lcd_write(0b00100);
    lcd_write(0b00000);
    lcd_write(0b01100);
    lcd_write(0b00100);
    lcd_write(0b00100);
    lcd_write(0b01110);
    lcd_write(0b00000);

    lcd_gotoCG(0b010000);   // Prvy riadok znaku 02, c s makcenom
    LCD_RS_H;               // 1 - zapis do CG alebo DD RAM
    lcd_write(0b01010);     // Zapis do modulu LCD
    lcd_write(0b00100);
    lcd_write(0b01111);
    lcd_write(0b10000);
    lcd_write(0b10000);
    lcd_write(0b10000);
    lcd_write(0b01111);
    lcd_write(0b00000);

    LCD_RS_L;               // 0 - koniec zapisu
	
	while(1);
	{
		for(i = 0; i < 16; i++)     // Posun doprava
		{
			__delay_cycles(500000);
			lcd_write(0x1C);
		}

		for(i = 0; i < 35; i++)     // Posun dolava
		{
			__delay_cycles(500000);
			lcd_write(0x18);
		}

		for(i = 0; i < 19; i++)     // Posun doprava (vycentrovanie)
		{
			__delay_cycles(500000);
			lcd_write(0x1C);
		}
	}
}

/*
Zaver:
    1) Vysvetlili a ukazali sme si pracu s inteligentnym LCD displejom.
    2) Zistili sme, ze displej nepodporuje zobrazenie znakov s interpunkciou.
    3) Ukazali sme si textovy posun na LCD displeji.

CV.12:
    1) Pridali sme si vlastne znaky dlhe i ("í") a c s makcenom (č).
    2) Povodny text sme upravili tak, aby sa zobrazil s novovytvorenymi znakmi.
*/
