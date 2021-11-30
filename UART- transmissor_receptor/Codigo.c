//UART _ Transmissor e Receptor

#include <msp430.h> 

#define LEDRED_ON     (P1OUT |= BIT0)
#define LEDRED_OFF     (P1OUT &= ~BIT0)
#define LEDRED_TOGGLE   (P1OUT ^= BIT0)

#define LEDVERDE_ON     (P4OUT |= BIT7)
#define LEDVERDE_OFF     (P4OUT &= ~BIT7)
#define LEDVERDE_TOGGLE   (P4OUT ^= BIT7)

#define CHAVE1_PRESSIONADA  ((P2IN & BIT1) == 0)
#define CHAVE2_PRESSIONADA  ((P1IN & BIT1) == 0)

#define FLLN(x) ((x)-1)
void clockInit();
void pmmVCore (unsigned int level);


void io_config();
void initializeUART_UCA0();
void initializeUART_UCA1();

unsigned int rx_buffer;    //recebe o bayte transmitido por TXD
unsigned int acertou1 = 0, acertou2 = 0;    //Quando for igual a 3, acende/apaga o ledRED ou LedVERDE
        //acertou1 == chave 1; acertou2 == chave 2
unsigned int flag = 3; //qualquer valor
int vet1[3], vet2[3], i=0, vet1_certo[3], vet2_certo[3];
unsigned int transmitiu = 0, recebeu = 0;


int main(void)
{

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    clockInit();

    //vetores com padroes certos
    vet1_certo[0]= 0xE7;
    vet1_certo[1]= 0x49;
    vet1_certo[2]= 0x07;

    vet2_certo[0]= 0xB2;
    vet2_certo[1]= 0x38;
    vet2_certo[2]= 0xFA;

    //vetores que vao ser enviados
    vet1[0]= 0xE7;
    vet1[1]= 0x49;
    vet1[2]= 0x07;

    vet2[0]= 0xB2;
    vet2[1]= 0x38;
    vet2[2]= 0xFA;

    io_config();
    initializeUART_UCA0();
    initializeUART_UCA1();

    __enable_interrupt();

    while(1){
        i=0, acertou1=0,acertou2=0;

        if(CHAVE1_PRESSIONADA){
            _delay_cycles(100000);
            flag = 0;                   //foi a chave 1 que foi apertada

            for(i=0; i<3; i++){
               recebeu = 0;
               transmitiu = 0;

               UCA0TXBUF = vet1[i];
               while((recebeu && transmitiu)==0);       //Enquanto n�o tiver recebido nem transmitido
            }

            if(acertou1 == 3)   //acertou sequencia
                LEDRED_ON;

            else if(acertou1 != 3)   //errou sequencia
                LEDRED_OFF;

            acertou1 = 0;
        }

        else if(CHAVE2_PRESSIONADA){
            _delay_cycles(100000);
            flag = 1;                   //foi a chave 2 que foi apertada

            for(i=0; i<3; i++){
                recebeu = 0;
                transmitiu = 0;

                UCA0TXBUF = vet2[i];
                while((recebeu && transmitiu)==0);       //Enquanto n�o tiver recebido nem transmitido
            }

            if(acertou2 == 3)   //acertou sequencia
                LEDVERDE_ON;

            else if(acertou2 != 3)   //errou sequencia
                LEDVERDE_OFF;

            acertou2 = 0;
        }
    }

    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void pmmVCore (unsigned int level)
{
#if defined (__MSP430F5529__)
    PMMCTL0_H = 0xA5;                       // Open PMM registers for write access

    SVSMHCTL =                              // Set SVS/SVM high side new level
            SVSHE            +
            SVSHRVL0 * level +
            SVMHE            +
            SVSMHRRL0 * level;

    SVSMLCTL =                              // Set SVM low side to new level
            SVSLE            +
//          SVSLRVL0 * level +              // but not SVS, not yet..
            SVMLE            +
            SVSMLRRL0 * level;

    while ((PMMIFG & SVSMLDLYIFG) == 0);    // Wait till SVM is settled

    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);      // Clear already set flags

    PMMCTL0_L = PMMCOREV0 * level;          // Set VCore to new level

    if ((PMMIFG & SVMLIFG))                 // Wait till new level reached
        while ((PMMIFG & SVMLVLRIFG) == 0);

    SVSMLCTL =                              // Set SVS/SVM low side to new level
            SVSLE            +
            SVSLRVL0 * level +
            SVMLE            +
            SVSMLRRL0 * level;

    PMMCTL0_H = 0x00;                       // Lock PMM registers for write access
#endif
}

