//USCI_A0 SPI transmite e recebe (A...Z, a...z, A... )
//USCI_B1 SPI = eco
/* Jumpers
 * P2.7 (CLK) --- P4.3 (CLK)
 * P3.3 (SIMO) --- P4.1 (SIMO)
 * P3.4 (SOMI) --- P4.2 (SOMI)
 * P3.2 (GPIO)------ P4.0 (STE) enable slave
 */

#include <msp430.h> 

void USCI_A0_config();      //Mestre
void USCI_B1_config();      //Slave

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	char letra, rx[26], i=0;

	USCI_A0_config();
	USCI_B1_config();

	__enable_interrupt();

	letra = 'A';

	P3OUT &= ~BIT2;         //habilitar escravo

	while(1){
	    while((UCA0IFG & UCTXIFG)==0);  //Esperar TXIFG = 1
	    UCA0TXBUF = letra;

	    while((UCA0IFG & UCRXIFG)==0);  //Esperar RXIFG = 1
	    rx[i++] = UCA0RXBUF;

	    letra++;
	    if(letra == ('Z'+1)){
	        i=0;
	        letra = 'a';        //faixa minuscula
	    }
	    if(letra == ('z'+1)){
            i=0;
            letra = 'A';        //faixa maiscula
        }
	}
	P3OUT |= BIT2;              //desabilita escravo
	return 0;
}

//Mestre
void USCI_A0_config(){
    UCA0CTL1 = UCSSEL__SMCLK | UCSWRST;       //RST = 1
    UCA0CTL0 = UCMST | UCSYNC;    // Mestre | Sincrono
    UCA0BRW = 11;                           //100 KHz aproximadamente

    P3SEL |= BIT4 | BIT3;                   //P3.3(SIMO) e P3.4(SOMI)
    P2SEL |= BIT7;                          //CLK
    P3DIR |= BIT2;                          //P3.2, controlar a hab do escravo
    P3OUT |= BIT2;                          //P3.2 = 1, desabilita escravo

    UCA0CTL1 = UCSSEL__SMCLK;               // RST=0 e SMCLK
}

//Slave
void USCI_B1_config(){
    UCB1CTL1 = UCSWRST;                     //RST=1 para USCI_A0
    UCB1CTL0 = UCMODE_2 | UCSYNC;           // SPI 4 Pinos (UCxSTE L) | Sincrono
    P4SEL |= BIT3 | BIT2 | BIT1 | BIT0;     //disponibiliza P4.3,2,1,0
    UCB1CTL1 = 0;
    UCB1IE = UCRXIE;                        //Interrupçao por recepçao
}

#pragma vector = USCI_B1_VECTOR
__interrupt void usci_b1_int(){
    UCB1IV;                                 //Apaga RXIFG
    UCB1TXBUF = UCB1RXBUF;                  //Transmite o que recebeu
}
