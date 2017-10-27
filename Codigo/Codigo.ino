// código para SCDTR
// versão parecida à que deu origem ao grafico

// Diogo Gois; Joao Ramiro; Jose Miragaia


// estados possiveis de ocupação da secretária
#define  OCUPADO 1
#define LIVRE 0
#define FREESTYLE -1
#define FEEDFOWARD -2


//com quantos valores se vai fazer a media
#define tamluxs 7

//pin onde se localiza o led (MUDAR PARA DEFINE)
#define led 9

//pin onde se localiza o LDR (MUDAR PARA DEFINE)
#define analogPin 1


//variaveis de HIGH E LOW
float minluxs = 20, maxluxs = 50;

float integ = 0;      //termo integrador
float desejado = 0;

bool FFD = false; 

//variável que entra na função de controlo
float ref = minluxs;

String input;
char inData[20] = "";

int index = 0;
char inChar;

//prints provisoriamente em comentario para testar função de matlab
unsigned long sampInterval = 10000; //10 milisecs

//Resistencia que nao e' o LDR (MUDAR PARA DEFINE)
float r1 = 10000.0;

//usada para obter quantos luxs esta o ldr a detetar atualmente
float luxs = 0;
float vetor[256];

//referencia de luxs que se quer
int ilum_min = 30;

//solução temporária para a utilização de delays
int medias = 0;

//valor de treshold, a partir do qual o termo integradoe não deverá passar
int adequateValue = 100; //valor maximo de lux tolerável antes de começar a alterar deve-se alterarad para dois limites diferentes (min e max)


long  int lastMeasure = 0;

//ultimos 5 valores de luxs
float last_luxs[tamluxs];

//novas variaveis de controlo
float K1, K2, Kp = 0.1, Ki = 38, b = 0.5, y, e , p, u, y_ant = 0, i_ant = 0, e_ant = 0, T;
//b entre 0 e 1

//variável que indica o tempo que decoreu desde o programa de controlo começou a correr
float time_stamp = 0;


int estado = LIVRE;


//função que calcula a média dos luxs
float average()
{
  float somatodos = 0, media = 0;
  int i;

  //soma todos os valores
  for (i = 0; i < tamluxs; i++)
  {
    somatodos += last_luxs[i];
  }

  //divide pelo numero de valores
  media = somatodos / tamluxs;

  return media;
}



//função que calcula o valor de luxs
float calc_luxs(int val)
{
  float R2;
  float luxs;
  float tensao;
  //regra de 3 simples para converter de 0-1023 para 0-5V
  tensao = (5 * val) / 1023.0;

  //equacao do divisor de tensao para obter resistencia R2
  R2 = ((r1 * 5) / tensao) - r1;

  //uso de reta para converter de R para lux
  luxs = ((log10(R2) - 4.8451) / -0.7186);
  luxs = pow(10, luxs);

  return luxs;

}

//função que preenche look up table
void calibration()
{
  //itera por todos os valores possiveis para o led
  for (int i = 0; i < 256; i++)
  {

    delay(35);

    //poe i como pwm do led corresponde à brightnedd
    analogWrite(led, i);

    //define valor na lookuptable e calcula lux atraves do valor lido no pino
    vetor[i] = calc_luxs(analogRead(analogPin));;

    //prints para ver o que esta a acontecer
    Serial.print(vetor[i]);
    Serial.print(' ');
    Serial.println(i);
  }

  //Serial.print("Calibration Complete");
}

//faz shif left dos last_lux e adicion o ultimo valor (TROCAR ISTO POR FUNCAO QUE MIRAGAIA QUERIA)
void shift_left(float current_luxs)
{
  /*
    for (int i = 1; i < tamluxs; i++)
    {
    last_luxs[i - 1] = last_luxs[i];
    }
    last_luxs[tamluxs - 1] = current_luxs;
  */
  last_luxs[lastMeasure % tamluxs] = current_luxs;
  lastMeasure++;

}


//função que utiliza a lookuptable creeada para encontrar o valor pwm a aplicar ao LED para chegar ao valor
// obtido através da função de controlo
int search(float u)
{
  float dif = 0;
  int i = 1;
  for (i = 1; i < 255; i++)
  {
    dif = vetor[i] - u;
    if (dif > 0)
    {
      //break;
      return i - 1;
    }
  }
  return i;
}


//funcao onde e controlado a luminosidade do led atraves de outras cenas
void controlo(float reference)
{




  //lê pino e obtem luxs
  luxs = calc_luxs( analogRead(analogPin));
  shift_left(luxs);
  y = average();



  //calcula erro em relacao a referencia ilum_min
  e = reference - y;

  //obtem parte proporcional
  p = K1 * reference - Kp * y;

  //obtem parte integral
  integ = i_ant + K2 * (e + e_ant);



  // verificar se o valor calculado, composto pelo parte integral não se econtra demasiado, sabendo que o sistema não consegue com corrigir o erro,
  //logo este termo iria crescer indefinidamente)
  // anti-windup
  // slide 25 cap 8

 

  //descobre valor led
  u = p + integ;
 float usat = 0;

 usat = u;
  if (u > vetor[255])
  {
    u = vetor[255];
  }else if (u< vetor[0])
  {
    
    u = vetor[0];
  }
  
  float wind = u - usat;

  integ += wind*K2*1.5;

  

  //faz analog write de tal valor

float  pwm = search (u);


  if (FFD == true)
    analogWrite(led, pwm + search(reference)+4);
  else
    analogWrite(led, pwm);


  //faz set das variaveis para o proximo loop
  y_ant = y;
  i_ant = integ;
  e_ant = e;


  //prints para fazer o grafico no matlab
  Serial.print(reference);
  Serial.print(" ; ");
  


  Serial.print(average());
  Serial.print(" ; ");

  Serial.print((pwm/255)* 100);
  Serial.println(" %");
  

  /*
    Serial.print("write value in lux :");
    Serial.println(u);

    Serial.print("write value :");
    Serial.println(search(u));
  */
}


