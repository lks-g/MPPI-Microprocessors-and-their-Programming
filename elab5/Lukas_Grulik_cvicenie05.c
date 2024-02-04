/*
    Cvicenie 5
    Meno: Lukas Grulik
*/

//**************************************************************************
// MPP cv.5
// Timer0_A3, prepinaj P1.0 a nezavisle aj P1.6, CCR0, kontinualny mod pocitadla, LFXT do SMCLK
// priorita spustenia viacerych ISR
//**************************************************************************

#include <msp430.h>

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// Zastav WDT

	P1DIR |= 0x41;					// Piny P1.0 a P1.6 nastav ako vystupy
	P1OUT = 0x00;					// Vynuluj vsetky bity, piny portu P1

	// Blok generovania hodinovych signalov
	// po resete je nastavene:

	// DCOCLK/1 -> MCLK -> CPU, (frekvencia cca 1000000 Hz)
	// LFXT/1   -> ACLK         (frekvencia presne 32768 Hz)

	// Potrebujeme zmenit v module hodinoveho signalu:
	// LFXT/1 -> SMCLK

	// Mozne nastavenie kondenzatorov oscilatora  LFXT
	// BCSCTL3 |=XCAP1;             // Kondenzatory pri krystali: xcap=10b -> 10 pF
	// BCSCTL3 &=~XCAP0;            // Po resete xcap=01b -> 6 pF, bolo malo

	P1OUT = 0x41;	                // Indikacia, ze LFXT nebezi, program ostava v nasledovnej slucke
	__bic_SR_register(OSCOFF);		// Zapnutie oscillatora LFXT1
		do {						// Pre istotu
		   IFG1 &= ~OFIFG;
		   __delay_cycles(50);
		   } while((IFG1&OFIFG));	// Cakanie na rozbeh LFXT
	P1OUT = 0x00;                   // LFXT bezi, zhasni LED-ky

	// Potom samotne nastavenie:

	// Nastavenie registrov casovaca	 T_A
	// Pocitadlo TAR po resete nepocita, je neaktivne

	CCR1 = 3000;                    // Komparacny register CCR1 | Capture / compare
	CCR0 = 3000;					// Komparacny register CCR0 | Capture / compare
									// Startovacia hodnota, od nej frekvencia
									// blikania LED nezavisi
                                    
	BCSCTL2 |= SELS;                // Zapis log. 1 na SELS bit aby sme mohli pouzit LFXT1
    TACTL = TASSEL_2 + MC_2;        // Timer_A control register | Zapisali sme (10) Continuous mode - timer pocita do 0FFFFh (65535d)
			                        // Skontrolovat parametre podla zadania, skomentovat v zavere
									// "Open declaration" pre TASSEL_1 | Preddefinovane bity
                                    // ? je hodinovy signal SMCLK zdrojom impulzov pre pocitadlo TAR ?,
                                    // - Ano, nastavili sme ho cez TASSEL_2
									// ? je start pocitania pocitadla v mode kontinualneho pocitania
									// (od  0 do 0xffff)? | Ano, pretoze MC_2 sme nastavili na 10.
									// Prikaz nuluje zvysne bity registra. Skontrolujem, co sa este
									// vsetko zmeni a ci si to mozem dovolit zmenit
									// kde je mazanie priznaku TAIFG? | V komparatore 1

	CCTL0 = CCIFG;					// Nastal priznak - Interrupt Flag zapis log. 1 | Zbytocny zapis
	CCTL0 = CCIE;					// Povolenie prerusenia od priznaku nastatia rovnosti obsahu TAR a CCR0
									// Mozem ostatne bity vynulovat? aky bude priznak?

	CCTL1 = CCIE;                   // Povolenie priznaku

    P1IES = BIT1 + BIT2;            // Reaguj na zostupnu hranu na P1.1 a P1.2
    //P1IES  &= ~ BIT3;             // Ak chcem nabeznu hranu....
    P1IFG = 0;                      // Nuluj vsetkych osem priznakov

    P1IE = BIT1 + BIT2;             // Povol spustenie ISR od pinov P1.1 a P1.6

	_BIS_SR(LPM0_bits + GIE);		// Prechod do modu LPM0, vsetky maskovatelne prer. povolit
									// Vypneme len MCLK, LFXT oscilator musi ostat bezat, lebo napaaja casovac

}

#pragma vector = PORT1_VECTOR
__interrupt void nieco (void)
{
    if (P1IFG & BIT1)           // Tlacidlo 1
    {
        CCTL0 ^= CCIE;          // Vypnutie prerusenia | Zastavie blikania
        P1OUT &= ~0x01;         // Zmen len zelenu LED
        P1IFG &= ~BIT1;         // Nuluj len priznak P1IFG.1
    }

    if (P1IFG & BIT2)           // Tlacidlo 2
    {
        CCTL1 ^= CCIE;          // Vypnutie prerusenia | Zastavie blikania
        P1OUT &= ~0x40;         // Zmen len cervenu LED
        P1IFG &= ~BIT2;         // Nuluj len priznak P1IFG.2
    }
}

// Obsluzny program casovaca T_A0   // priznak TACCTL0_CCIFG

#pragma vector = TIMER0_A0_VECTOR	// Skontrolujte nazov vektora
__interrupt void komp0 (void)
{

	P1OUT ^= 0x01;		            // Zmen log. stav na P1.0

	// f = 32768Hz * 0.5s = 16384
	CCR0 += 16384;					// Pripocitaj hodnotu delta k obsahu CCR0
									// Frekvencia zavisi od hodnoty delta
}

#pragma vector = TIMER0_A1_VECTOR   // Skontrolujte nazov vektora
__interrupt void komp1 (void)
{

    P1OUT ^= 0x40;                  // Zmen log. stav na P1.6

    // f = 32768Hz * 0.7s = 22938
    CCR1 += 22938;                  // Pripocitaj hodnotu delta k obsahu CCR1
                                    // Frekvencia zavisi od hodnoty delta
    CCTL1 &= ~CCIFG;                // Nulovanie flagu
}

/*

Zaver:

    1) Prestudovali sme si sposob konfiguracie casovaca Timer_A.
    2) Riadiaci register casovaca sme nastavili na TASSEL_2 (10).
    3) V kode sme upravou hodnot v registroch CCR0 a CCR1 (Capture/Compare) menili rychlost blikania.
       - Pouzili sme vzorec f = freq(Hz) * perioda(napr. 0.5s)
       - Vysledok sme zapisali do registra CCR0 / CCR1
    4) Prerobili sme program tak aby sme pomocou tlacidiel na pinoch portu P1.1 (zelene) a P1.6 (cervene)
       zapinali / vypinali blikanie LED diod.
       - Aby sme zabezpecili zhasnutie LED diody pri prepnuti, pouzili sme vypnutie prerusenia
         CCIE (Vynulovali sme prerusenie v registroch CCR0 a CCR1).

Otazky (56 - 63):

    1)  "Open declaration" pre TASSEL_1
         - Preddefinovane bity, jednoduchsi zapis oproti manualnemu zapisu na vybrane bity.
    2)  Je hodinovy signal SMCLK zdrojom impulzov pre pocitadlo TAR ?
         - Ano, nastavili sme ho cez TASSEL_2.
    3)  Je start pocitania pocitadla v mode kontinualneho pocitania (od  0 do 0xffff) ?
         - Ano, pretoze MC_2 sme nastavili na 10.
    4)  Kde je mazanie priznaku TAIFG?
         - V komparatore 1.
*/
