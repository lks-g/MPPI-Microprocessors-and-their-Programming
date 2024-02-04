/*
Cvicenie. 3
Meno: Lukas Grulik
*/

//Mikropocitace a ich programovanie
//******************************************************************************
// Priklad nastavenia modulu generovania hodinovych signalov
//******************************************************************************

#include <msp430.h>

short int i;

void blink(char n, unsigned int cykly);
void delay(unsigned int j);

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;   // Zastavenie casovaca watchdog

	P1DIR |= 0x41;              // Nastav piny portu P1.0 and P1.6 ako vystupne,
							    // Funkcia ostatnych pinov portu sa nemeni

	P1OUT = 0x01;		        // log.1 na P1.0 a log.0 na vsetky ostatne piny,
						        // teda aj na P1.6

// Kalibracia oscilatora DCO na DCO_FREQ = 1MHz a jeho zapnutie

	BCSCTL1 = CALBC1_1MHZ;		// Kalibracna konstanta
	DCOCTL = CALDCO_1MHZ;		// Kalibracna konstanta
	__bic_SR_register(SCG0);    // Zapnutie DCO
								// Intrizicka funkcia
	

// Test rozbehu krystalom riadeneho oscilatora LFXT1OSC

	__bic_SR_register(OSCOFF);  // Zapnutie LFXT1 , pre istotu
	do {
	   IFG1 &= ~OFIFG;          // Zapis log.0 na bit IFG1 | Kontrola ci sa oscilator rozbehol
	   __delay_cycles(50);
	   } while((IFG1&OFIFG));   // Cakanie na rozbeh LFXT1


while(1) {
		
//PRVE nastvenie zdroja hodinoveho signalu DCO->MCLK
		
		BCSCTL3 &= ~(LFXT1S1|LFXT1S0);	// Prepnutie medzi VLO a LFXT1 | Bity nastavime na log. 0
										// Prepnutie na LFXT1
		BCSCTL2 &= ~(SELM1|SELM0);		// Medzi DCOCLK a VLOorLFXT1 pre MCLK
										// DCOCLK -> MCLK
		BCSCTL2 &= ~(DIVM1|DIVM0);		// Delicka MCLK 1:1
		
		//REG |= BIT3;                  // Zapis log. 1 na specificky bit (bit3)
		//REG &= ~BIT3;                 // Zapis na vynulovanie specifickeho bitu (bit3)


		blink(5, 60000);    // 5-krat preblikni LED - cerv-zel-cerv-zel-cerv-zel-cerv-zel-cerv-zel
							//cislo 20000 nevyjadruje cas ale pocet vykonani funkcie "void delay(int j);"

		delay(60000);	    // svit LED sa chvilu nemeni, (ostava svietit zelena)
		delay(60000);
// DRUHE nastavenie zdroja hodinoveho signalu (DCO/2)->MCLK
		
		// vydelime DCOCLK dvoma, ostatne nastavenia su
        //rovnake z predosleho

        // DIVM0=1, DIVM1=0
        BCSCTL2 |= DIVM0;   // 500kHz | Do kontrolneho registra 2 sme zapisali na bit log.1

        blink(5, 30000);    // 5-krat preblikni LED - cerv-zel-cerv-zel-cerv-zel-cerv-zel-cerv-zel
                            //cislo 20000 nevyjadruje cas ale pocet vykonani funkcie "void delay(int j);"

        delay(30000);       // svit LED sa chvilu nemeni, (ostava svietit zelena)
        delay(30000);
		
// TRETIE nastavenie zdroja hodinoveho signalu LFXT1->MCLK

    __bic_SR_register(OSCOFF);  // Zapnutie LFXT1 , pre istotu
    do {
       IFG1 &= ~OFIFG;          // Zapis log.0 na IFG1 | Kontrola ci sa oscilator rozbehol
       __delay_cycles(50);
       } while((IFG1&OFIFG));   // Cakanie na rozbeh LFXT1


     BCSCTL2 &= ~DIVM0;         // Delicka MCLK 1:1 | Zapisme log.0

     BCSCTL2 |= (SELM1|SELM0);  // LFXT alebo VLO | Na oba bity zapiseme log.1

     //BCSCTL3 &= ~(LFXT1S1|LFXT1S0);       // Prepnutie medzi VLO a LFXT1 | Bit je nastaveny uz od zaciatku
                                            // Prepnutie na LFXT1
     blink(5,1966);         // 5-krat preblikni LED - cerv-zel-cerv-zel-cerv-zel-cerv-zel-cerv-zel
                            // Cislo 20000 nevyjadruje cas ale pocet vykonani funkcie "void delay(int j);"

     delay(1966);           // Svit LED sa chvilu nemeni, (ostava svietit zelena)
     delay(1966);

		
// STVRTE nastavenie zdroja hodinoveho signalu VLO->MCLK

    // LFXT1S1 do log.1
    BCSCTL3 |= LFXT1S1; // Zapis log.1 na bit 1
                        // * log. 0 nemusime zapisat, pretoze uz bola zapisana v predoslich zapisoch

    blink(5,720);       // 5-krat preblikni LED - cerv-zel-cerv-zel-cerv-zel-cerv-zel-cerv-zel
                        // Cislo 20000 nevyjadruje cas ale pocet vykonani funkcie "void delay(int j);"

    delay(720);         // Svit LED sa chvilu nemeni, (ostava svietit zelena)
    delay(720);



			P1OUT = 0x00;   // zhasni obe LED
			delay(720);     //@ fMCLK = 1000kHz
						    // a nechaj chvilu zhasnute (aby sme videli,
						    //ze program sa nachadza prave tu)
			delay(720);
			P1OUT = 0x01;   // zapni zelenu LED
	}
}

