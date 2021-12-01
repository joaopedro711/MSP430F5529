//SPI - leitura dos flancos e mostrar no LCD

#include <msp430.h>
#include <stdint.h>

uint8_t i2cSendByte(uint8_t addr, uint8_t byte);
uint8_t i2cGetByte(uint8_t addr);
void lcdWriteNibble(uint8_t nibble, uint8_t rs);
void lcdWriteByte(uint8_t byte    , uint8_t rs);
void lcdInit();
void print(char *str);
void lcdHex16 (uint16_t v);
void lcdHex8 (uint8_t n);
void lcdDec16 (uint16_t d);
void lcdFloat1 (float f);
void print2(char *str);
void configTimer1Sec();
void configTimerA0();
void configPinos();
void i2cInit();
void intToChar(uint16_t v, char string[], uint8_t init);
void continuePrinting(char *str);
uint8_t spiTransfer(uint8_t byte);
void spiConfig(uint8_t pol, uint8_t pha, uint8_t MSBfirst, uint8_t isMaster);

unsigned char ordem = 5; //0 P1.4 ocorreu primeiro, 1 P1.5 primeiro,2 nenhum ocorreu.
unsigned int tempo_p14_us; //Duracao do pulso em P1.4, em milissegundos
unsigned int tempo_p15_us; //Duracao do pulso em P1.5, em milissegundos

uint8_t trigger = 0, responseSlave, checkIn = 0;
uint16_t timer;
char tempoP14Char[3];
char tempoP15Char[3];

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    i2cInit();
    lcdInit();
    configTimer1Sec();
    configTimerA0();
    configPinos();
    print("P14 T=   ms     ");
    print2("P15 T=   ms     ");
    spiConfig(0 , 0, 1, 1);

    __enable_interrupt();

    while(1)
    {

        if(trigger){             //Para o programa rodar a cada 1 segundo

            lcdWriteByte(0x86, 0);
            intToChar(tempo_p14_us, tempoP14Char, 0);
            continuePrinting(tempoP14Char);
            lcdWriteByte(0xC6, 0);
            intToChar(tempo_p15_us, tempoP15Char, 0);
            continuePrinting(tempoP15Char);

            if((tempo_p14_us < tempo_p15_us) && (tempo_p14_us > 0)){
                ordem = 0;
            }
            else if(tempo_p15_us == 0 && tempo_p14_us >0){
                ordem = 0;
            }
            else if(tempo_p14_us == 0 && tempo_p15_us >0){
                ordem = 1;
            }
            else if((tempo_p15_us < tempo_p14_us) && (tempo_p15_us > 0)){
                ordem = 1;
            }

            else if(tempo_p15_us ==  0 && tempo_p14_us == 0){
                ordem = 2;
            }
            else if(tempo_p14_us == tempo_p15_us){
                ordem = 2;
            }

            if(ordem == 0){
                  lcdWriteByte(0xCE, 0);
                  continuePrinting("  ");

                lcdWriteByte(0x8E, 0);
                continuePrinting("#1");
            }
            else if(ordem == 1){
                  lcdWriteByte(0x8E, 0);
                  continuePrinting("  ");

                lcdWriteByte(0xCE, 0);
                continuePrinting("#1");
            }
             else{
                lcdWriteByte(0x8E, 0);
                continuePrinting("  ");
               lcdWriteByte(0xCE, 0);
               continuePrinting("  ");
            }

             responseSlave = spiTransfer(ordem);
             tempo_p14_us = tempo_p14_us/4;
             tempo_p15_us = tempo_p15_us/4;
              responseSlave = spiTransfer(tempo_p14_us & 0xF0 );      //Envia os bits mais significativos de tempo_p14_us
              responseSlave = spiTransfer(tempo_p14_us & 0x0F );      //Envia LSB
              responseSlave = spiTransfer(tempo_p15_us & 0xF0 );
              responseSlave = spiTransfer(tempo_p15_us & 0x0F );

            trigger = 0;
            tempo_p14_us = 0;
            tempo_p15_us = 0;

        }


    }

    return 0;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void TIMER2_ISR(){
    trigger = 1;


}


