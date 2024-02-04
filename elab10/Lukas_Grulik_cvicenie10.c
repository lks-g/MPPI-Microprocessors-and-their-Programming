/*
    Cvicenie 10
    Meno: Lukas Grulik
*/

//***********************************************************
// Zbernica I2C
//***********************************************************

#include <msp430.h>

unsigned char i,j,outLED;

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;   // Zastav watchdog
    outLED = 1;
    i = 1;
    j = 1;  // Adresa bola poslana, j = 2 -data boli poslane

    P1DIR = 1;  // P1.0 vystup (zel. LED)

    // Nastavenie alternativnej funkcie pinov
    // Piny P1.6 and P1.7 dalej nebudu riadene bitmi registra P1OUT:

    P1SEL |= BIT6|BIT7;     // Pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)
                            // Pin P1.5 (hodiny pri SPI) nepouzivame
    P1SEL2 |= BIT6|BIT7;    // To istee aj tu: pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)

    P2DIR |= BIT5;  // Vystup pre modru LED
    P2OUT &= ~BIT5; // Zhasni modru LED

    // Inicializacia periferie I2C_USCI

    // Uved modul do resetu, potom: I2C Master, zdroj hodin,
    // adresa slave-u, smer dat, ......, uvolni USCI z resetu:

    // Pred robenim zmien v USCI module, uved ho do stavu resetu:
    UCB0CTL1 |= UCSWRST;    // Uvedenie do resetu, pre istoru, po PUC je uz modul v resete

    // V I2C mode mame rovnako nazvany register UCB0CTL0,
    // ale jeho styri horne bity maju uplne iny vyznam ako v SPI moode!!!
    // v pripade registra UCB0CTL1, je v nom viacej nastavovacich bitov, ktore NEboli
    // pouzite v rezime SPI.

//****** Nastavenie bitov v registri UCB0CTL0 *****//

    // Stav UCB0CTL0 po resete: 0x01, UCSYNC bit = 1, synchronny mod -
    // - je riadeny hodinovym signalom 

    // Bity registra UCB0CTL0, ktore je potrebne nulovat (pre istotu)
    UCB0CTL0 &= ~(UCA10|UCSLA10|UCMM);  // Tieto bity nuluj, pre istotu
    // UCA10 - vlastna addresa UCSI je 7-bit-ova
    // UCSLA10 - adresa slave-u je 7-bit-ova
    // UCMM - nie multi-master, blok porovnavania adresy je neaktivny

    // Bity registra UCB0CTL0, ktore je potrebne nastavit do log.1
    UCB0CTL0 |= (UCMST|UCMODE1|UCMODE0|UCSYNC); // Bity do log.1, musia byt nastavene
    
    // UCMST - master mode
    // UCMODE1|UCMODE0 - I2C mode
    // UCSYNC - synchronny rezim, pre istotu

// ***** Nastavenie bitov v registri UCB0CTL1 ***** //

    // Obsah registra UCB0CTL1 po resete: 0x01, USCI modul je drzany v stave resetu,
    // uz bolo spominane

    // Bity registra UCB0CTL1 ktore su sice nulove ale pre istotu ich nulujeme
    UCB0CTL1 &= ~(UCSSEL1|UCTXNACK|UCTXSTP|UCTXSTT);    // Nulovanie bitov a sucasne musi byt modul stale v resete (UCSWRST bit=1)
    // UCTXNACK - potvrdzuj normalne, ACK bit
    // UCTXSTP negeneruj STOP stav, neskor ho pouzijeme
    // UCTXSTT negeneruj START stav, neskor ho pouzijeme

    // Bity registra UCB0CTL1, ktore je potrebne nastavit do log.1
    UCB0CTL1 |= UCSSEL0|UCSSEL1|UCTR|UCSWRST;
    // Zdroj hodin SMCLK, 1MHz, (UCSSEL1,0 = 1)
    // UCTR=1 - modul USCI bude vysielat data do externeho obvodu slave
    // (po vyslani adresy slave modul vysle R/*W bit =0)
    // UCSWRST=1 - drz modul v resete, pre istotu

    // Registre delicky hodinoveho signalu:
    // Musia byt nastavene po PUC resete, lebo nie su automaticky inicializovane
    UCB0BR0 = 8;    // Delicka, spodnych 8 bitov, delenie 8, minimalne dovolene je :4
    UCB0BR1 = 0;    // Delicka, hornych 8 bitov,
    // Signal SCL bude mat frekvenciu 1000000 / 8 = 125000 Hz. nie,
    // ale 112000 Hz (merane osciloskopom)

