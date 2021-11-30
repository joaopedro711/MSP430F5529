#include <msp430.h> 
#include <stdint.h>
/*
* Conexoes do LCD:
* SDA -> P3.0
* SCL -> P3.1
*
* Conexoes do micro servo
* fio pwm do micro servo no P7.4
*
* Conxoes do sensor de distancia
* Tem um curto de P1.2 e P1.3 com o ECHO, para cada um deles pegar algum flanco na captura
* P1.2 vai ser o timer de flanco de subida, logo quando come�ar  o echo
* P1.3 eh o flanco de descida do echo
* P2.0 eh o trigger
*
* LEDS EXTERNOS
* LED vermelho no P8.2
* LED amarelo no  P2.7
* LED Azul no     P1.6
*/
//************************************************* LEDS e CHAVE ***********************************************************
#define LEDRED_ON     (P8OUT |= BIT2)
#define LEDRED_OFF     (P8OUT &= ~BIT2)


#define LEDYellow_ON     (P2OUT |= BIT7)
#define LEDYELLOW_OFF     (P2OUT &= ~BIT7)

#define LEDBLUE_ON     (P1OUT |= BIT6)
#define LEDBLUE_OFF     (P1OUT &= ~BIT6)


#define CHAVE1_PRESSIONADA  ((P2IN & BIT1) == 0)

//************************************************* LCD *********************************************************************
volatile int count;    // Utilizada para controle do tempo

int line = 0x00;        // Utilizada para pular linha do LCD

// Declaracao  de tipos de unidades de tempo
typedef enum {us, ms, sec, min} timeunit_t;

// Definicao de Macros para o LCD
#define LCD 0x27

#define CHAR  1
#define INSTR 0

#define BT_ON  1
#define BT_OFF 0

#define BT  BIT3
#define EN  BIT2
#define RW  BIT1
#define RS  BIT0

// Cabecalhos das funcoes de timer
void wait(int time, timeunit_t unit);

// Cabecalhos das funcoes de I2C
void i2cConfig();
int i2cWrite(int addr, int * data, int nBytes);
int i2cRead(int addr, int * data, int nBytes);
int i2cWriteByte(int addr, int byte);
int i2cReadByte(int addr, int * data);

// Cabecalhos das funcoes de LCD
int lcdIsBusy();
void lcdInit();
void lcdClear();
void lcdWriteNibble(int nibble, int isChar);
void lcdWriteByte(int byte, int isChar);
void lcdCursor(int line, int column);
void lcdPrintChar(int ch);
void lcdPrint(uint8_t * str);

//****************************************************************************************************************************
//************************************************* SENSOR DE DISTANCIA ******************************************************
volatile unsigned int t_inicio=0, t_fim=0, t_total=0, flag=0, flag2=0, contador=0;
volatile unsigned int distancia=0;

