/*
    Cvicenie 7
    Meno: Lukas Grulik
*/

//***********************************************************
// Stopky s LED displejom a riadením, 4x 7segmentov
//***********************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT0;      // (1 << 0)
#define FS_L        P1OUT &= ~BIT0;     // (1 << 0)

const unsigned char tvary_cislic[10] = {0x3F, 6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7F, 0x6F};
// Ulozene do FLASH
// "1"     6 = 0000 0110, hgfedCBa
// "6"  0x7c = 0111 1100 -> 0111 1101 0x7d
// "0"  0x3F = 0011 1111, hgFEDCBA

unsigned char jed_sek = 0, des_sek = 0, jed_min = 0, des_min = 0, RXin;

unsigned char i = 0;  // Ktory rad sa vysiela

void fs_strobe(void);
void disp_write(void);

void disp_write(void) // Funkcia na osetrenie spravneho zobrazenia udajov na displeji
{
    if (jed_sek == 10) {jed_sek = 0, des_sek++;}
    if (des_sek == 6 ) {des_sek = 0, jed_min++;}
    if (jed_min == 10) {jed_min = 0, des_min++;}
    if (des_min == 6 ) {des_min = 0;}

    if (jed_sek == 255) {jed_sek = 9, des_sek--;}
    if (des_sek == 255) {des_sek = 5, jed_min--;}
    if (jed_min == 255) {jed_min = 9, des_min--;}
    if (des_min == 255) {des_min = 5;}

    UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
    // Do registra vloz prvok z pola podla hodnoty cislice rady jedotiek sekund
    // Start prenosu prvých 8 bitov
    i = 1;    // ktory rad bude poslany ako nasledujuci - des_sek
}

void fs_strobe(void) // Vygeneruj prepisovaci pulz na P1.0
    {
        FS_H; // P1OUT |= BIT0;
        asm("   nop");
        FS_L; // P1OUT &= ~BIT0;
    }

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // Zastav watchdog

    i = 1;

    P1OUT = 0x00;             // Inicializácia P1, hlavne kvoli prepisovaciemu pulzu
    P1DIR |= BIT0;            // Nastav P1.0 na vystup, smer ostatnych pinov sa nastavi pri pripojovani periferie USCI nizsie

    // Nastavenie alternativnej funkcie pinov. Piny P1.5 a P1.7 uz nebudu vic riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;       // Pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)
                              // Signál UCB0MISO nepouzivame, tak pin P1.6 ostane v zakladnej I/O funkcii, cez P1OUT
    P1SEL2 |= BIT7|BIT5;      // To iste: pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)

    // Nastavenie modulu/periferie USCI pre mod SPI...
    // Nez budeme menit nastavenie modulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST;     // Vstup do stavu reset, po resete procesora je uz v resete, len pre istotu

    // Stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1);   // Bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCMSB|UCMST|UCSYNC);               // Bity do log.1, MUSIME nastavit

    // Zdroj signalu pre UCBOCLK:
    // UCB0CTL1 &= ~(UCSSEL1);     // Bity do nuly,
    // UCB0CTL1 |=UCSSEL0;         // Bity do log.1, zdroj hodin je ACLK, 32kHz

    UCB0CTL1 &= ~(UCSSEL1);        // Bity do nuly,
    UCB0CTL1 |=(UCSSEL0);          // Bity do log.1, zdroj hodin je ACLK, 32kHz

    // Delicka frekvencie UCB0CLK - dva registre:
    // Registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 1; // Delicka hodin, spodnych 8 bitov, delenie 1
    UCB0BR1 = 0; // Delicka hodin, hornych 8 bitov

    UCB0CTL1 &= ~UCSWRST;  // Uvolni USCI z resetu, modul je pripraveny vysielat data

    // ******************** koniec nastavovania modulu USCI
    // Teraz je modul USCI v mode SPI a je pripraveny vysielat data

    IE2 |= UCB0RXIE; // UCB0TXIE; povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // Pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR

    // Nastavenie riadiacich registrov prerusenia od potu P1
    P1IES = BIT1 + BIT2;    // Reaguj na zostupnu hranu na P1.1 a P1.2
    //P1IES  &= ~ BIT3;     // ak chcem nabeznu hranu....
    P1IFG = 0;              // Nuluj vsetkych osem priznakov

    P1IE = BIT1 + BIT2 + BIT3;  // povol spustenie ISR od pinov P1.1, P1.2 a P1.3

    // Kontrola cinnosti krystaloveho oscilatora LFXT, je potrebny ako pre casovac tak aj pre USCI

    P1OUT = 0x40;                   // Zelena led indikuje, ze LFXT este nebezi
    __bic_SR_register(OSCOFF);      // Zapni oscilator LFXT1, len pre istoru
        do {
           IFG1 &= ~OFIFG;          // Periodicky nuluj priznak nefunkcnosti oscilatora
           __delay_cycles(50);
           } while((IFG1&OFIFG));   // je stale nefunkcny?
    P1OUT = 0x00;                   // LFXT bezi, zhasni LED

    // Nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami bloku 4x8 bitov dat z USCI

    CCR0 = 3000;               // Komparacny register CCR0 (TACCR0)
                               // Startovacia hodnota
   TACTL = TASSEL_1 + MC_2;    // Hodinovy signal pre casovac bude ACLK, 32kHz

    // ************** koniec nastavovania modulu casovaca

   CCTL0 = CCIE;               // Povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                               // Prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

   _BIS_SR(LPM0_bits + GIE);   // Vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                               // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                               // lebo napaja casovac aj seriovy port
    }

    // ISR od casovaca, LEN od príznaku TACCTL0_CCIFG