void clockInit()
{
    pmmVCore(1);
    pmmVCore(2);
    pmmVCore(3);

    P5SEL |= BIT2 | BIT3 | BIT4 | BIT5;
    UCSCTL0 = 0x00;
    UCSCTL1 = DCORSEL_5;
    UCSCTL2 = FLLD__1 | FLLN(25);
    UCSCTL3 = SELREF__XT2CLK | FLLREFDIV__4;
    UCSCTL6 = XT2DRIVE_2 | XT1DRIVE_2 | XCAP_3;
    UCSCTL7 = 0;                            // Clear XT2,XT1,DCO fault flags

    do {                                    // Check if all clocks are oscillating
      UCSCTL7 &= ~(   XT2OFFG |             // Try to clear XT2,XT1,DCO fault flags,
                    XT1LFOFFG |             // system fault flags and check if
                       DCOFFG );            // oscillators are still faulty
      SFRIFG1 &= ~OFIFG;                    //
    } while (SFRIFG1 & OFIFG);              // Exit only when everything is ok

    UCSCTL5 = DIVPA_1 | DIVA_0 | DIVM_0; //Divide ACLK por 2.

    UCSCTL4 = SELA__XT1CLK    |             // ACLK  = XT1   =>      16.384 Hz
              SELS__XT2CLK    |             // SMCLK = XT2   =>   4.000.000 Hz
              SELM__DCOCLK;                 // MCLK  = DCO   =>  25.000.000 Hz

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void io_config(){
    //Led1
    P1SEL &= ~BIT0;     //como GPIO
    P1DIR |= BIT0;      //saida
    LEDRED_OFF;

    //Led2
    P4SEL &= ~BIT7;     //como GPIO
    P4DIR |= BIT7;      //saida
    LEDVERDE_OFF;

    //chave 1
    P2SEL &= ~BIT1;                 //como gpio
    P2DIR &= ~BIT1;                 //como entrada
    P2REN   |=  BIT1;               //Habilita Resistor,como  entrada precisa de resistor
    P2OUT   |=  BIT1;               //Habilita PULLUP, quer dizer que o botao ativo em 0

    //chave 2
    P1SEL &= ~BIT1;                 //como gpio
    P1DIR &= ~BIT1;                 //como entrada
    P1REN   |=  BIT1;               //Habilita Resistor,como  entrada precisa de resistor
    P1OUT   |=  BIT1;               //Habilita PULLUP, quer dizer que o botao ativo em 0

}

//Transmissao
void initializeUART_UCA0(){
    //TXD
    P3DIR |= BIT3;
    P3SEL |= BIT3;

    //Desliga o m�dulo
    UCA0CTL1 |= UCSWRST;

    UCA0CTL0 = UCPEN |    //Parity enable: 1=ON, 0=OFF
               //UCPAR |    //Parity: 0:ODD, 1:EVEN
               //UCMSB |    //LSB First: 0, MSB First:1
               //UC7BIT |   //8bit Data: 0, 7bit Data:1
               UCSPB |    //StopBit: 0:1 Stop Bit, 1: 2 Stop Bits
               UCMODE_0 | //USCI Mode: 00:UART, 01:Idle-LineMultiprocessor, 10:AddressLine Multiprocessor, 11: UART with automatic baud rate
               //UCSYNC    //0:Assynchronous Mode, 1:Synchronous Mode
               0;

    UCA0CTL1 = UCSSEL__SMCLK | //00:UCAxCLK, 01:ACLK, 10:SMCLK, 11:SMCLK
               //UCRXEIE     | //Erroneous Character IE
               //UCBRKIE     | //Break Character IE
               //UCDORM      | //0:NotDormant, 1:Dormant
               //UCTXADDR    | //Next frame: 0:data, 1:address
               //UCTXBRK     | //TransmitBreak
               UCSWRST;        //Mant�m reset.

    //BaudRate: 57600
    //BRCLK  = 4 MHz
    //UCBRx  = 4
    //UCBRSx = 5
    //UCBRFx = 3
    //UCOS16 = 1

    UCA0MCTL = UCBRF_3 |    //Modulation Stage. Ignored when UCOS16 = 0
               UCBRS_5 |    //sexta Modulation stage
               UCOS16 |     //Oversampling Mode. 0:disabled, 1:enabled.
               0;

    UCA0BR0 = (4 & 0x00FF); //Low byte
    UCA0BR1 = (4 >> 8);     //High byte

    //Liga o m�dulo
    UCA0CTL1 &= ~UCSWRST;

    UCA0IE =   UCTXIE | //Interrupt on transmission
             //  UCRXIE |   //Interrupt on Reception
               0;
}

//REcepcao
void initializeUART_UCA1(){
    //RXD
    P4SEL |= BIT2; //Disponibilizar P4.2
    PMAPKEYID = 0X02D52; //Liberar mapeamento de P4
    P4MAP2 = PM_UCA1RXD; //P4.2 = RXD

    //Desliga o m�dulo
    UCA1CTL1 |= UCSWRST;

    UCA1CTL0 = UCPEN |    //Parity enable: 1=ON, 0=OFF
               //UCPAR |    //Parity: 0:ODD, 1:EVEN
               //UCMSB |    //LSB First: 0, MSB First:1
               //UC7BIT |   //8bit Data: 0, 7bit Data:1
               UCSPB |    //StopBit: 0:1 Stop Bit, 1: 2 Stop Bits
               UCMODE_0 | //USCI Mode: 00:UART, 01:Idle-LineMultiprocessor, 10:AddressLine Multiprocessor, 11: UART with automatic baud rate
               //UCSYNC    //0:Assynchronous Mode, 1:Synchronous Mode
               0;

    UCA1CTL1 = UCSSEL__SMCLK | //00:UCAxCLK, 01:ACLK, 10:SMCLK, 11:SMCLK
               //UCRXEIE     | //Erroneous Character IE
               //UCBRKIE     | //Break Character IE
               //UCDORM      | //0:NotDormant, 1:Dormant
               //UCTXADDR    | //Next frame: 0:data, 1:address
               //UCTXBRK     | //TransmitBreak
               UCSWRST;        //Mant�m reset.

    //BaudRate: 57600
    //BRCLK  = 4 MHz
    //UCBRx  = 4
    //UCBRSx = 5
    //UCBRFx = 3
    //UCOS16 = 1

    UCA1MCTL = UCBRF_3 |    //Modulation Stage. Ignored when UCOS16 = 0
               UCBRS_5 |    //sexta Modulation stage
               UCOS16 |     //Oversampling Mode. 0:disabled, 1:enabled.
               0;

    UCA1BR0 = (4 & 0x00FF); //Low byte
    UCA1BR1 = (4 >> 8);     //High byte

    //Liga o m�dulo
    UCA1CTL1 &= ~UCSWRST;

    UCA1IE =   UCRXIE |   //Interrupt on Reception
               0;
}




//INTERRUPT de Transmissao
#pragma vector = USCI_A0_VECTOR
__interrupt void UART0_INTERRUPT(void)
{
    switch(_even_in_range(UCA0IV,4))
    {
        case 0: break;
        case 2:  //Reception Interrupt

            break;
        case 4: //Transmissao interrupt
            transmitiu = 1;                 //transmitiu
            break;
        default: break;
    }
}


//INTERRUPT de RECEPCAO
#pragma vector = USCI_A1_VECTOR
__interrupt void UART_INTERRUPT(void)
{
    switch(_even_in_range(UCA1IV,4))
    {
        case 0: break;
        case 2:  //Reception Interrupt
            recebeu = 1;
            rx_buffer = UCA1RXBUF;              //rx_buffer recebe o valor que o UCA1 recebeu

            if(flag == 0){                          //caso o bot�o 1 tenha sido apertado
                if((transmitiu && recebeu)==1){
                    if(rx_buffer == vet1_certo[i])
                        acertou1++;
                }
            }

            if(flag == 1){                          //caso o bot�o 2 tenha sido apertado
                if((transmitiu && recebeu)==1){
                    if(rx_buffer == vet2_certo[i])
                        acertou2++;
                }
            }

            break;
        case 4: //Transmissao interrupt
            break;
        default: break;
    }
}

