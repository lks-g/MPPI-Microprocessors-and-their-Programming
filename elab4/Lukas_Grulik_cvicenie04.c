/*
     Cvicenie 4
     Meno: Lukas Grulik
*/

#include <msp430.h>

int i,j;

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// Zastav watchdog
	// Nastavenie riadiacich a datovych registrov portu P1
	P1DIR = BIT0|BIT6;				// Piny P1.0 a P1.6 nastav na vystup
	P1OUT = 0x00;					// Zhasni obe LED

	P2REN=0x3f;         // Povolit rezistory, aby definovali log uroven | Obmedzenie externych vplyvov
	P2OUT=0x00;         // Pull down, P2DIR je nulovy, vstup, to je v poriadku

	// Nastavenie riadiacich registrov prerusenia od portu P1
	P1IES = BIT1 + BIT2;	// Reaguj na zostupnu hranu P1.1 a P1.2
	//P1IES  &= ~ BIT3;	    // Ak chcem nabeznu hranu....
	P1IFG = 0;		        // Nuluj vsetkych osem priznakov

	P1IE = BIT1 + BIT2 + BIT3;	// Povol spustenie ISR len od pinu P1.1, P1.2 a P1.3
	
	_BIS_SR(GIE);	        // Povol vsetky maskovatelne preruseni
					        // Od tohto okamihu je mozne spustit ISR

 // _BIS_SR(GIE + LPM0_bits);   // Uved CPU do nizkoprikonoveho rezimu LPM0
	_BIS_SR(GIE + LPM4_bits);   // Uved CPU do nizkoprikonoveho rezimu LPM4
									
	while(1);		// Nekonecna slucka
}
//#pragma vector = 2    // Aj takto je mozne definovat vektor prerusenia od portu P1
					    // Pozri v: .../ccsv5/ccs_base/msp430/include/msp430g2231.h

#pragma vector = PORT1_VECTOR           // Vektor prerusenia
__interrupt void nieco (void)
{
/*
	for (j=16; j > 0; j--)
	{
		P1OUT ^= 0x01;			        // Zmen stav na p1.0
		for (i = 20000; i > 0; i--);	// Programove oneskorenie
	}
	P1OUT = 0x00;				        // Vypni obe LED

	P1IFG &= ~BIT1;                     // Viacpriznakova ISR, mazanie prizankov programom
*/

	if (P1IFG & BIT1)       // Vykoname ked nastane prerusenie bitu 1
	{
	    P1OUT ^= 0x01;		// Zmen len zelenu LED
	    P1IFG &= ~BIT1;		// Nuluj len priznak P1IFG.1
	}

    if (P1IFG & BIT2)       // Vykoname ked nastane prerusenie bitu 2
    {
        P1OUT ^= 0x40;      // Zmen len cervenu LED
        P1IFG &= ~BIT2;     // Nuluj len priznak P1IFG.1
    }

	if (P1IFG & BIT3)       // Vykoname ked nastane prerusenie bitu 3
	{
		if(P1IN & BIT4)
		{
			P1OUT ^= 0x01;	// Zmen zelenu LED
		}
		else
		{
			P1OUT ^= 0x40;	// Zmen cervenu LED
		}
		P1IFG &= ~BIT3;		// Nuluj len priznak P1IFG.3
	}
}

/*
Zaver:

    1) Ukazali sme si pracu s tlacitkami.
        - Najprv sme testovali ci sa pod stlaceni zeleneho tlacitka rozsvieti zelena LED dioda.
        - Kod sme upravili tak aby sa sa pocet bliknuti nepredlzoval po stlaceni tlacidla.
            - Presunuli sme P1IFG pod for cyklus.
    2) Vyskusali sme si pracu s rezimom LPM0 (Low Power Mode 0) a LPM4 (Low Power Mode 4).
        - Zmeny odberu prudu sme kontrolovali pomocou ampermetra.
        - Povodna spotreba bola cca 0.45 mA, 0.25 - 0.15 mA pri LPM0 a 0.08 mA pri LPM4.
    3) Upravili sme program tak aby sme mohli pomocou zeleneho a cerveneho tlacitka
       prepinat medzi zelenou a cervenou LED diodou.
    4) Ako poslednu vec sme si vysvetlili a ukazali pracu s enkoderom.
       - Upravili sme program tak aby sme smerom doprava (na nabeznu hranu)
         rozsvietili cervenu LED diodu a smerom dolava (na dobeznu hranu) zelenu LED diodu.
*/