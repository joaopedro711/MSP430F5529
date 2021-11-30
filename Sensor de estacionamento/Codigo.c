//Sensor de estacionamento
/*
 * Tem um curto de P1.2 e P1.3 com o ECHO, para cada um deles pegar algum flanco na captura
 * P1.2 vai ser o timer de flanco de subida, logo quando come�ar  o echo
 * P1.3 eh o flanco de descida do echo
 * P2.0 eh o trigger
 * P2.5 eh o Buzzer
 */

#include <msp430.h> 

//macro para leds
#define LED1_ON               (P1OUT |= BIT0)
#define LED1_OFF             (P1OUT &= ~BIT0)

#define LED2_ON               (P4OUT |= BIT7)
#define LED2_OFF             (P4OUT &= ~BIT7)

//Macro para as chaves
#define CHAVE1_PRESSIONADA  ((P2IN & BIT1) == 0)
#define CHAVE2_PRESSIONADA  ((P1IN & BIT1) == 0)


//variaveis globais
volatile unsigned int t_inicio, t_fim, t_total, flag=0, flag2=0;
volatile unsigned int distancia;


void io_config();
void acende_leds();
void debounce();
int calc_freq();
void ta2_prog();

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    io_config();
//********************************************************************************************************
    //Timer PWM para o Trigger
    TA1CTL = TASSEL__ACLK |          //uso o ACLK
             MC__UP       |          //conta at� CCR0
             TACLR;                  //Clear o timer

    TA1CCR0 = 1639-1;                 //Teto de contagem, 1639-1, conta 20 Hz

    TA1CCR1 = TA1CCR0 / 2;          //Duty cicle em 50%
    TA1CCTL1 = OUTMOD_7;            //MOdo Reset/Set
//********************************************************************************************************

//********************************************************************************************************
    //Configura��o do ECHO
    //Conto o tempo do ECHO
    TA0CTL = TASSEL__SMCLK   |
             MC__CONTINUOUS;

    //Configuro o canal 1 pra ler o flanco de subida
    TA0CCTL1 = CM_1     |               //captura no flanco de subida
               CCIS_0   |               //Timer tipo A
               SCS      |               //captura sincrona
               CAP      |               //modo de captura
               CCIE;                    //habilito a interrup��o

    //Configuro o canal 2 pra ler o flanco de descida
    TA0CCTL2 = CM_2     |               //captura no flanco de descida
               CCIS_0   |               //Timer tipo A
               SCS      |               //captura sincrona
               CAP      |               //modo de captura
               CCIE;                    //habilito a interrup��o

    TA0CTL |= TACLR;                    //zero a contagem
//********************************************************************************************************

//********************************************************************************************************
    //Configurar o Timer PWM do Buzzer
    //TIMER A2.2
    TA2CTL = TASSEL__ACLK   |           //USo o ACLK, conta at� 2^15
             MC__UP         |           // USo o up, pra contar at� CCR0
             TACLR;                     //zero o timer

    TA2CCR0 = 7-1;                        //Teto de contagem da frquencia     //Esse valor ir� ser alterado conforme a distancia

    TA2CCR2 = TA2CCR0 >> 1;              //Duty cicle em 50%                    //esse valor vai ser alterado conforme a distancia
    TA2CCTL2 = OUTMOD_7;                //MOdo Reset/Set
//********************************************************************************************************

    //habilito as interrup��es
    __enable_interrupt();

   volatile unsigned int freq;

    while(1){
        //se entrou no flanco de descida, ou seja se pegou o ECHO
        if(flag == 1){
            if(t_fim > t_inicio){
               t_total = t_fim - t_inicio;
            }
            else{                                           //caso t_fim seja menor que t_inicio
                t_total = (0xFFFF - t_inicio) + t_fim;
            }
            distancia = t_total * 0.016212463;// 0,51...= 17000/32768;  17000 microseg, se refere a velocidade do som /2, pois o ECHO pega a distancia indo e voltando, porem s� quero 1 delas.
            flag = 0;       //volto a flag pra zero

            //Caso uma das chaves estiverem sendo pressionadas, o som do buzzer vai seguir a segunda figura
            //Quanto mais afastado, mais agudo fica
            if(CHAVE1_PRESSIONADA || CHAVE2_PRESSIONADA || (CHAVE1_PRESSIONADA && CHAVE2_PRESSIONADA)){

               acende_leds();
               flag2 = 1;
               freq = calc_freq(distancia);
               ta2_prog(freq);
               debounce();
            }

            //Caso n�o esteja sendo nada pressionado, vai seguir a primeira imagem, se a flag2 == 0;
            //Mais proximo a distancia, mais agudo fica, ou seja mais rapido a frequencia
            if(flag2 == 0){
                freq = calc_freq(distancia);
                ta2_prog(freq);

                acende_leds();
            }
                flag2 = 0;
        }
    }
    return 0;
}

