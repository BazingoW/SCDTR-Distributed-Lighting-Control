// código para SCDTR


// Diogo Gois; Joao Ramiro; Jose Miragaia


// estados possiveis de ocupação da secretária
#define  OCUPADO 1           //valor de lux converge para maxluxs
#define LIVRE 0              //valor de lux converge para minlux
#define FREESTYLE -1         //valor de lux converge para valor definido pelo utilizador


//com quantos valores se vai fazer a media
#define tamluxs 7

//pin onde se localiza o led
#define led 9

//pin onde se localiza o LDR
#define analogPin 1

//Resistencia do circuito do LDR
#define r1 10000.0


//variaveis de HIGH E LOW
float minluxs = 20, maxluxs = 50;

//valor de lux definido pelo utilizador
float desejado = 0;

//toggles feedforward ON or OFF
bool FFD = false; 

//variável que entra na funcão de controlo
float ref = minluxs;

//periodo de amostragem em microsegundos
unsigned long sampInterval = 10000; //10 milisecs

//Look up table. Valor de lux correspondente para cada PWM
float lookUp[256];

//hold the current iteration 
long int lastMeasure = 0;

//ultimos 5 valores de luxs
float last_luxs[tamluxs];

//variaveis do controlador PID
float K1, K2, Kp = 0.1, Ki = 38, b = 0.5, y, e , p, u, y_ant = 0, i_ant = 0, e_ant = 0, integ=0;

//estado atual da secretatia
int estado = LIVRE;



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
  minluxs = lookUp[85];
  maxluxs = lookUp[170];

  //desliga o LED "a sala está vazia logo, luzes desligadas"
  analogWrite(led, 85);

  //normaliza o vector que contem os ultimos valores lidos para não ocorrer erros nas médias, (falta confirmar se o valor da media chega a ~0 nofim)
  for (i = 0; i < tamluxs + 3; i++)
  {
    delay (50);
    val = analogRead(analogPin);

    

    shift_left(calc_luxs(val));
    //Serial.println(average());


  }

  /*analogWrite(led, 255);

    Serial.print(micros());
    Serial.print(" ; ");
    Serial.println(calc_luxs( analogRead(analogPin)));*/
lookUp [0] = 0;

}


// the loop function runs over and over again forever
void loop() {

  unsigned long startTime = micros();
 
  controlo(ref);

  unsigned long endTime = micros();
  delayMicroseconds(sampInterval - (endTime - startTime));

}


void SerialInputs()
{


int index = 0;
String input;
char inData[10] = "";


  if (Serial.available() > 0)
  {
    
    
    while (Serial.available() > 0)
    {
     
      if (index < 9) // One less than the size of the array
      {
        inData[index] =  Serial.read(); //Read a character
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
}


//função que calcula a média dos luxs
float average()
{
  float somatodos = 0;
  int i;

  //soma todos os valores
  for (i = 0; i < tamluxs; i++)
  {
    somatodos += last_luxs[i];
  }
  
  //retorna a média
  return somatodos / tamluxs;
}


//função que calcula o valor de luxs
float calc_luxs(int val)
{
  float R2;
  float luxs;
  float tensao;
  
  //converte de 0-1023 para 0-5V
  tensao = (5 * val) / 1023.0;

  //equacao do divisor de tensao para obter resistencia R2
  R2 = ((r1 * 5) / tensao) - r1;

  //uso de reta logaritmica para converter de R para lux
  luxs = ((log10(R2) - 4.8451) / -0.7186);
  luxs = pow(10, luxs);

  return luxs;
}

//preenche look up table
void calibration()
{
  //itera por todos os valores possiveis para o led
  for (int i = 0; i < 256; i++)
  {
    delay(35);

    //poe i como pwm do led
    analogWrite(led, i);

    //define valor na lookuptable e calcula lux atraves do valor lido no pino
    lookUp[i] = calc_luxs(analogRead(analogPin));

    //prints para ver o que esta a acontecer
    Serial.print(lookUp[i]);
    Serial.print(' ');
    Serial.println(i);
  }
}

//guarda valor de lux num vetor com ultimos valores de lux
void shift_left(float current_luxs)
{  
  last_luxs[lastMeasure % tamluxs] = current_luxs;
  lastMeasure++;
}


//procura na lookuptable o valor pwm de LED correspondente aos lux pretendidos
int search(float u)
{
  
  int i = 1;

  //itera pela lookuptable toda
  for (i = 1; i < 255; i++)
  {
    //se valor de look up table for maior que a procura
    if (lookUp[i] > u)
    {
      //retorna pwm correspondente
      return i - 1;
    }
  }

  //caso nao encontre nada retorna 255
  return i;
}


//funcao onde e controlado a luminosidade do led atraves de feedforward e feedback (com antiwindup)
void controlo(float reference)
{
  //lê pino e obtem luxs
  
  shift_left(calc_luxs( analogRead(analogPin)));
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
  if (u > lookUp[255])
  {
    u = lookUp[255];
  }else if (u< lookUp[0])
  {
    
    u = lookUp[0];
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