void setup() {

  int i = 0, val = 0;
  //inicializa tamluxs a -1
  for (i = 0; i < tamluxs; i++)
  {
    last_luxs[i] = -1;
  }



  //calcula constantes com base nos parametros
  K1 = Kp * b;
  K2 = Kp * Ki * sampInterval / 2000000;

  //comecao serial comunication
  Serial.begin(250000);

  //calibra sistema, cria a lookuptable
  calibration();
  minluxs = vetor[85];
  maxluxs = vetor[170];

  //desliga o LED "a sala está vazia logo, luzes desligadas"
  analogWrite(led, 85);

  //normaliza o vector que contem os ultimos valores lidos para não ocorrer erros nas médias, (falta confirmar se o valor da media chega a ~0 nofim)
  for (i = 0; i < tamluxs + 3; i++)
  {
    delay (50);
    val = analogRead(analogPin);

    luxs = calc_luxs(val);

    shift_left(luxs);
    //Serial.println(average());


  }

  /*analogWrite(led, 255);

    Serial.print(micros());
    Serial.print(" ; ");
    Serial.println(calc_luxs( analogRead(analogPin)));*/
vetor [0] = 0;

}


// the loop function runs over and over again forever
void loop() {

  unsigned long startTime = micros();

  /*
    //set the led to something
    analogWrite(led, 228);   // turn the LED on (HIGH is the voltage level)

    delay(25);

    //le valor do led e calcula luxs
    luxs = calc_luxs(analogRead(analogPin));

    //faz shift left dos luxs
    shift_left(luxs);

    //faz print da average
    Serial.println(average());
  */

  /*Serial.print(micros());
    Serial.print(" ; ");
    Serial.println(calc_luxs( analogRead(analogPin)));*/

  //valor de delay possivelmente demasiado baixo o grafico obtido tem 10* de delay(se retirado o if)


  //escolher qual a referencia a seguir provisóriamente em comentário pois os valores de referencia vão ser
  // actualizados quando é dada o novo comando pela janela


/*
  if (luxs > vetor[255])
  {
    analogWrite(led, 0);
    luxs = calc_luxs( analogRead(analogPin));
    integ = 0;
    i_ant = 0;
    shift_left(luxs);
    y = average();
  }
  else*/
    controlo(ref);





  index = 0;

  if (Serial.available() > 0)
  {
    while (Serial.available() > 0)
    {

      if (index < 19) // One less than the size of the array
      {
        inChar = Serial.read(); // Read a character
        inData[index] = inChar; // Store it
        index++; // Increment where to write next
        inData[index] = '\0'; // Null terminate the string
      }

    }
    Serial.println(inData);
    input = String(inData);
    Serial.println(input);

    if (input.substring(0, 3) == "ocu")
    {
      if (input[4] == '0')
      {
        estado = LIVRE;
        ref = minluxs;
        Serial.println("changed to free");
      }
      else if (input[4] == '1')
      {
        estado = OCUPADO;
        ref = maxluxs;

        Serial.println("changed to occupied");
      }
      else
        Serial.println("Please revise the intruction to send commands");
    }
    else if (input.substring(0, 3) == "min")
    {
      minluxs = input.substring(4, 19).toFloat();
      Serial.print("changed min to");
      Serial.println(minluxs);
      if (estado == LIVRE)
        ref = minluxs;
    }
    else if (input.substring(0, 3) == "max")
    {

      maxluxs = input.substring(4, 19).toFloat();
      Serial.print("changed max to");
      Serial.println(maxluxs);
      if (estado == OCUPADO)
        ref = maxluxs;
    }
    else if (input.substring(0, 3) == "lux")
    {
      ref = input.substring(4, 19).toFloat();
      desejado = ref;
      estado = FREESTYLE;
      Serial.print("mode set to freestyle ");
      Serial.println(ref);
    }
    else if (input.substring(0, 3) == "ffd")
    {
      ref = input.substring(4, 19).toFloat();

      if (input[4] == '0')
      {
        Serial.println("feedfoward OFF");
        FFD = false;
      }
      else if (input[4] == '1')
      {
        Serial.println("feedfoward ON");
        FFD = true;
      }
      else
        Serial.println("Please revise the intruction to send commands");

      if (estado == OCUPADO)
        ref = maxluxs;

      else if (estado == LIVRE)
        ref = minluxs;

      if (estado == FREESTYLE)
        ref = desejado;

    }
    else
      Serial.println("Please revise the intruction to send commands");

    Serial.println(ref);
  }


  unsigned long endTime = micros();
  delayMicroseconds(sampInterval - (endTime - startTime));




  // esta secção serve para as medidas serem mais precisas(não sei qual o impacto no sistema real)
  /*
    //le valor do led e calcula luxs
      luxs = calc_luxs(analogRead(analogPin));

      //faz shift left dos luxs
      shift_left(luxs);
  */

}