#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMERA0_ISR(){
    switch (TA0IV){
        case 0x06:

            tempo_p14_us = TA0CCR3/32;      //Coloca o valor em milisegundo


            break;

        case 0x08:


            tempo_p15_us = TA0CCR4/32;      //Coloca o valor em milisec (TA0CCRx/ 31,768 = 32)


        case 0x0E:


        default:
            break;
    }

}

void intToChar(uint16_t v, char string[], uint8_t init){
    volatile int d1,d2,d3;
    uint16_t numero;

        numero = v;

        d1 = numero % 10;
        d2 = (numero % 100) / 10;
        d3 = (numero% 1000) /100;
        string[init] = d3 +'0';
        string[init + 1] = d2 +'0';
        string[init + 2] = d1 +'0';
}

void configTimerA0(){
    TA0CTL = TASSEL__ACLK  |   MC__UP  | TACLR | TAIE;
    TA0CCR0 = 25414;                //1ms = 10^3*fclk-1 = limiar = 31,768 | 800ms  = 31,768*800 = 25414,4 = 25414
    TA0CCTL3 = CCIE | CAP | CM_2;
    TA0CCTL4 = CCIE | CAP | CM_1;
}

void configPinos(){
    P1SEL |= BIT5;
    P1DIR &= ~BIT5;
    P1IN  &= ~BIT5;

    P1SEL |= BIT4;
    P1DIR &= ~BIT4;
    P1IN  |= BIT4;


    P1SEL &= ~BIT3;
    P1DIR |= BIT3;
    P1OUT &= ~BIT3;

    P1SEL &= ~BIT2;
    P1DIR |= BIT2;
    P1OUT &= ~BIT2;

}

void configTimer1Sec(){
    TA2CTL = TASSEL__ACLK | MC__UP | TACLR;
    TA2CCR0 = 0x8000;                   //Conta 1s
    TA2CCTL0 = CCIE;
}

void spiConfig(uint8_t pol, uint8_t pha, uint8_t MSBfirst, uint8_t isMaster){
    UCB1CTL1 = UCSWRST;                 //Reseta a interface
    UCB1CTL0 = UCMODE_0 | UCSYNC;       //Modo SPI de 3 fios

    if(isMaster){
        UCB1CTL1 |= UCSSEL__SMCLK;
        UCB1BRW    =1;
        UCB1CTL0 |= UCMST;
    }

    if(pol)
        UCB1CTL0 |= UCCKPL;
    if(pha)
        UCB1CTL0 |= UCCKPH;
    if(MSBfirst)
        UCB1CTL0 |= UCMSB;
    P4SEL |= BIT1 | BIT2 | BIT3;

    UCB1CTL1 &= ~UCSWRST;
}

uint8_t spiTransfer(uint8_t byte){
    while(!(UCB1IFG & UCTXIFG));        //Espera o buffer de transmissao ficar vazio
    UCB1TXBUF  = byte;

    while(!(UCB1IFG & UCRXIFG));        //Espera o buffer de recep��o ficar cehio
    return UCB1RXBUF;                   //Para fazer a leitura do buffer
}

void i2cInit(){
    UCB0CTL1   = UCSWRST;                              //Reseta a interface
    UCB0CTLW0 |= 0x0F00 | UCSSEL__SMCLK;              //Configura como mestre
    UCB0BRW    = 100;
    P3SEL     |= (BIT0 | BIT1);
    UCB0CTL1  &= ~UCSWRST;
}