void blink(char n, unsigned int cykly) // Funkcia vykona blikanie LED diod.
{
	for(i=0;i<n;i++){
	    delay(cykly);
		P1OUT ^= 0x41;  //bitova operacia exclusive OR.
						//0x41=0b01000001, instrukcia precita stav vsetkych pinov portu P1
						//nacitane hodnoty bit po bite X-OR -uje s 0x41, vysledok zapise na vsetky piny portu
						//co svietilo - zhasne, co bolo zhasnute zasvieti. Ale len na bitovej pozicii .0 a .6
						//teda tam, kde operand 0x41 obsahuje log. 1
		delay(cykly);
		P1OUT ^= 0x41;
}
	
}

void delay(unsigned int j) // Funkcia sluzi na osetrenie mechanickeho kmitania
{
	do {(j--);
	asm(" nop");	    //funkcia musi obsahovat nejaku zmysluplnu instrukciu - staci aj assemblerovska " nop"
                        //inak prekladac funkciu "void delay(long int j)" vobec neprelozi a program ju nebude
                        //nikdy volat
	}
	while (j != 0);     // Cakaci cyklus
}

/*
 Zaver:

     1. Prestudovali sme si blokovu schemu generovania hodinového signalu.
        - Rozlozenie oscilatorov (Uloha.1 - DCO, XT2 (v nasom pripade sa nenachadza),
          Uloha.2 - LFXT1, Uloha.3 - LP/LF), deličky, multiplexora a hlavnych hodin.
     2. Kod sme upravovali podla zadania. Upravy sme kontrolovali podla blokovej schemy.
     3. Ukazali sme si, ze vieme dosiahnut taky isty vysledok na roznych oscilatoroch.
        - Zabezpecili sme to upravou hodnot vo funkciach "blink" a "delay" tak aby kazdy cyklus trval rovnaku dobu.
        - V ulohe 2 sme o polovicu znizili frekvenciu hodinoveho signalu (blink) a hodnotu oneskorenia (delay).
        - V tretej ulohe sme presli z 1MHz na 32kHz, kde sme taktiez upravili hodnotu "blikania" a "cakania".
     4. Problem nastal pri druhom spusteni programu (po upravach). Kod sa vykona spravne len pri prvom spusteni.
        - Problem bol sposobeny prepinanim na iny typ hodin co sposobilo, ze sa oscilator vypol a nedokazal sa nanovo rozbehnut.
        - Tuto chybu sme odstranili pridanim cyklusu do ulohy.3 ktory, vyresetuje oscilator a nasledne ho skontroluje.
*/