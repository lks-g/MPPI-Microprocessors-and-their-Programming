/*
    Cvicenie 9 - Tabulka - 64
    Meno: Lukas Grulik
*/

//**************** externy D/A prevodnik, vzorky z tabulky, ********* Direct Digital Synthesis
#include <msp430.h>

const unsigned int vzorky[] = {2046,2247,2445,2640,2829,3010,3183,3344,3493,3628,3747,3850,3936
		,4004,4053,4082,4092,4082,4053,4004,3936,3850,3747,3628,3493,3344,3183,3010,2829,2640,
		2445,2247,2046,1845,1647,1452,1263,1082,909,748,599,464,345,242,156,88,39,10,0,10,39,88,
		156,242,345,464,599,748,909,1082,1263,1452,1647,1845,2046,2247 //};
        ,2445,2640,2829,3010,3183,3344,3493,3628,3747,3850,3936};

		//,4004,4053,4082,4092,4082,4053,4004,3936,3850,3747,3628,3493,3344,3183,3010,2829,2640 };
		//,2445,2247,2046,1845,1647,1452,1263,1082,909,748,599,464,345,242,156,88,39,10,0,10,39,88
	    //,156,242,345,464,599,748,909,1082,1263,1452,1647,1845};

// Pole vzorky je ulozene v pamati FLASH, lebo ma modifikator const
// Kedze nasa periferia vie poslat len 8 bitov
// musime 16 bitove vzorky rozdelit na polovicu
unsigned int i = 0, j = 0, k = 50 ;
unsigned int data16;
char RXin;
char *horne8;
char *dolne8;

#define tlv5636_cntrl	0x9001  // Pomale ustalenie vyst. napatia,3252,  referencia 1.024V

//#define tlv5636_cntrl 0x9002  // Kontrolny register referencia 2.048V

void main(void)
{
    dolne8 = &data16;
    horne8 = &data16;
    horne8 = horne8+1;

	WDTCTL = WDTPW + WDTHOLD;

// Inicializacia P1

	P1OUT = 0x00;
    P1DIR |= BIT6;  // Pin P1.6 bude generovat pulz FS

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
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1);    // Bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCCKPL|UCMSB|UCMST|UCSYNC);         // Bity do log.1, MUSIME nastavit
    // Ked bolo UCCKPH a sucasne UCCKPL v log.1, t.j. CLK bol medzi vysielaniami v log.0 a
    // Data sa citaju s dobeznou hranou, tak prevodnik akceptoval kazdu druhu vzorku (16bitovu)

    // Zdroj signalu pre UCBOCLK:
    UCB0CTL1 |=UCSSEL0|UCSSEL1; // Bity do log.1, zdroj hodin je SMCLK, 1MHz (cca)

    // Delicka frekvencie UCB0CLK - dva registre:
    // Registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 4;    // Delicka hodin, spodnych 8 bitov, delenie 4, staci rychlost takato, 250kHz
    UCB0BR1 = 0;    // Delicka hodin, hornych 8 bitov
    // Teda UCBCLK bude  SMCLK/4 = 250kHz

    UCB0CTL1 &= ~UCSWRST;   // Uvolni USCI z resetu, modul je pripraveny vysielat data

// ******************** koniec nastavovania modulu USCI

    // IE2 |= UCB0RXIE; UCB0TXIE; povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR
    // nic sa nepovoluje

    // Nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami dat z USCI
    CCR0 = 3000;               // Komparacny register CCR0 (TACCR0)
                               // Startovacia hodnota

   TACTL = TASSEL_1 + MC_2;    // Hodinovy signal pre casovac bude ACLK, 32kHz
                               // MC_2 - pocitaj dookola od 0 to 0xffffh a znova od 0
                               // v prikaze je "=" vsetky ostatne bity su nulovane
                               // Prikaz tiez sucasne nuluje priznak TAIFG

// ************** koniec nastavovania modulu casovaca

   CCTL0 = CCIE;           // Povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           // Prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

// Koniec nastavovani

   // Naplnenie pomocnych premennych na vysielanie
   data16 = tlv5636_cntrl;
   //horne8 = tlv5636_cntrl>>8;    // Hornych 8 bitov
   //dolne8 = (char)tlv5636_cntrl; // Len spodnych 8 bitov

   _BIS_SR(GIE + LPM0_bits);       // Vstup do hibernacie LPM0
}

// ISR od casovaca, priznak TACCTL0_CCIFG
#pragma vector = TIMER0_A0_VECTOR
__interrupt void rovnost (void)
{
    //P1OUT |= BIT1;      // Ako dlho bezi ISR, start merania

    P1OUT |=BIT6;         // Vytvor pulz na P1.6 pre signal FS prevodnika
    __delay_cycles(5);

    P1OUT &= ~BIT6;       // Zapis log.0 na P1.6

    // Odosielanie
    UCB0TXBUF = *horne8;  // Do shift registra vloz
                          // Hornych 8 bitov konfiguracneho slova
                          // prevodnika DAC
    UCB0TXBUF = *dolne8;  // a hned aj dolne8; horne8 sa uz skopirovali do vysielacieho registra a BUF je uz volny pre dolne8
                          // to je krasa buffrovaneho serioveho portu
/*
    i++;    // Premenna pre pocitadlo od 0 po 63.
    if(i==64)i=0;
*/
    j += k;       // Zvecsi j o fazu k, citanie kazdej vzorky pola raz -> k=16
    i = j;        // Drz v j celu hodnotu pre nasledovne pricitanie hodnoty k
    i = i>>4;     // Odrez spodne bity, adresuj pole vyssimi bitmi i
    j &= 0x3FF;   // Max hodnota 0x3f = 63

    data16 = vzorky[i]; // Pole so 64 prvkami | Zapis aktualnej vzorky

    CCR0 += 5;    // Dlzka pauzy medzi dvoma vysielaniami. musi to byt dlhsie ako trva vypocet v ISR

    RXin=UCB0RXBUF; // Precitanim UCB0RXBUF nulujem priznak UCB0RXIFG, ani to teraz netreba, len zo slusnosti
    //P1OUT &= ~BIT1; Ako dlho trva rutina, koniec merania
}

/*
Zaver:
    1) Sledovali sme zmeny priebehu analogoveho signalu na osciloskope.
    2) Referencne napatie sme zmenili z 1.024V na 2.048V.
       - Upravou hodnoty v kontrolnom registri tlv5636_cntrl z 0x9001 na 0x9002.
    3) Frekvenciu generovaneho signalu je mozne zvysit / znizit zmenou casovaca,
       alebo upravou (pridanim alebo odstranenim) vzoriek z pola "vzorky".
    4) Namiesto enkodera sme pouzili okno "Expressions" kde sme menili hodnotu premennej "k".
*/