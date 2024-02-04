/*
    Cvicenie 9 - Oscilator
    Meno: Lukas Grulik
*/

//**************** externy D/A prevodnik, vzorky z tabulky, ********* Direct Digital Synthesis
#include <msp430.h>

long y[3] = {0, -408, -797};  // Hodnoty v poli su so znamienkom
                              // hodnoty sa budu menit
int A = 4000;                 // max 4000 ,min 0, ako const to dlho cita
long out = 0;
unsigned int data16;
char RXin;
char *horne8;
char *dolne8;

#define tlv5636_cntrl	0x9001  // Pomale ustalenie vyst. napatia,3252,  referencia 1.024V

void main(void)
{
    dolne8 = &data16;
    horne8 = &data16;
    horne8 = horne8+1;

	WDTCTL = WDTPW + WDTHOLD;
// Inicializacia P1
	P1OUT = 0x00;
    P1DIR |= BIT6|BIT1;  // Pin P1.6 bude generovat pulz FS

// Nastavenie alternativnej funkcie pinov.
// Piny P1.5 and P1.7 uz dalej nebudu riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;     // Pripoj vyvod USCI UCB0MOSI (bude  vystup dat) na pin P1.7 (BIT7)
                            // a vystup hodin. sig. UCB0CLK na pin P1.5 (BIT5)
                            // Vyvod MISO, P1.6 nebudeme pouzivat, ostane riadeny reg-om P1OUT
    P1SEL2 |= BIT7|BIT5;    // To iste, MOSI na P1.7, UCBOCLK na P1.5

// Nastavenie modulu/periferie USCI pre mod SPI...

    // Nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST;    // Vstup do stavu reset, po resete procesora je uz v resete, len pre istotu
    
    // Stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // Bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCCKPL|UCMSB|UCMST|UCSYNC);      // Bity do log.1, MUSIME nastavit
    // Ked bolo UCCKPH a sucasne UCCKPL v log.1, t.j. CLK bol medzi vysielaniami v log.0 a
    // data sa citaju s dobeznou hranou, tak prevodnik akceptoval kazdu druhu vzorku (16bitovu)

    // Zdroj signalu pre UCBOCLK:
    UCB0CTL1 |=UCSSEL0|UCSSEL1; // Bity do log.1, zdroj hodin je SMCLK, 1MHz (cca)

    // Delicka frekvencie UCB0CLK - dva registre:
    // registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 4; //delicka hodin, spodnych 8 bitov, delenie 4, staci rychlost takato, 250kHz
    UCB0BR1 = 0; //delicka hodin, hornych 8 bitov
    // Teda UCBCLK bude  SMCLK/4 = 250kHz

    UCB0CTL1 &= ~UCSWRST;   // Uvolni USCI z resetu, modul je pripraveny vysielat data

// ******************** koniec nastavovania modulu USCI

    //IE2 |= UCB0RXIE; UCB0TXIE; Povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // Pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR
    // nic sa nepovoluje

    // Nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami dat z USCI
    CCR0 = 3000;               // Komparacny register CCR0 (TACCR0)
                               // Startovacia hodnota

   TACTL = TASSEL_1 + MC_2;    // Hodinovy signal pre casovac bude ACLK, 32kHz
                               // MC_2 - pocitaj dookola od 0 to 0xffffh a znova od 0
                               // V prikaze je "=" vsetky ostatne bity su nulovane
                               // Prikaz tiez sucasne nuluje priznak TAIFG

// ************** koniec nastavovania modulu casovaca

   CCTL0 = CCIE;           // Povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           // Prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

// Koniec nastavovani

   // Naplnenie pomocnych premennych na vysielanie
   data16 = tlv5636_cntrl;

	_BIS_SR(GIE + LPM0_bits);   // Vstup do hibernacie LPM0
}

// ISR od casovaca, priznak TACCTL0_CCIFG
#pragma vector = TIMER0_A0_VECTOR
__interrupt void rovnost (void)
{
    P1OUT |= BIT1;      // Ako dlho bezi ISR, start merania

    P1OUT |=BIT6;       // Vytvor pulz na P1.6 pre signal FS prevodnika
    __delay_cycles(5);

    P1OUT &= ~BIT6;     // Zapis log.0 na P1.6

    UCB0TXBUF = *horne8;    // Do shift registra vloz
                            // Hornych 8 bitov konfiguracneho slova
                            // prevodnika DAC
    UCB0TXBUF = *dolne8;    // a hned aj dolne8; horne8 sa uz skopirovali do vysielacieho registra a BUF je uz volne pre dolne8
                            // To je krasa buffrovaneho serioveho portu
                            
    y[0] = ((A * y[1]) >> 11) - y[2]; // Vypocet novej hodnoty do prevodnika
        y[2] = y[1];                // Presun hodnot, najstarsia sa prepise
        y[1] = y[0];                // Presun hodnot
        out = y[0] + 2048;          // Prevodnik pracuje len s kladnymi cislami,
                                    // preto posun zapornych cisiel do kladnych
                                    // "y" a "out" su so znamienkom

    data16 = out;

    CCR0 += 13; // Dlzka pauzy medzi dvoma vysielaniami.
                // Musi to byt dlhsie ako trva vypocet v ISR. Pretoze vypocet trva dlhsie.

    RXin=UCB0RXBUF; // Precitanim UCB0RXBUF nulujem priznak UCB0RXIFG, ani to teraz netreba, len zo slusnosti
    P1OUT &= ~BIT1; // Ako dlho trva rutina, koniec merania
}

/*
Zaver:
    1) Tabulku sme nahradili kodom od riadku 106.
    2) Zmenou hodnoty v premennej "A" sme sledovali zmeny sinusovky.
*/