//Interrup��o do timer A0
//ser� a interrup��o do p1.2 e P1.3
#pragma vector = TIMER0_A1_VECTOR
__interrupt void timer_isr_p13(){
    switch (TA0IV)
    {
    case TA0IV_NONE:
        break;
    case TA0IV_TA0CCR1:             //P1.2
        t_inicio = TA0CCR1;         //recebo o tempo do inicio, quando o flanco sobe
        break;
    case TA0IV_TA0CCR2:             //P1.3
        t_fim = TA0CCR2;            //recebo o tempo do fim, quando o flanco desce
        flag = 1;
        break;
    case TA0IV_TA0CCR3:
        break;
    case TA0IV_TA0CCR4:
        break;
    case TA0IV_TAIFG:
        break;
    default:
        break;
    }
}



void io_config(){
    //configurando o pino do trigger P2.0

    P2SEL |= BIT0;       //Usarei como From module
    P2DIR |= BIT0;      //como saida


    //From module pinos p1.2 e p1.3
    P1SEL |= BIT2;
    P1DIR &= ~BIT2;     //como entrada

    P1SEL |= BIT3;
    P1DIR &= ~BIT3;     //como entrada

    //Led1
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    LED1_OFF;

    //Led2
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    LED2_OFF;

    //configurando o pino do buzzer

    P2SEL |= BIT5;                   //Usarei como From module
    P2DIR |= BIT5;                   //como saida

    //chave 1
    P2SEL &= ~BIT1;                 //como gpio
    P2DIR &= ~BIT1;                  //como entrada
    P2REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P2OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0

    //chave 2
    P1SEL &= ~BIT1;               //como gpio
    P1DIR &= ~BIT1;               //como entrada
    P1REN   |=  BIT1;              //Habilita Resistor, como � entrada precisa de resistor
    P1OUT   |=  BIT1;              //Habilita PULLUP, quer dizer que o bot�o � ativo em 0
}

//Fun��o que acende os leds conforme a distancia
void acende_leds(){
    if(distancia < 10){
        LED2_ON;
        LED1_ON;
    }
    else if(distancia < 30 && distancia > 9){
        LED1_ON;
        LED2_OFF;
    }
    else if(distancia < 50 && distancia > 29){
        LED1_OFF;
        LED2_ON;
    }
    else{
        LED1_OFF;
        LED2_OFF;
    }
}

void debounce(){                        //Debouce para as chaves
    volatile unsigned int x = 50000;
    while (x--);
}

//calcula a frequencia que deve ficar o buzzer, porem depois na TA2_prog esse valor divide o ACLK
int calc_freq(int dist){
    //Indica que nenhuma chave est� pressionada
    if(flag2 == 0){
        if(dist >= 5 && dist <= 50){
            //Equa��o da reta
            return ( 5555.555556 - 111.111111111 * dist);
        }
        else
            return 0;
    }

    //Indica se alguma das chaves est� sendo pressionada
    if(flag2 == 1){
        if(dist >= 5 && dist <= 50){
            //Equa��o da reta, conforme a figura 2
            return (111.111111111 * dist - 555.555556);
        }
        else{
            return 0;
        }
    }
}

//Vai passar o valor para o teto de contagem, em que a ficar� mais agudo ou mais grave
void ta2_prog(int freq){
    volatile int x;
    x = 32768 / freq;

    TA2CCR0 = x;               //ACLK divide a freq, e ser� o teto de contagem
}