void io_config();
void config_timers_sensor();
void debounce_sensor();
//****************************************************************************************************************************
//************************************************* MICRO SERVO **************************************************************
void microservo();
void debounce_micro();
//****************************************************************************************************************************

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	int flag3=0;                //serve pra printar apenas uma vez a msg "Mantenha a mao..."

	io_config();

    i2cConfig();
    lcdInit();

    config_timers_sensor();

    //habilito as interrupçoes
    __enable_interrupt();

    while(1){

        if(CHAVE1_PRESSIONADA){
            debounce_sensor();
            lcdClear();
            contador=0;
            lcdPrint("  MANUTENCION!");
            flag2 = 1;
            LEDRED_ON;
            LEDYellow_ON;
            LEDBLUE_ON;
            while(flag2 == 1){
                if(CHAVE1_PRESSIONADA){         //caso a chave seja pressionada novamente
                    debounce_sensor();
                    flag2 = 0;
                    LEDRED_OFF;
                    LEDYELLOW_OFF;
                    LEDBLUE_OFF;
                    TA0CTL |= TACLR;                    //zero a contagem
                    flag3=0;
                }
            }
        }
        //se entrou no flanco de descida, ou seja se pegou o ECHO
        if(flag2 == 0){

            if(flag == 1){
                if(t_fim > t_inicio){
                   t_total = t_fim - t_inicio;
                }
                else{                                           //caso t_fim seja menor que t_inicio
                    t_total = (0xFFFF - t_inicio) + t_fim;
                }
                distancia = t_total * 0.016212463;// 0,51...= 17000/32768;  17000 microseg, se refere a velocidade do som /2, pois o ECHO pega a distancia indo e voltando, porem s� quero 1 delas.

                if(distancia < 10){
                    contador ++ ;
                    flag3 = 0;
                    if(contador==1){
                        lcdClear();
                        LEDRED_OFF;
                        LEDYellow_ON;
                        LEDBLUE_OFF;
                        lcdPrint("   PROCESSING!");
                    }
                }
                else{
                    contador = 0 ;

                    if(flag3 == 0){
                        LEDRED_ON;
                        LEDYELLOW_OFF;
                        LEDBLUE_OFF;

                        lcdClear();
                        lcdPrint("Mantenha a mao 2");
                        lcdCursor(1, 0);                //pula pra linha 2
                        lcdPrint("seg na frente.");
                        debounce_micro();
                    }
                    flag3 ++;
                }
            }

            //quando passar cerca de 2 seg na frente do sensor em menos de 10 cm
            if(contador == 15){
                LEDRED_OFF;
                LEDYELLOW_OFF;
                LEDBLUE_ON;
                lcdClear();
                lcdPrint("    OBRIGADO!");

                microservo();                       //puxo o microservo
                debounce_sensor();                 //espero um tempo
                debounce_sensor();

                int i;
                for(i=0; i<30000; i++){
                    TA0CTL |= TACLR;                   //zero a contagem
                }
                contador = 0;
            }
            flag = 0;       //volto a flag pra zero
        }
    }


	return 0;
}
//*************************************************************I2C e LCD**********************************************************
// Configura o modo I2C
void i2cConfig() {
    UCB0CTL1  = UCSWRST;                    // Reseta a interface
    UCB0CTL0  = UCMST | UCMODE_3 | UCSYNC;  // Interface eh mestre, modo I2C sincrono
    UCB0CTL1 |= UCSSEL__SMCLK;              // Usa SMCLK eh 1MHz
    UCB0BRW   = 11;                         // SCL eh SMCLK / 11 ~ 100 KHz

    // Configura os pinos
    P3SEL |=   BIT0 | BIT1;
    P3REN |=   BIT0 | BIT1;
    P3OUT |=   BIT0 | BIT1;
    P3DIR &= ~(BIT0 | BIT1);

    // Desliga o reset da interface
    UCB0CTL1 &= ~UCSWRST;
}

/**************************** Inicializa o LCD********************************/
void lcdInit() {
    wait(15, ms);
    lcdWriteNibble(0x3, INSTR); // Garante que o LCD
    wait(5, ms);
    lcdWriteNibble(0x3, INSTR); // Esteja em modo de 8 bits
    wait(100, us);
    lcdWriteNibble(0x3, INSTR); // antes de ir para 4 bits

    lcdWriteNibble(0x2, INSTR); // Entra em modo de 4 bits
    while(lcdIsBusy());

    lcdWriteByte(0x28, INSTR);  // Modo 4 bits, 2 linhas
    while(lcdIsBusy());

    lcdWriteByte(0x08, INSTR);  // Desliga LCD
    while(lcdIsBusy());

    lcdWriteByte(0x01, INSTR);  // Limpa LCD
    while(lcdIsBusy());

    lcdWriteByte(0x06, INSTR);  // Cursor move para a direita
    while(lcdIsBusy());

    lcdWriteByte(0x0C, INSTR);  // Liga LCD
    while(lcdIsBusy());
}

// Timer atrelado a funcao wait do timer
#pragma vector = TIMER2_A0_VECTOR
__interrupt void TA2_CCR0_ISR() {
  count--;
}

/**************************************Funcoes de tempo**********************************/

// Funcao utilizada para esperar uma determinada quantidade de tempo
void wait(int time, timeunit_t unit) {
  if (unit == us) {
    // Use o SMCLK
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR;
    TA2CCR0 = time;

    while (!(TA2CCTL0 & CCIFG));
    TA2CCTL0 &= ~CCIFG;
  } else {
    // Use o ACLK
    TA2CTL = TASSEL__ACLK  | MC__UP | TACLR;

    if (unit == ms) {
      TA2CCR0 = (time << 5) - 1;

      while (!(TA2CCTL0 & CCIFG));

      TA2CCTL0 &= ~CCIFG;
    } else if (unit == sec) {
      count = time;

      TA2CCR0 = 0x8000 - 1;

      TA2CCTL0 = CCIE;
      while (count);
    } else if (unit == min) {
      count = time * 60;

      TA2CCR0 = 0x8000 - 1;

      TA2CCTL0 = CCIE;
      while (count);
    }
  }
  TA2CTL = MC__STOP;
}