// ***** Nastavenie adresy slave-u, register UCBxI2CSA ***** //
    UCB0I2CSA = 0x3B;     // Spravna adresa 0111010
    //UCB0I2CSA = 0x4C;   // Nespravna adresa
    
// **** Uvolnenie modulu USCI z resetu **** //

    UCB0CTL1 &= ~UCSWRST;   // Uvolni modul USCI z resetu

    // Vyber priznakov
    IFG2 &=~(0xC);      // Vynuluj USCI B0 RX and TX priznaky
    IE2 |= UCB0TXIE;    // Povol priznak po starte vysielania dat/addresy
    // Jeden vektor pre prijem, UCB0TXIFG aj vysielnie, UCB0RXIFG

    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // Nuluj vsetky stavove priznaky USCI I2C
    // UCB0I2CIE |= UCNACKIE|UCSTPIE|UCSTTIE|UCALIE; povol vsetky stavove priznaky I2C
    UCB0I2CIE |= UCNACKIE;  // Povol len priznak nepotvrdenia
    // USI je uvolnene davno z resetu

// Nastavenie modulu casovaca a oscilator LFXT

// Test start oscilatora LFXT
    P1OUT = 0x01;   // Zelena LED svieti ak LFXT nebezi
    __bic_SR_register(OSCOFF);      
        do {                        
           IFG1 &= ~OFIFG;
           __delay_cycles(50);
           } while((IFG1&OFIFG));   
    P1OUT = 0x00; // Zhasni LED, LFXT bezi

// Nastavenie casovaca
    CCR0 = 3000;       

   TACTL = TASSEL_1 + MC_2; // ACLK je zdrojom hodin
                               
   CCTL0 = CCIE;            // Povol prerusenie, zmaz priznak

// ****** Koniec nastavenie casovaca ****** //
   _BIS_SR(LPM0_bits + GIE);
}

#pragma vector = TIMER0_A0_VECTOR       
__interrupt void porov (void)
{
// Vyrob novy stav LED slpca
    switch (i)
    {
        case 1:
            outLED = outLED<<1;   // Zasviet LED o jednu vyssie
            if(outLED == 0)       // Ak je 1 mimo 8 bitov
            {
                outLED = 0x40;    // Vrat na predposlednu poziciu
                i = 2;
            }
        break;
        case 2:
            outLED = outLED>>1;   // Zasviet LED o jednu nizsie
            if(outLED == 0)       // Ak je 1 mimo 8 bitov
            {
                outLED = 2;       // Vrat na predposlednu poziciu z druheho konca
                i = 1;
            }
        break;
    }

    UCB0CTL1 |= UCTXSTT; // Start vysielania
    j = 1;               // Najprv addresu slave

    CCR0 += 32768;
    
    P1OUT ^= 1;         // Zmen zelenu LED
}

#pragma vector = USCIAB0TX_VECTOR    // USCI_B0 I2C vysiel/prij. addr./data!!!!
__interrupt void adresa_data (void)  // Spusti sa po priznakoch UCB0TXIFG a UCB0RXIFG
{
// Po zacati vysielanie adresy sa prvy raz spusti aby naplnila datami TXBUF
// Druhy raz sa spusti, ked sa zacnu vysielat data. Vtedy je ten spravny cas
// nastavit generovanie STOP, ktore sa ale neprejavi okamzite ale az po vyslani dat
// do TXBUF sme v druhom behu nic nezapisali tak sa vygeruje ten STOP stav

switch (j){
case 1:
    j = 2;
    UCB0TXBUF = ~outLED;
    break;
case 2:
    j = 1;
    UCB0CTL1 |= UCTXSTP;    // Az sa data vyslu, vysli stop
    break;
}

    P2OUT ^= BIT5;  // Zmen modru LED

    IFG2 &=~(0x0C); // Nuluj priznaky USCI RX/TX adresa/data
}

#pragma vector = USCIAB0RX_VECTOR   // USCI_B0 I2C status, nie priznak CB0RXIFG!
__interrupt void status (void)      // Spusti sa ak UCALIFG, UCNACKIFG, ICSTTIFG, UCSTPIFG
{
    UCB0CTL1 |= UCTXSTP; // Adresa nerozpoznana vysli stop
    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // Vynuluj vsetky priznaky
    //P2OUT ^= BIT5;     // Zmen modru LED
}

/*
Zaver:
    1) Vysvetlili a ukazali sme si pracu s expanderom.
    2) Zistili sme adresu IO expandera.
       - Posledne 3 bity su hardverovo nastavene.
    3) Priebeh komunikacie na zbernici I2C sme sledovali pomocou osciloskopu.
*/