uint8_t i2cSend(uint8_t addr, uint8_t * data, uint8_t nBytes){
    UCB0I2CSA  = addr;               //Configura o endere�o do escravo
    UCB0CTLW0 |= UCTXSTT | UCTR;     //Trasnmite o endere�o

    while(!(UCB0IFG & UCTXIFG));      //Espera o start acontecer
    UCB0TXBUF = *data++;              //Escreve o primeiro byte
    nBytes--;

    while(!(UCB0IFG & UCTXIFG));      //Aguarda ACK/NACK
    if(UCB0IFG & UCNACKIFG){
        UCB0CTLW0 |= UCTXSTP;         //Caso nao tenha escravo, pede stop e espera ser enviado para retornar
        while(UCB0CTLW0 & UCTXSTP);
        return 1;
    }

    while(nBytes--){                     //Recebedno ACK e continuando a comunica�ao                                   //Enviando os proximos bytes at� TXBUF ficar vazio
        while(!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = *data++;
    }

    while(!(UCB0IFG &UCTXIFG));         //Espera o envio do ultimo byte
    UCB0CTLW0 |= UCTXSTP;              //Para pedir stop

    while(UCB0CTLW0 & UCTXSTP);        //Espera o stop ser enviado para poder retornar

    return 0;
}

uint8_t i2cGet(uint8_t addr, uint8_t * data, uint8_t nBytes){
    UCB0I2CSA  = addr;               //Configura o endere�o do escravo
    UCB0CTLW0 &= ~UCTR;               //Receptor
    UCB0CTLW0 |= UCTXSTT;

    while(UCB0CTLW0 & UCTXSTT);        //Espera o ciclo do ACK

    if(UCB0CTLW0 & UCNACKIFG){           //Caso seja for nack
        UCB0CTLW0 |= UCTXSTP;           //Pede para parar
        return 1;
    }

    while(nBytes--){
        while(!(UCB0IFG &UCRXIFG));
        *data++ = UCB0RXBUF;
    }

    UCB0CTLW0 |= UCTXSTP;           //Pede para parar antes do ultimo envio

    while(UCB0CTLW0 & UCTXSTP);

    return 0;
}

uint8_t lcdAddr = 0x27;
uint8_t contador = 0;

#define EN BIT2
#define RW BIT1
#define BT BIT3

void print(char *str){

    lcdWriteByte(0x80, 0);

    while(*str){
        lcdWriteByte(*str++, 1);
    }
}

void print2(char *str){
    lcdWriteByte(0xC0, 0);

    while(*str){
        lcdWriteByte(*str++, 1);

    }
}

void continuePrinting(char *str){
    while(*str){
        lcdWriteByte(*str++, 1);

    }
}


void lcdWriteNibble(uint8_t nibble, uint8_t rs){
    i2cSendByte(lcdAddr, nibble << 4 | BT | 0  |rs );
    i2cSendByte(lcdAddr, nibble << 4 | BT | EN |rs );
    i2cSendByte(lcdAddr, nibble << 4 | BT | 0  |rs );

}


void lcdWriteByte(uint8_t byte, uint8_t rs){
    lcdWriteNibble(byte >> 4    , rs);
    lcdWriteNibble(byte & 0x0F  , rs);
}


void lcdInit(){

    if(i2cSendByte(lcdAddr, 0))
        lcdAddr = 0x3F;

    lcdWriteNibble(0x3,0);              //Colocando em modo 8 bits
    lcdWriteNibble(0x3,0);
    lcdWriteNibble(0x3,0);

    lcdWriteNibble(0x2,0);              //Colocando em modo 4bits

    lcdWriteByte(0x0C, 0);              //Liga o cursor e o display e faz o cursor psicar

    lcdWriteByte(0x02, 0);
    lcdWriteByte(0x01, 0);

}

uint8_t i2cSendByte (uint8_t addr, uint8_t byte){
    return i2cSend(addr, &byte, 1);
}

uint8_t i2cGetByte(uint8_t addr){
    uint8_t byte;
    i2cGet(addr, &byte,1);
    return byte;
}