/**********************************************Escrita no LCD*************************************/
// Escrita com I2C
int i2cWrite(int addr, int * data, int nBytes) {
    UCB0IFG = 0;

    UCB0I2CSA = addr;
    UCB0CTL1 |= UCTXSTT | UCTR;

    while (!(UCB0IFG & UCTXIFG));
    UCB0TXBUF = *data++;
    nBytes--;

    while(UCB0CTL1 & UCTXSTT);

    if (UCB0IFG & UCNACKIFG) {
        UCB0CTL1 |= UCTXSTP;
        while (UCB0CTL1 & UCTXSTP);
        return 1;
    }

    while (nBytes--) {
        while(!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = *data++;
    }

    while(!(UCB0IFG & UCTXIFG));

    UCB0CTL1 |= UCTXSTP;
    while (UCB0CTL1 & UCTXSTP);

    return 0;
}

// Leitura com I2C
int i2cRead(int addr, int * data, int nBytes) {
    UCB0IFG = 0;

    UCB0I2CSA =    addr;
    UCB0CTL1 &= ~(UCTR);
    UCB0CTL1 |= UCTXSTT;

    while(UCB0CTL1 & UCTXSTT);

    if (nBytes == 1) {
        UCB0CTL1 |= UCTXSTP;
    }

    if (UCB0IFG & UCNACKIFG) {
        UCB0CTL1 |= UCTXSTP;
        while (UCB0CTL1 & UCTXSTP);
        return 1;
    }

    while (nBytes) {
        while(!(UCB0IFG & UCRXIFG));

        *data = UCB0RXBUF;
        data++;
        nBytes--;

        if (nBytes == 1) {
            UCB0CTL1 |= UCTXSTP;
        }
    }

    while (UCB0CTL1 & UCTXSTP);

    return 0;
}

// Escrever um byte com I2C
int i2cWriteByte(int addr, int byte) {
    return i2cWrite(addr, &byte, 1);
}

// Ler um byte com I2C
int i2cReadByte(int addr, int * data) {
    return i2cRead(addr, data, 1);
}

// Verifica se o LCD est� ocupado
int lcdIsBusy() {
    int data;
    volatile int nibble = 0x0;
    nibble << 4;

    i2cWriteByte(LCD, nibble | BT | 0   | RW | INSTR);
    i2cWriteByte(LCD, nibble | BT | EN  | RW | INSTR);

    i2cReadByte(LCD, &data);

    i2cWriteByte(LCD, nibble | BT | 0   | RW | INSTR);
    return data & BIT7;
}

// Limpar o LCD
void lcdClear() {
    lcdWriteByte(0x01, INSTR);
    while(lcdIsBusy());
}

// Escrever um nibble no LCD
void lcdWriteNibble(int nibble, int isChar) {
    nibble <<= 4;
    //                         BT | EN | RW | RS
    i2cWriteByte(LCD, nibble | BT | 0  | 0 | isChar);
    i2cWriteByte(LCD, nibble | BT | EN | 0 | isChar);
    i2cWriteByte(LCD, nibble | BT | 0  | 0 | isChar);
}

// Escrever um byte no LCD
void lcdWriteByte(int byte, int isChar) {
    lcdWriteNibble(byte >> 4  , isChar);  // Manda o MSNibble
    lcdWriteNibble(byte & 0x0F, isChar);  // Manda o LSNibble
}

// Posiciona o cursor do LCD na posi��o desejada
void lcdCursor(int line, int column) {
    if (line) {
        lcdWriteByte(BIT7 | BIT6 + column, INSTR);
        while(lcdIsBusy());
    } else {
        lcdWriteByte(BIT7 + column, INSTR);
        while(lcdIsBusy());
    }
}

// Escreve um char no LCD
void lcdPrintChar(int ch) {
    if (ch == '\n') {
      line ^= BIT6;
      lcdWriteByte(BIT7 | line, INSTR);
      while(lcdIsBusy());
    } else {
      lcdWriteByte(ch, CHAR);
      while(lcdIsBusy());
    }
}

// Escreve uma string no LCD, 4 digitos
void lcdPrint(uint8_t * str) {
  while (*str) {
    lcdPrintChar(*str);
    str++;
  }
}

// Escreve uma string no LCD, 3 digitos
void lcdPrint3(uint8_t * str) {
  while (*str) {
    lcdPrintChar(*str);
    str++;
  }
}
//*********************************************************************************************************************************************

//**********************************************SENSOR DE DISTANCIA **************************************************************************

void config_timers_sensor(){
    //********************************************************************************************************
    //Timer PWM para o Trigger
    TA1CTL = TASSEL__ACLK |          //uso o ACLK
             MC__UP       |          //conta até CCR0
             TACLR;                  //Clear o timer

    TA1CCR0 = 1639-1;                 //Teto de contagem, 1639-1, conta 20 Hz

    TA1CCR1 = TA1CCR0 / 2;          //Duty cicle em 50%
    TA1CCTL1 = OUTMOD_7;            //MOdo Reset/Set
//**********************************************************************
    //Configuração do ECHO

    //Conto o tempo do ECHO
    TA0CTL = TASSEL__SMCLK   |
             MC__CONTINUOUS;

    //Configuro o canal 1 pra ler o flanco de subida
    TA0CCTL1 = CM_1     |               //captura no flanco de subida
               CCIS_0   |               //Timer tipo A
               SCS      |               //captura sincrona
               CAP      |               //modo de captura
               CCIE;                    //habilito a interrupcao

    //Configuro o canal 2 pra ler o flanco de descida
    TA0CCTL2 = CM_2     |               //captura no flanco de descida
               CCIS_0   |               //Timer tipo A
               SCS      |               //captura sincrona
               CAP      |               //modo de captura
               CCIE;                    //habilito a interrupcao

    TA0CTL |= TACLR;                    //zero a contagem
//******************************************************************************
}

//Interrupcao do timer A0
//Será as interrupções do P1.2 e P1.3
#pragma vector = TIMER0_A1_VECTOR
__interrupt void timer_isr_p1(){
    switch (TA0IV){
    case TA0IV_NONE:
        break;
    case TA0IV_TA0CCR1:             //P1.2
        t_inicio = TA0CCR1;         //recebo o tempo do inicio, quando o flanco sobe
        break;
    case TA0IV_TA0CCR2:             //P1.3
        t_fim = TA0CCR2;            //recebo o tempo do fim, quando o flanco desce
        flag = 1;                   //quando pego o flanco de descida
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

    //LedRED
    P8SEL &= ~BIT2;     //como GPIO
    P8DIR |= BIT2;      //saida
    LEDRED_OFF;

    //LedYELLOW
    P2SEL &= ~BIT7;     //como GPIO
    P2DIR |= BIT7;      //saida
    LEDYELLOW_OFF;

    //LedBLUE
    P1SEL &= ~BIT6;     //como GPIO
    P1DIR |= BIT6;      //saida
    LEDBLUE_OFF;

    //chave 1
    P2SEL &= ~BIT1;                 //como gpio
    P2DIR &= ~BIT1;                 //como entrada
    P2REN   |=  BIT1;               //Habilita Resistor,como  entrada precisa de resistor
    P2OUT   |=  BIT1;               //Habilita PULLUP, quer dizer que o botao ativo em 0
}

void debounce_sensor(){                        //espera um tempo
    volatile unsigned int x = 100000;
    while (x--);
}
//******************************************************** MICRO SERVO *******************************************************************************


void microservo(){
    //config pino do microservo
    P7SEL |= BIT4;              //como from module
    P7DIR |= BIT4;              //Como saida

    //Configurar o Timer PWM do Micro servo
    //TIMER B0.2

    TB0CTL = TBSSEL__ACLK   |          //USo o ACLK, conta até 2^15
             MC__UP         |           // USo o up, pra contar at� CCR0
             TBCLR;                     //zero o timer

    TB0CCR0 = 655;                    //Teto de contagem da frquencia     //periodo de 20 milisegundos

    TB0CCR2 = 16;                      //Duty cicle para posição de 90 graus
    TB0CCTL2 = OUTMOD_7;                //Modo Reset/Set

    debounce_micro();

    TB0CCR2 = 47;                       //Duty cicle para posição de 180 graus

    debounce_micro();
}

void debounce_micro(){            //tempo pro servo
    volatile unsigned int x = 50000;
    while (x--);
}
