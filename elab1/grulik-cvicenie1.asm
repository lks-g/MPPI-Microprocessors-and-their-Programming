; Cvicenie 1
; Meno: Lukas Grulik

;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       	; Include device header file
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         	; inicializacia ukazovatela zasobnika
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  	; zastavenie watchdog-u
;-------------------------------------------------------------------------------

SetupP1		bis.b   #01000001b,&P1DIR	; P1.0 a P6.0 je nastaveny ako vystup | Rozsvietenie oboch LED diod

		mov.b	#001h,&P1OUT    		; Zelena LED sa rozsvieti ako prva.
Mainloop	xor.b   #01000001b,&P1OUT	; Softverovy prepinac | Upravou hodnoty rozsvietime buď zelenú, červenú, alebo obe LED diody.
		mov.w   #10,R14					; Zapisali sme hodnotu do registra R14 na spomalenie cyklu. (10x pomalsie)
Wait		mov.w   #65000,R15			; Register R15 - pocitadlo | Upravou hodnoty v registri R15 na 30 000 sme blikanie 2x zrychlili  
L1		dec.w   R15						; Dekrementacia R15
 		jnz     L1						; Test: R15 = 0
 		mov.w   #65000,R15				; Nanovo sme zapisali hodnotu na R15 | Zmenou hodnoty v registri R15 upravime rychlost blikania LED diod
 		dec.w	R14 					; Dekrementacia R14
 		jnz     L1						; Test: R15 = 0
		jmp     Mainloop				; Ak R15 > 0, opakujeme cyklus

;-------------------------------------------------------------------------------

; Zaver:
;	1. Preštudovali sme si rozloženie na vývojovej doske.
;	2. Upravovali sme hodnotu registrov a sledovali zmeny na LED diodach.
;	   Zmenou hodnoty v registri R15 vieme upravit rychlost blikania LED diod.
;	3. Krokovali sme program a manualne upravovali hodnoty v registroch.   
; 	4. Ukazali sme si pracu s debuggerom, pouzivanie breakpointov.
;	5. Problem nastal pri spomaleni blikania LED diod. 
;	   Do registra R15 sme zapisali hodnotu 130 000 co v hex. sustave je 1FBD0. 
;	   Kedze register ma 16b. Operand pretecie a cislo sa oreze na FBD0 | 64 464d. 
;	   To sposobi, ze LED diody nebudu blikat 2x pomalsie.
;	   Problem sme vyriesili pridanim volneho registra R14 ktory zabezpecil pridavnou dekrementaciou spomalenie programu.

;-------------------------------------------------------------------------------