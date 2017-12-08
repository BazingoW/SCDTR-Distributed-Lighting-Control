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

//variavel que corresponde ao declive da recta de calibração
float declive = 0;

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

  //calcula constantes com base nos parametros (esta' nos slides)
  K1 = Kp * b;
  K2 = Kp * Ki * sampInterval / 2000000;

  //comecao serial comunication
  Serial.begin(250000);

  //calibra sistema, cria a lookuptable
 // calibration();
 recta_luxs();

  //define minlux a 1/3 da escala
  minluxs = lookUp[85];

  //define minlux a 1/3 da escala
  maxluxs = lookUp[170];

//define minlux a 1/3 da escala
  minluxs =  85/declive;

  //define minlux a 1/3 da escala
  maxluxs = 170/declive;


  //desliga o LED "a sala está vazia logo, luzes desligadas"
  analogWrite(led, 85);

  //inicializa o vetor de last_luxs
  for (i = 0; i < tamluxs + 3; i++)
  {
    delay (50);

    //Obtem ultimos valores de lux e 
    shift_left(calc_luxs(analogRead(analogPin)));
   
  }

   //define o primeiro valor da lookup table a zero 
   lookUp [0] = 0;
}


// the loop function runs over and over again forever
void loop() {


  //define start time
  unsigned long startTime = micros();

  //faz o controlo do sistema
  controlo(ref);

  //define tempo qd controlo acaba
  unsigned long endTime = micros();

  //ve se utilizador pos algum input
  SerialInputs();

  //espera o tempo necessario para passar 1 sampling interval
  delayMicroseconds(sampInterval - (endTime - startTime));
}


//usada para alterar ocupacao de secretaria entre outras coisas
void SerialInputs()
{

int index = 0;

//string de data recebida
String input;

//vetor de chars recebidos
char inData[10] = "";

  //se utilizador escreveu algo
  if (Serial.available() > 0)
  {
    
    //le o que o utilizador escreveu
    while (Serial.available() > 0)
    {
     
      if (index < 9) // One less than the size of the array
      {
        inData[index] =  Serial.read(); //Read a character
        index++; // Increment where to write next
        inData[index] = '\0'; // Null terminate the string
      }

    }

    //converte vetor de chars em string
    input = String(inData);
    
    //verrifica se utilizador quer mudar utilização da secretaria
    if (input.substring(0, 3) == "ocu")
    {
      //se ocu 0
      if (input[4] == '0')
      {
        //Define como estando livre
        estado = LIVRE;
        ref = minluxs;
        Serial.println("changed to free");
      }
       //se ocu 1
      else if (input[4] == '1')
      {
        //Define como estando ocupada
        estado = OCUPADO;
        ref = maxluxs;

        Serial.println("changed to occupied");
      }
      else
        Serial.println("Please revise the intruction to send commands");
    }
    //define novo minlux
    else if (input.substring(0, 3) == "min")
    {
      minluxs = input.substring(4, 19).toFloat();
      Serial.print("changed min to");
      Serial.println(minluxs);

      //muda a referencia para novo valor de minluxs
      if (estado == LIVRE)
        ref = minluxs;
    }
    //define novo maxlux
    else if (input.substring(0, 3) == "max")
    {

      maxluxs = input.substring(4, 19).toFloat();
      Serial.print("changed max to");
      Serial.println(maxluxs);

       //muda a referencia para novo valor de minluxs
      if (estado == OCUPADO)
        ref = maxluxs;
    }
    //se utilizador quer que haja um valor de lux especifico
    else if (input.substring(0, 3) == "lux")
    {
      desejado = input.substring(4, 19).toFloat();

      //define esse valor como a referencia
      ref = desejado;

      //poe a secretaria para estar nem livre, nem ocupada
      estado = FREESTYLE;
      
      Serial.print("mode set to freestyle ");
      Serial.println(ref);
    }
    //se utilizador quer ativar ou desativar o feedforward
    else if (input.substring(0, 3) == "ffd")
    {    

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
  //luxs = ((log10(R2) - 4.8451) / -0.7186);
  luxs = log10(R2) - 6 / (-0.5);
  luxs = pow(10, luxs);

  Serial.println(R2);
  delay(1000);

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
    Serial.print(analogRead(analogPin));
    Serial.print(' ');
    Serial.println(i);
  }
}


void recta_luxs()
{
  analogWrite(led,200);

  delay(1000);
  float luxs = 0;
  luxs =  calc_luxs(analogRead(analogPin));
  Serial.print(calc_luxs(analogRead(analogPin)));
  Serial.print(' ');
  Serial.println(200); 
  declive = 200/luxs;
  Serial.println(declive);
  delay(1000);

  

   
}

//guarda valor de lux num vetor com ultimos valores de lux
void shift_left(float current_luxs)
{  
  last_luxs[lastMeasure % tamluxs] = current_luxs;
  lastMeasure++;
}


//procura na lookuptable o valor pwm de LED correspondente aos lux pretendidos
float search(float u)
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
  float usat = 0;
  float wind;
  float  pwm;
  
  //lê pino e obtem luxs  
  shift_left(calc_luxs( analogRead(analogPin)));

  //faz a media dos ultimos lux
  y = average();

  //calcula erro em relacao a referencia ilum_min
  e = reference - y;

  //obtem parte proporcional
  p = K1 * reference - Kp * y;

  //obtem parte integral
  integ = i_ant + K2 * (e + e_ant);

//descobre valor led
  u = p + integ;

//parte do windud
usat=u;
/*
  if (u > lookUp[255])
  {
    usat = lookUp[255];
  }else if (u< lookUp[0])
  {
    
    usat = lookUp[0];
  }*/
    if (u > 255/declive)
  {
    usat = 255/declive;
  }else if (u< 0)
  {
    
    usat = 0;
  }

  

  //apenas e diferentde de 0 se estiver saturado
  wind = usat - u;

  //adicionar a proxima iteracao integradora o anti windup para o sistema nao integrar bue
  integ += wind*K2*1.5;

  //procura valor na lookup table
  // pwm = search (u);
  pwm = declive * usat ;
  
/*
  //write to pin pwm, if feedforward is on add that as well
  if (FFD == true)
    //analogWrite(led, search(reference + u));
    analogWrite(led, declive * (reference + u));
  else
    analogWrite(led, pwm);*/

//write to pin pwm, if feedforward is on add that as well
  if (FFD == true)
    //analogWrite(led, search(reference + u));
{ 
  pwm = declive * (reference + usat);
}  

if(pwm <0)
    pwm = 0;
if(pwm > 255)
    pwm = 255;

analogWrite(led,pwm);


  //faz set das variaveis para o proximo loop
  y_ant = y;
  i_ant = integ;
  e_ant = e;

  
 //prints pedidos pelo prof
  Serial.print(reference);
  Serial.print(" ; ");
  Serial.print(average());
  Serial.print(" ; ");
  if (FFD == true)
   // Serial.print(((search(reference+u))/255)* 100);
    Serial.print(((declive*(reference+u))/255)* 100);
  else
    Serial.print((pwm/255)* 100);
  Serial.println(" %");
  Serial.println(u);
}