#pragma vector = TIMER0_A0_VECTOR
__interrupt void komp0 (void)
{
    // ISR sa spustila lebo bol obsah registrov TAR a CCR0 rovnaký (teraz už nemusí tak by)

    jed_sek++;      // Jednotky sekund - Inkrementacia
    //jed_sek--;    // Jednotky sekund - Dekrementacia

    disp_write();   // Zavolanie funkcie na reset

    CCR0 += 32768;  // Spusti ISR od casovaca kazdu sekundu
}

#pragma vector = USCIAB0RX_VECTOR   // Bude ine start after data have been sent
__interrupt void after_sent (void)
{
    switch(i)       // Vyber cislic ktore sa budu zobrazovat na displeji
            {
            case 1:
                UCB0TXBUF =  ~(tvary_cislic[des_sek]);
                i = 2;  // ktory rad bude poslany dalsi - jed_min)
            break;

            case 2:
                UCB0TXBUF =  ~(tvary_cislic[jed_min]);
                i = 3; // ktory rad bude poslany dalsi - des_min)
            break;

            case 3:
                UCB0TXBUF =  ~(tvary_cislic[des_min]);
                i = 4; // Co bude poslane ako dalsie - prepisovací pulz
            break;

            case 4:
                fs_strobe();    // Prepisovaci pulz | Zapise hodnotu z posuvnych registrov
                                // z pamate na vystup.

                i = 5;    // Ak i = 5 ziadny case neplati len zmaz priznak
            break;
            }

    RXin=UCB0RXBUF;       // Precitanim UCB0RXBUF nulujem priznak UCB0RXIFG
}

#pragma vector = PORT1_VECTOR
__interrupt void nieco (void)
{
    if (P1IFG & BIT1)       // Tlacidlo 1 - Zelene
    {
        //P1OUT ^= 0x01;    // Zmen len zelenu LED
        jed_sek = 0;        // jednotky sekund - Nulovanie hodnoty
        des_sek = 0;        // desiatky sekund - ...
        jed_min = 0;        // jednotky minut  - ...
        des_min = 0;        // desiatky minut  - ...
        disp_write();       // Zavolanie funkcie na zabezpecenie spravneho zobrazenia udajov.
        P1IFG &= ~BIT1;     // Nuluj len priznak P1IFG.1
    }

    if (P1IFG & BIT2)       // Tlacidlo 2 - Cervene
    {
        //P1OUT ^= 0x40;    // Zmen len cervenu LED
        CCTL0 ^= CCIE;      // XOR bitu Capture/compare interrupt enable
                            // Spustime alebo zastavime pocitanie
        P1IFG &= ~BIT2;     // Nuluj len priznak P1IFG.2
    }

    if (P1IFG & BIT3)       // Enkoder
    {
        if(P1IN & BIT4)
        {
            //P1OUT ^= 0x01;// Zmen zelenu LED
            jed_sek++;      // Jednotky sekund - Inkrementacia
            disp_write();
        }

        else
        {
            //P1OUT ^= 0x40;// Zmen cervenu LED
            jed_sek--;      // Jednotky sekund - Dekrementacia
            disp_write();
        }

        P1IFG &= ~BIT3;     // Nuluj len priznak P1IFG.3
    }
}

/*

Zaver:

    1) Vysvetlili sme si ako funguje 7-segmentovy displej.
    2) Upravili sme pole "tvary_cislic" tak aby sa 6 prvok (segment A) rozsvietil spravne.
       - Pripocitali sme 1-tku k 6 prvku (0x7c => 0x7d).
    3) Kod sme upravili tak aby hodnotu dekrementoval.
       - Podmienky sme upravili tak aby sa hodnoty spravne orezavali a dekrementovali sme
         hodnotu jednotiek sekund (jed_sek--).
    4) Do programu sme implementovali novu funkcionalitu.
       - Vytvorili sme si pomocnu funkciu na odosielanie dat "disp_write()".
         Zapisali sme do nej podmienky na zabezpecenie spravneho orezania udajov
         a spusti odosielanie.
       - Pridali sme si tlacidla (zelene a cervene) a enkoder.
       - Potom sme zelenemu tlacidlu nastavili aby po stlaceni nuloval udaj na displeji.
         Cervene tlacidlo sme nastavili tak aby po prvom stlaceni zastavil pocitanie,
         a druhym stlacenim ho obnovil.
       - Enkoder otacanim doprava (reaguje na nabeznu hranu) inkrementuje udaj na displeji
         otacanim dolava (reaguje na dobeznu hranu) dekremenuje udaj.

*/
