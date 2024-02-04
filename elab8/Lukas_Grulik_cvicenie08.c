/*
    Cvicenie 8
    Meno: Lukas Grulik
*/

//******************************************************************************
// MPP Cv. 8
// Analogovo/digitalny prevodnik - voltmeter
//******************************************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT6;
#define FS_L        P1OUT &= ~BIT6;

#define TEMP // Definicia pre ulohy teplomera

void fs_strobe(void);

unsigned char jed = 4, des = 3, sto = 2, tis = 1;
const unsigned char tvary_cislic[11] = {0x3F, 6, 0x5b, 0x4f, 0x66, 0x6D,
										0x7D, 0x07, 0x7F, 0x6F,0x40};

// long signed int vysledok=70;
// long int vysledok1;

unsigned char i;	    // Ktora cislicovka sa bude vysielat

unsigned char RXin;     // Na kopirovanie z UCB0RXBUF

void fs_strobe(void)    // Generuj SW zapisovaci pulz 74HC595, signal STCP, pin12
{
        FS_H;   // P1OUT |= BIT6;
        asm(" nop");
        FS_L;   // P1OUT &= ~BIT6;
}

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// Zastav WDT

    // Inicializaci P1
    P1OUT = 0x00;
    P1DIR |= BIT6;  // Nastav pin 6 do vystupu, prepisovaci pulz, pin 6 nebude riadeny periferiou UCSI

    // Nastavenie alternativnej funkcie pinov.
    // Piny P1.5 and P1.7 uz dalej nebudu riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;     // Pripoj vyvod USCI UCB0MOSI (bude  vystup dat) na pin P1.7 (BIT7)
                            // a vystup hodin. sig. UCB0CLK na pin P1.5 (BIT5)
                            // vyvod MISO, P1.6 nebudeme pouzivat, ostane riadeny reg-om P1OUT, bude generovat prepisovaci pulz
    P1SEL2 |= BIT7|BIT5;    // To iste, MOSI na P1.7, UCBOCLK na P1.5
    
    // Nastavenie modulu/periferie USCI pre mod SPI...
    // Nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST;    // Vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


    // Stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1);    // Bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCMSB|UCMST|UCSYNC);                // Bity do log.1, MUSIME nastavit

    // Zdroj signalu pre UCBOCLK:
    UCB0CTL1 &= ~(UCSSEL1);  // Bity do nuly
    UCB0CTL1 |= UCSSEL0;     // Bity do log.1, zdroj hodin je ACLK, 32kHz

    // Delicka frekvencie UCB0CLK - dva registre:
    // Registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 1;    // Delicka hodin, spodnych 8 bitov, delenie 1
    UCB0BR1 = 0;    // Delicka hodin, hornych 8 bitov

    UCB0CTL1 &= ~UCSWRST;   // Uvolni USCI z resetu, modul je pripraveny vysielat data

    // ******************** koniec nastavovania modulu USCI

    IE2 |= UCB0RXIE;    // UCB0TXIE; - Povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // Pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR

//*********************   nastavenie modulu casovaca

    // Najprv start oscilatora LFXT, je potrebny ako pre casovac tak aj pre USCI
    P1OUT = 0x40;                   // Zelena led indikuje, ze LFXT este nebezi
    __bic_SR_register(OSCOFF);      // Zapni oscilator LFXT1, len pre istoru
        do {
           IFG1 &= ~OFIFG;          // Periodicky nuluj priznak nefunkcnosti oscilatora
           __delay_cycles(50);
           } while((IFG1&OFIFG));   // Je stale nefunkcny?
    P1OUT = 0x00;                   // LFXT bezi, zhasni LED

    // Nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi prevodmi
    CCR0 = 3000;               // Komparacny register CCR0 (TACCR0)
                               // Startovacia hodnota

   TACTL = TASSEL_1 + MC_2;    // Hodinovy signal pre casovac bude ACLK
                               // MC_2 - pocitaj dookola od 0 to 0xffffh a znova od 0
                               // v prikaze je "=" vsetky ostatne bity su nulovane
                               // Prikaz tiez sucasne nuluje priznak TAIFG

// ************** koniec nastavovania modulu casovaca

       CCTL0 = CCIE;           // Povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                               // Prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

