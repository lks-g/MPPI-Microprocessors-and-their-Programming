/*
 Cvicenie. 2
 Meno: Lukas Grulik
--------------------------------------------------
 * Mikropocitace a ich programovanie
 * cvicenie c. 2 softverovy test tlacitka na porte
*/

#include  <msp430.h>

void onesk(unsigned int i);     // Deklaracia funkcie onesk

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;   // Zastav casovac watchdog-u
  P1DIR |= 0x41;              // Nastav piny P1.0 and P1.6 do vystupneho modu | Nezmenime žiadny bit v registri okrem vybraných dvoch.
  P1DIR &= 0xF7;			        // Nastav pin P1.3 do vstupnej funkcie
                              // (Zbytocny prikaz. Preco?) *
  unsigned int j = 0;         // Kolko bitovy je short integer? *
  for (;;)                    // Nekonecny cyklus
  {
  	P1OUT = 0x00;             // Vystupny register z portu

  	while(P1IN & BIT3);       // Test pustene tl, test na log.1 z P1.3 | Citanie tlacidla
    //stlacil som
  	P1OUT = 0x01;             // Rozsvietime zelenu LED diodu zapisom 1 na pin P1.0

  	onesk(1000);              // Nasledujuci test spusti az pochvili, preco? | Zavolanie funkcie onesk
  	while(~P1IN & BIT3);      // Test stlacene tl | Cakam kym je stlacene tlacidlo
    // Pustil som
    // LED stale svieti
  	P1OUT = 0x40;             // Rozsvietime cervenu LED diodu zapisom 1 na pin P1.6

  	onesk(1000);
  	while(P1IN & BIT3);       // Test pustene tl
    // Stlacil som
  	P1OUT = 0x00;             // LED zhasne

  	onesk(1000);
	while(~P1IN & BIT3);        // Test stlacene tl
    // Pustil som
    // LED stale zhasnuta
	onesk(1000);

	while(P1IN & BIT3);         // Test pustene tl
    // Stlacil som
    // LED stale zhasnuta
    // onesk(2); netreba, je tam 'for'

  	for(j=0;j<5;j++)
  	{
  		P1OUT = 0x41;           // Nastavime piny P1.0 a P1.6 na hodnotu 1
  		onesk(50000);
  		// 50000 je na signed integer
  		// uz prilis velka
  		// Na unsigned integer este nie
  		P1OUT = 0x00;           // LED zhasne
  		onesk(20000);
  	}
  	while(~P1IN & BIT3);      // Test stlacene
  }
}

// Cakacia funkcia
void onesk(unsigned int i)   // Definicia funkcie onesk
{
	do {(i--);
	    asm(" nop");           // Musi byt 1 medzera pred nop aby to chapal ako instrukciu a nie ako navestie
	}
    while (i != 0);
}

/*
  Zaver:

    1. Otazka (20): Kolko bitovy je short integer?
        - Short int ma 2 bajty co je 16 bitov.
        - V jazyku C to vieme velmi lahko zistit pomocou prikazu sizeof().
    2. Funkciou "onesk" osetrime mechanicke kmitanie kontaktov tlacidla.
        - Zmenou hodnoty vstupneho parametra "i" vieme upravit rychlost rozsvietenia LED diod.
    3. Otazka (19): Zbytocny prikaz. Preco?
        - Po resete je kazdy pin nastaveny na nulu. Cize zapiseme nulu na miesto kde uz nula je.
    4. Problem nastal pri testovani tlacidla. LED dioda neblikala a tlacidlo nereagovalo spravne pri stlaceni.
        - Chyba bola sposobena tym, ze program preskocil cyklus vo funkcii "onesk".
        - Tuto chybu sme opravili odkomentovanim prikazu "asm(" nop");" vo funkcii onesk.
    5. Po uprave kodu nastal opat problem a to, ze LED diody nezhasli ked mali.
        - Chyba sa opravila restartovanim dosky a opatovnym nahratim kodu.
    6. Program sme upravili tak aby pri stlaceni a podrzani tlacidla svietila zelena LED dioda
       a po pusteni sa rozsvietila cervena LED dioda a zelena zhasla.
*/
