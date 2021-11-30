//Jogo Genius

#include <msp430.h> 

#define LED1_ON             (P1OUT |= BIT0)
#define LED1_OFF            (P1OUT &= ~BIT0)
#define LED1_TOGGLE         (P1OUT ^= BIT0)

#define LED2_ON             (P4OUT |= BIT7)
#define LED2_OFF            (P4OUT &= ~BIT7)
#define LED2_TOGGLE         (P4OUT ^= BIT7)

#define CHAVE1_PRESSIONADA  ((P2IN & BIT1) == 0)
#define CHAVE2_PRESSIONADA  ((P1IN & BIT1) == 0)

unsigned int sequence[10];

//Funcao que configura os botoes e leds
void io_config();
void debounce();

//Fun��es do Jogo
void start();


void createSequence();
unsigned int vetor[16] = {1,0,1,0,1,1,0,0,1,1,1,0,0,0,0,1};
unsigned int b;


void showSequence();

int game();
unsigned int acertou_sequencia;
unsigned int x=0;             //variavel que veh o indice que tem que comparar
//questao 2e
void youwon(); //Sequ�ncia correta
void youlose(); //Sequ�ncia Incorreta


unsigned int n = 2;       //come�a com 2, pois eh o inicio



int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    io_config();                //Chama a função que configura GPIO



    int proxima_fase=0;
    while(1){
        if(n == 2){
            proxima_fase=0;
            start();
            tempo();                    //da um debounce antes de piscar os leds
        }


        createSequence(n);              //Chamo a fun��o de criar sequenca, cada tentativa vai ser uma sequenci aleatoria;
        showSequence(n);

        acertou_sequencia = game(n);

        //acertou eh a variavel global que a funcao game retorna
        if(acertou_sequencia == 1){
            youwon(); //Sequ�ncia correta
            tempo();
            proxima_fase = 1;
        }
        if(acertou_sequencia==0){
            youlose(); //Sequ�ncia Incorreta
            tempo();
            proxima_fase = 0;
        }
        if((proxima_fase==1) && (n < 11)){         //caso tenha acertado, aumenta os numeros
            n++;
        }
        if(proxima_fase==0){                //caso tenha errado, volta pra dois numeros
            n=2;
        }
        if(n==11){                 //vai travar o programa
            while(1){
                LED2_OFF;
                tempo();
                LED2_ON;
                tempo();
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
    P4OUT   &=  ~BIT7;             //Inicia o Led 1, apagado, pois est� em 0

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



void tempo(){
    volatile unsigned int x = 50000;
    while (x--);
}


//Funcoes do Jogo

void start(){
    LED1_ON;
    LED2_ON;
    tempo();
    LED1_OFF;
    tempo();
    LED2_OFF;
}

void createSequence(int n){
    //Codigo da LFSR
    unsigned int x;
    for(x=0; x<n; x++){
        int i;

        b = vetor[15] ^ vetor[13];
        b ^= vetor[12];
        b ^= vetor[10];

        for(i=15;i>0;i--){
            vetor[i] = vetor[i-1];
        }

        vetor[0] = b;
        sequence[x] = b;
    }
}

void showSequence(int n){
    int i;
    for(i=0;i<n;i++){
        if(sequence[i]== 0){
            LED1_ON;
            tempo();
            LED1_OFF;
            tempo();
        }
        else{
            LED2_ON;
            tempo();
            LED2_OFF;
            tempo();
        }
    }
}


int game(int n)
{
    unsigned int i, chave1=0, chave2=1;
    for(i=0; i<n; i++){
        while((!CHAVE1_PRESSIONADA) && (!CHAVE2_PRESSIONADA)){ //fica aqui at� acionar alguma chave
        }

        if(CHAVE1_PRESSIONADA){
            tempo();
            while(CHAVE1_PRESSIONADA){      //enquanto n�o soltar a chave 1
            }
            tempo();

            if(sequence[i] != chave1){      //se n�o for 0, quer dizer que errou
                return 0;
            }
        }
        else{
            tempo();
            while(CHAVE2_PRESSIONADA){      //enquanto n�o soltar a chave 2
            }
            tempo();

            if(sequence[i] != chave2){      //se n�o for 1, ta errado
                return 0;
            }
        }
    }

    return 1;
}


void youwon(){
    int i;
    for(i=0; i<5; i++){
        LED2_ON;
        tempo();
        LED2_OFF;
        tempo();
    }
}
void youlose(){
    int i;
    for(i=0; i<5; i++){
        LED1_ON;
        tempo();
        LED1_OFF;
        tempo();
    }
}