#ifndef TEMP
/*************** zaciatok nastavovania periferie prevodnika ADC10**********/
	ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; // Interval ustalenia vstup. napatia,
												// ADC10SHT_2 znaci
												// * 16 x period hodin ADC10CLK / 8
												// tj 25.6 us
												// ADC10ON - zapnutie prevodnika,
												// ADC10IE - povolenie prerusenia
	// SREF_0 -> VR+ = VCC, VR- = VSS,
	// MSC = 0 po skonceni prevodu sa nespusti hned novy prevod
	// REFON = 0 vypni interny zdroj referencneho napatia
	// ADC10IFG = 0 nulovanie priznaku,  ENC = 0 prevod zakazany

	ADC10CTL1 = INCH_0|ADC10DIV_7;  // Vstup prevodnika pripojime na kanal A0,
									// ktory je na pine, kde bol povodne P1.0
									// t.j. nastavime alternativnu funkciu pinu
    // Odpojit zelenu LED-ku!!
	// SHSx = 0x00 - spustenie prevodu od bitu ADC10SC
	// ADC10DF =0  - hodnota napatia vyjadrena vo formate: priamy kod (straight binary code)
	// ADC10DIV_7 = 0x00 - delicka hodin signalu 1:8 (5MHz/8=625kHz)
	// ADC10SSELx = 0x00 - volba hodinoveho signalu pre prevodnik -	oscilator prevodnika, ADC10OSC

	ADC10AE0 |= 0x01;   // Odpojenie digitalnych casti pinu portu P1.0
/************************koniec nastavovanie prevodnika ADC10 ********************/
#else
    /*************** zaciatok nastavovania periferie prevodnika ADC10**********/
        ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + ADC10IE + REFON;
        // ADC10SHT_3 je interval ustalenia vstup. napatia, znaci
        // * 64 x period hodin ADC10CLK/8, tj 409.6 us, t.j. 0.4ms
        // ADC10ON - zapnutie prevodnika,
        // ADC10IE - povolenie prerusenia
        // SREF_1 -> VR+ = VREF+ and VR- = GND, treba zapnut Vref
        // REFON=1 zapni interny zdroj referencneho napatia
        // REF2_5V = 0 napatie Vref bude 1.5 V
        // MSC = 0 po skonceni prevodu sa nespusti hned novy prevod
        // ADC10IFG = 0 nulovanie priznaku,  ENC = 0 prevod zakazany

        ADC10CTL1 = INCH_10|ADC10DIV_7;         // Vstup prevodnika pripojime na kanal A10,
                                                // ktory je intrne spojeny s teplotnym cidlom
        // INCH_10 = Teplotny senzor
        // SHSx = 0x00 - spustenie prevodu od bitu ADC10SC
        // ADC10DF =0  - hodnota napatia vyjadrena vo formate: priamy kod (straight binary code)
        // ADC10DIV_7 = 0x00 - delicka hodin signalu 1:8 (5MHz/8=625kHz)
        // ADC10SSELx = 0x00 - volba hodinoveho signalu pre prevodnik -  oscilator ADC10OSC
    /************************koniec nastavovanie prevodnika ADC10 ********************/
#endif

    _BIS_SR(LPM0_bits + GIE);    // Vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                                 // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                                 // lebo napaja casovac aj seriovy port
}

#pragma vector = TIMER0_A0_VECTOR       // Spusti sa raz za sekundu
__interrupt void komp0 (void)
{
    ADC10CTL0 |= ENC + ADC10SC;         // Povolenie prevodu - bit ENC a
                                        // Start prevodu - bit ADC10SC
     CCR0 += 32768;                     // Raz za sekundu
}

