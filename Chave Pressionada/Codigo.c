//Qual chave foi pressionada primeiro

#include <msp430.h> 

#define LED1_ON             (P1OUT |= BIT0)
#define LED1_OFF            (P1OUT &= ~BIT0)

#define LED2_ON             (P4OUT |= BIT7)
#define LED2_OFF            (P4OUT &= ~BIT7)

#define CHAVE1_PRESSIONADA  ((P2IN & BIT1) == 0)
#define CHAVE2_PRESSIONADA  ((P1IN & BIT1) == 0)

unsigned int acendeu_led1 = 0, acendeu_led2=0;            //variaveis globais que diz se acendeu cada led

//Funcao que configura os botoes e leds
void io_config();
void config_chaves();
void debounce();



int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    io_config();                //Chama a fun��o que configura o GPIO
    config_chaves();            //Chama a fun��o que configura as chaves

    __enable_interrupt();       //Habilita as interrup��es

    while(1){

        if((acendeu_led2 == 1) || (acendeu_led1==1)){       //Caso tenha entrado na interrup��o
            if(((P1OUT & BIT0)== 0) || ((P4OUT & BIT7)== 0)){ //Se algum dos leds est�o acesos
                debounce();

            }


            if((!(CHAVE1_PRESSIONADA)) && (!(CHAVE2_PRESSIONADA))){
                acendeu_led2=0;
                acendeu_led1=0;
                LED1_OFF;
                LED2_OFF;

                debounce();

            }
        }

    }
    return 0;
}





void io_config(void){
    //Configurando os Leds como Sa�da
    P1SEL   &=  ~BIT0;             //Coloco como GPIO
    P1DIR   |=  BIT0;              //Led1,coloca 1, ou seja, Configura como Sa�da de dados
    P1OUT   &=  ~BIT0;             //Inicia o Led 1, apagado, pois est� em 0

    P4SEL   &=  ~BIT7;             //Coloco como GPIO
    P4DIR   |=  BIT7;              //Led2,coloca 1, ou seja, Configura como Sa�da de dados
    P4OUT   &=  ~BIT7;             //Inicia o Led 2, apagado, pois est� em 0

    //Configura os bot�es como entrada
    P2SEL   &=  ~BIT1;             //Coloco como GPIO
    P2DIR   &= ~BIT1;              //Bot�o 1, coloca 0, ou seja entrada de dados
    P2REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P2OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0

    P1SEL   &=  ~BIT1;             //Coloco como GPIO
    P1DIR   &= ~BIT1;              //Bot�o 2, coloca 0, ou seja entrada de dados
    P1REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P1OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0
}

//Configurando chaves para interrup��o
void config_chaves(void){
    P2SEL   &=  ~BIT1;             //Coloco como GPIO
    P2DIR   &= ~BIT1;              //Bot�o 1, coloca 0, ou seja entrada de dados
    P2REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P2OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0

    P2IE    |=  BIT1;              //Ativar interrup��o
    P2IES   |=  BIT1;              //flanco de descida
    //P2IES   &=  ~BIT1;

   do {                            //Zero as flags de interrup��es
       P2IFG = 0;
   } while (P2IFG != 0);

    //Configura chave 2
    P1SEL   &=  ~BIT1;             //Coloco como GPIO
    P1DIR   &= ~BIT1;              //Bot�o 1, coloca 0, ou seja entrada de dados
    P1REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P1OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0

    P1IE    |=  BIT1;              //Ativar interrup��o
    P1IES   |=  BIT1;              //flanco de descida
    //P1IES   &=  ~BIT1;

    do {                           //Zero as flags de interrup��es
        P1IFG = 0;
    } while (P1IFG != 0);
}


//ISR para a porta P2.1
#pragma vector = PORT2_VECTOR;
__interrupt void minha_isr1(void){
    switch (P2IV){

    case 0x04:                         //P2.1, caso o bot�o seja precionado

        acendeu_led1=1;
        if((acendeu_led2 == 0 ) && (acendeu_led1==1)){
            LED1_ON;           //acendo o led 1, caso bot�o 1 seja pressionado
        }
        break;

    default:
        break;

    }
}
//ISR para a porta P1.1
#pragma vector = PORT1_VECTOR;
__interrupt void minha_isr2(void){
    switch (P1IV){
    case 0x04:                         //P1.1, caso o bot�o seja pressionado
        acendeu_led2=1;
        if((acendeu_led2 == 1 ) && (acendeu_led1==0)){
            LED2_ON;           //acendo o led 2, caso bot�o 2 seja pressionado
        }

        break;

    default:
        break;

    }
}

void debounce(){
    volatile unsigned int x = 50000;
    while (x--);
}
