// código para SCDTR  
// versão parecida à que deu origem ao grafico

// Diogo Gois; Joao Ramiro; Jose Miragaia





//com quantos valores se vai fazer a media
#define tamluxs 5

//pin onde se localiza o led (MUDAR PARA DEFINE)
#define led 9

//pin onde se localiza o LDR (MUDAR PARA DEFINE)
#define analogPin 1


//prints provisoriamente em comentario para testar função de matlab


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
int adequateValue= 420;

//ultimos 5 valores de luxs
float last_luxs[tamluxs];

//novas variaveis de controlo
float K1, K2, Kp = 1.5, Ki = 3, b = 0.5, y, e , p, u, y_ant = 0, i_ant = 0, e_ant = 0, T;
//b entre 0 e 1

//variável que indica o tempo que decoreu desde o programa de controlo começou a correr
float time_stamp = 0;


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

    delay(50);

    //poe i como pwm do led corresponde à brightnedd
    analogWrite(led, i);

    //define valor na lookuptable e calcula lux atraves do valor lido no pino
    vetor[i] = calc_luxs(analogRead(analogPin));;

    //prints para ver o que esta a acontecer
    //Serial.print(vetor[i]);
    //Serial.print(' ');
    //Serial.println(i);

  }

  //Serial.print("Calibration Complete");
}

//faz shif left dos last_lux e adicion o ultimo valor (TROCAR ISTO POR FUNCAO QUE MIRAGAIA QUERIA)
void shift_left(float current_luxs)
{
  for (int i = 1; i < tamluxs; i++)
  {
    last_luxs[i - 1] = last_luxs[i];
  }
  last_luxs[tamluxs - 1] = current_luxs;
}


//função que utiliza a lookuptable creada para encontrar o valor pwm a aplicar ao LED para chegar ao valor
// obtido através da função de controlo
int search(float u)
{
  float dif = 0;
  int i = 1;
  for (i = 1; i < 256; i++)
  {
    dif = vetor[i] - u;
    if (dif > 0)
    {
      //break;
      return i - 1;
    }
  }
  return i - 5;
}


//funcao onde e controlado a luminosidade do led atraves de outras cenas
void controlo()
{
  float integ;      //termo integrador



  //lê pino e obtem luxs
  luxs = calc_luxs( analogRead(analogPin));
  shift_left(luxs);
  y = average();



  //calcula erro em relacao a referencia ilum_min
  e = ilum_min - y;

  //obtem parte proporcional
  p = K1 * ilum_min - Kp * y;

  //obtem parte integral
  integ = i_ant + K2 * (e + e_ant);

  // verificar se o valor calculado, composto pelo parte integral não se econtra demasiado, sabendo que o sistema não consegue com corrigir o erro,
  //logo este termo iria crescer indefinidamente) 
  // anti-windup
  
  if (abs(integ)> adequateValue)
  {
    integ = i_ant;
  }
  
  
  //descobre valor led
  u = p + integ;

  //faz analog write de tal valor
  analogWrite(led, search(u) + 2);


  //faz set das variaveis para o proximo loop
  y_ant = y;
  i_ant = integ;
  e_ant = e;


  //prints para fazer o grafico no matlab

  Serial.print(average());
  Serial.print(" ");
  Serial.println(time_stamp);

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


  T = 0.2;
  //calcula constantes com base nos parametros
  K1 = Kp * b;
  K2 = Kp * Ki * T / 2;

  //comecao serial comunication
  Serial.begin(9600);

  //calibra sistema, cria a lookuptable
  calibration();

  //desliga o LED "a sala está vazia logo, luzes desligadas"
  analogWrite(led, 0);

  //normaliza o vector que contem os ultimos valores lidos para não ocorrer erros nas médias, (falta confirmar se o valor da media chega a ~0 nofim)
  for (i = 0; i < tamluxs + 3; i++)
  {
    delay (50);
    val = analogRead(analogPin);

    luxs = calc_luxs(val);

    shift_left(luxs);
    //Serial.println(average());


  }
}


// the loop function runs over and over again forever
void loop() {
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

  //valor de delay possivelmente demasiado baixo o grafico obtido tem 10* de delay(se retirado o if)

  medias ++;
  
  delay(40);
  if (medias == 10)
  {
    controlo();
    time_stamp += 0.04;
    medias = 0;
  }

// esta secção serve para as medidas serem mais precisas(não sei qual o impacto no sistema real)
/*
  //le valor do led e calcula luxs
    luxs = calc_luxs(analogRead(analogPin));

    //faz shift left dos luxs
    shift_left(luxs);
*/

}