#pragma vector=ADC10_VECTOR         // Prevod sa skoncil, bit ADC10IFG od prevodnika ADC10 je v log.1,
__interrupt void ADC10_ISR(void)
{
    signed int pomoc = 0;
    long int vysledok;

#ifndef TEMP
		vysledok=(int)ADC10MEM; // 0 - 1023 => 0 - 3220 | Hodnota konverzie z prevodniku

        /********* Voltmeter *********/
		// 4753 bytes pri pouziti dat. typu float
		//vysledok = vysledok * 3.24;
		//vysledok = vysledok / 1.022;

		// 1197 bytes pri pouziti dat. typu int
        //vysledok = vysledok * 3240;
        //vysledok = vysledok / 1022;

        // 1101 bytes pri pouziti bit shift operatora
        vysledok = vysledok * 12985; // (4096 / 1022) * 3240 = 12985
        vysledok = vysledok >> 12;   // 2^12 = 4096 | Bitovy posun o 12 doprava

		/************** prevod bin do BCD ******************/
		pomoc=(int)vysledok;    // Pri odcitavani moze byt "pomoc" aj zaporna
#else
        vysledok=(int)ADC10MEM; // 0 - 1023 => 0 - 3220 V
        
        /********* Teplomer *********/

        //1221 byte pri deleni
        //1119 byte pri bitovom posune

        //Vypocet napatia
        vysledok = vysledok * 6006; // (4096 / 1023) * 1500 = 6006
        vysledok = vysledok >> 12;  // 2^12 = 4096 | Bitovy posun o 12 doprava

        //Uprava rovnice
        //100 * Vtemp(mV) = 355 * T + 98600
        //100 * Vtemp(mV) - 98600 = 355 * T
        //(100 * Vtemp(mV) - 98600)/355 = T

        //Aplikacia rovnice
        vysledok = vysledok * 100;
        vysledok = vysledok - 98600;
        vysledok = vysledok * 144;      // Prevod na stotiny stupna celzia
                                        // (512 / 355) * 100 = 144
        vysledok = vysledok >> 9;       // 2^9 = 512 | Bitovy posun

        /************** prevod bin do BCD ******************/
        pomoc=(int)vysledok;
        // pomoc = (int)((long int)vysledok >> 11);  // Pri odcitavani moze byt "pomoc" aj zaporna
#endif

        // Musi byt preto definovana ako znamienkova
		jed = 0; des = 0, sto = 0, tis = 0;

		do {
		    pomoc-=1000;
		    tis++;
		}

		while(pomoc>=0);	    // Kolko je tisicok
		tis--;
		pomoc+=1000;

		do {
		    pomoc-=100;
		    sto++;
		}

		while(pomoc>=0);	    // Kolko je stoviek
		sto--;
		pomoc+=100;

		do {
		    pomoc-=10;
		    des++;
		}

		while(pomoc>=0);
		des--;
		pomoc+=10;

		do {
		    pomoc-=1;
		    jed++;
		}

		while(pomoc>=0);
		jed--;
		//pomoc += 1;

		if(tis>9){tis=10;sto=10;des=10;jed=10;}     // Cislo vacsie ako 9999
		// Vypise pomlcky
		// Potlacenie prvej nuly??

/****************** vysielanie dat na cislicovky****************/

		UCB0TXBUF = ~(tvary_cislic[jed]);	// Kopiruj data do registra a start vysielania

		i = 1;	// Nasledujucich 8 bitov bude rad desiatok
}

#pragma vector = USCIAB0RX_VECTOR   // Spusti potom, co si vyslal data (8 bitov)
__interrupt void dalsie_cislicovky (void)
{
    switch(i)
            {
            case 1:
                UCB0TXBUF =  ~(tvary_cislic[des]);
                i = 2; // Ktory rad bude poslany dalsi - stovky
            break;

            case 2:
#ifndef TEMP
                UCB0TXBUF =  ~(tvary_cislic[sto]);      // Desatinna bodka
#else
                UCB0TXBUF =  ~(tvary_cislic[sto]|0x80); // Desatinna bodka
#endif
                i = 3;      // Ktory rad bude poslany dalsi - tisicky
            break;

            case 3:
#ifndef TEMP
                UCB0TXBUF =  ~(tvary_cislic[tis]|0x80);
#else
                UCB0TXBUF =  ~(tvary_cislic[tis]);
#endif
                i = 4;        // Co bude poslane ako dalsie  - prepisovaci pulz
            break;

            case 4:
                fs_strobe();  // Prepisovaci pulz

                i = 5;        // Ak i = 5 ziadny case neplati len nuluj priznak
            break;
            }

    RXin=UCB0RXBUF;           // Precitanim UCB0RXBUF nulujem priznak UCB0RXIFG
}

/*
               
Zaver:

    1) Vyskusali sme si pracu s analogovo digitalnym prevodnikom.
    2) Kod sme upravili tak aby sa spravne zobrazila hodnota napatia na LED displeji.
    3) Proces nasobenia sme prerobili tak, aby nebolo potrebne pouzit ciselny format
       s desatinnou čiarkou.
    4) Naprogramovali sme si digitalny voltmeter.
       - Hodnotu sme zobrazili na LED displeji,
         postupne zlava od jednotiek po tisiciny voltu.
    5) Ako poslednu vec sme prerobili program tak aby LED displej ukazal
       aktualnu teplotu procesora v stotinach stupna celzia.
       - Ukazali sme si riesenie s pouzitim delenia a bitoveho posunu.

Otazky:

    1) Preco displej nezobrazuje hodnoty napatia s krokom 0.001V ale s krokom vacsim?
       Aku hodnotu ma tento krok?
       - Je to sposobene bitovym rozlisenim ktore mame k dispozicii.
       - Vypocet na zistenie jedneho bitu je 3.23 / 1024 = 0,003154...
         Jeden bit z prevodniku zodpoveda hodnote cca 3 mV.

    2) Aké je rozlíšenie (krok v °C) teplomera ?
       - Jeden bit z nasho prevodnika je 0,41 stupna celzia.
       Vypocet: 1,5 - 0,986 / 0,00355 = 144,788... => Max. teoreticka teplota
                0 - 0,986 / 0,00355   = -277,74... => Min. teoreticka teplota
       MAX - MIN / Pocet urovni => 144,7887323943662 - (-277,7464788732394) / 1024 = 0,41

*/

