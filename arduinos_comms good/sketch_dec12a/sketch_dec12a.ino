#include <Wire.h>

#define analogPin 1
#define n_iter 25
#define tamluxs 7


//Resistencia do circuito do LDR
#define r1 10000.0
#define rh 0.01
#define ZERO 200

long int lastMeasure = 0;
//ultimos 5 valores de luxs
float last_luxs[tamluxs];
unsigned long sampInterval = 10000; //10 milisecs

float declive = 0;
float lux_max = 0.0;
float lux_min = 0.0;

float K1, K2, Kp = 0.1, Ki = 38, b = 0.5, y, e , p, u, y_ant = 0, i_ant = 0, e_ant = 0, integ = 0;


float kself, kmutuo;
int ledpin = 9;
int raspberry_add = 100;
float luxs = 3;
int estado = 0;
float ref = 20.0;
//valor que se se encontrar a 1 quer dizer que não é preciso enviar a mesnagem para o rapberry
int central = 0;
int arduinos[1];
byte end_consensus[2] = {'z', 0};

int pin_verif = 2;
int address, address_aux;
long  temp;
int flag = 0;
int flag_arduinos = 0;
int flag_cons = 0;
bool on = 0;
float ext_illum = 0;
float min_best[n_iter];
byte buffer[3] ;
byte serverMSG[4];
byte request[4];


//vetor de chars recebidos
byte inData[10];
byte inData1[10];


//System
float k11 = 0.67, k12 = 0.18, k21 = 0.01, k22 = 0.04;
float L1 = 30, o1 = 0, L2 = 30, o2 = 0;
float K[][2] = {{k11, k12}, {k21, k22}};
float L[][1] = {{L1}, {L2}};
float o[][1] = {{o1}, {o2}};

//Cost function
float c1 = 1, c2 = 1;
float c[] = {c1, c2};
float q1 = 1.0, q2 = 1.0;
float Q[][2] = {{q1, 0}, {0, q2}};

//Consensus variables
float rho = 0.15;
//node 1
float d1[] = {0, 0};
float d1_av[] = {0, 0};
float d_copy[] = {0, 0};
float y1[] = {0, 0};
float k1[] = {k11, k12};

//necessário? i think so
//node 2
float d2[] = {0, 0};
float d2_av[] = {0, 0};
float y2[] = {0, 0};
float k2[] = {k21, k22};


void setup()
{
  Serial.begin(250000);           // start serial for output
  inData[0] = 42;
  inData1[0] = 42;
  inData1[1] = '\0';
  inData[1] = '\0';
  // define os endereços do arduino segundo o estado do pin
  // define initial message to send to other arduinos


  //esta linha passa a ser desnecessaria pois os valores dos k,L e o vão ser lidos/definidos
  if (digitalRead(pin_verif) == HIGH)
  {
    address = 2;
    address_aux = 1;
    k11 = k22;
    k12 = k21;
    L1 = L2;
    o1 = o2;
  }
  else
  {
    address = 1;
    address_aux = 2;
  }

  K1 = Kp * b;
  K2 = Kp * Ki * sampInterval / 2000000;


  // formato da mensagem será '#-address-mensagetosend'it is currently used 1 byte per item, the mensage to send can be changed to type of data -value, or if it is knowned only the values
  //aquando de receber a mesagem tranformar o valor de byte em inteiro  int(buffer[2])
  buffer[0] = '%';
  buffer[1] = '0' + address;
  buffer[2] = 123;
  serverMSG[0] = '$';
  serverMSG[2] = address;




  Wire.begin(address); // join i2c bus (address optional for master)
  Wire.onReceive(receiveEvent); // register event


  Wire.beginTransmission(address_aux); // transmit to device
  Wire.write("O");
  Wire.endTransmission();    // stop transmitting
  //Serial.println("SentData");
  //example of how it should be read the message
  //Serial.println(int(buffer[2]));

  /*//inicializar o vector que contem valores lidos pelos ldr
    for (int i=1;i<15;i++)
    {
    delay(35);
     shift_left(calc_luxs( analogRead(analogPin)));
    }*/
}



void loop()
{


  if (flag > 0)
  {
    if (flag == 1)
    {
      Wire.beginTransmission(address_aux); // transmit to device #8
      Wire.write("R");        // sends five bytes
      Wire.endTransmission();    // stop transmitting
      Serial.println("SentData");
      calibrar1();

      // start the consensus from the address 1
      if (address == 1)
      {
        Serial.println("please w8 a few seconds for the consensus to be operational");
        iteracao();
      }

      on = 1;
      flag = 0;

    } else if (flag == 2)
    {
      calibrar1();
      // start the consensus from the address 1
      if (address == 1)
      {
        Serial.println("please w8 a few seconds for the consensus to be operational");
        iteracao();
        //define tempo qd controlo acaba
        //unsigned long endTime = micros();
      }
      on = 1;
      flag = 0;
    }

  }


  if (on)
  {
    unsigned long startTime = micros();
    //transmit(buffer,address_aux);

    //read messages that occur in the serial port
    SerialInputs();

    //define start time
    // prob this is not the final example becuase we might need check the messages to send to the server and the consensus
    if (flag == 3 && flag_cons == 0)
    {
      // Serial.println("data info");

      if (inData[2] == ZERO)
        inData[2] = 0;
      if (inData[4] == ZERO)
        inData[4] = 0;
      // Serial.println("valores de d da iteração anterior");
      //Serial.println(int(inData[1]));
      // Serial.println(float(int(inData[2])/100));
      //Serial.println(int(inData[1]));
      //Serial.println(int(inData[4])/100);

      /*
        Serial.println("anterior");
        Serial.println(d_copy[0]);

        Serial.println("novo ");
        Serial.println(inData[1] + inData[2] / 100.0);*/

       if(      (abs(d_copy[0]-   (inData[1] +inData[2] / 100))   <1) && (abs(d_copy[1] - (inData[3] + inData[4] / 100))<1) && (d_copy[0]!=0))
      //if (int(d_copy[0]) - int(inData[1]) == 0  && int(d_copy[1]) - int(inData[3] == 0) && d_copy[0] != 0)
      {
        Serial.println("end consensus");

        transmit(end_consensus, address_aux, 2);
        flag_cons = 1;
        flag = 0;

      } else
      {
        /*
          Serial.println("anterioir");
          Serial.println(d_copy[0]);
          Serial.println(d_copy[1]);*/


        d_copy[0] = int(inData[1]) + int(inData[2]) / 100;
        d_copy[1] = int(inData[3]) + int(inData[4]) / 100;
        iteracao();
        flag = 0;
        /*
          Serial.println("novo");
          Serial.println(d_copy[0]);
          Serial.println(d_copy[1]);*/

      }

    }

    if (flag_cons )
    {
      startTime = micros();
      controlo(L1);
      if ( flag_arduinos == 4)
      {
        flag_arduinos = 0;
        if (char(inData1[1]) == 's')
        {
          Serial.print("set room occupattion:  ");
          Serial.println(inData1[2]-'a');
          if (estado != (inData1[2] - 'a'))
          {
            estado = inData1[2] - 'a' ;
            if (estado == 1)
              L1 = lux_max;
            else
              L1 = lux_min;
            // indica ao programa que deverá ser recalculado o consensus enviar mensagem
            flag_cons = 0;
            flag = 0;
            iteracao();
            Serial.println("b4");
            Serial.println(estado);
            return;
            
          }
        } else {
          switch (char(inData1[2]))
          {
            case 'o':
              {
                serverMSG[1] = 'o';
                if (estado == 1)
                  serverMSG[3] = estado;
                else
                  serverMSG[3] = ZERO;
                serverMSG[4] = 0;
                break;
              }
            //lower bound not sure what this is
            case 'L':
              {
                serverMSG[1] = 'L';
                serverMSG[3] = L1;
                serverMSG[4] = 0;
                break;
              }
            // background illuminace
            case 'O':
              {
                serverMSG[1] = 'O';
                serverMSG[3] = int(o1);
                serverMSG[4] = int((o1-int(o1))*100);
                serverMSG[4] = 0;
                break;
              }
            //illuminace control referece
            case 'r':
              {
                serverMSG[1] = 'L';
                serverMSG[3] = int(L1);
                serverMSG[4] = int((L1-int(L1))*100);
                serverMSG[5] = 0;
                break;
              }
          }
          // confirmar o tamanho das coisas
          //100 DUMMY ADDRESS for the rasp1 to sniff
          transmit(serverMSG, address_aux, 12);
          Serial.println(serverMSG[3]);
        }

      }
      // confirmar o tamanho das coisas
      //transmit(serverMSG,other_address,);
      //measured luminance
      unsigned long endTime = micros();
      delayMicroseconds((sampInterval - (endTime - startTime)) / 2);
      luxs = calc_luxs(analogRead(analogPin));
      serverMSG[1] = '1';
      serverMSG[3] = int(luxs);
      serverMSG[4] = int((luxs - int(luxs)) * 100);
      serverMSG[5] = 0;//parece fixe por um 0 no fim pq acaba a leitura NULL
      transmit(serverMSG, address_aux, 6);


      //espera o tempo necessario para passar 1 sampling interval
      delayMicroseconds((sampInterval - (endTime - startTime)) / 2);

      //duty cycle in percentage
      serverMSG[1] = 'd';
      serverMSG[3] = int(d_copy[address - 1]);
      serverMSG[4] = int((d_copy[address - 1] - int(d_copy[address - 1])) * 100);
      serverMSG[5] = 0;
      transmit(serverMSG, address_aux, 6);

    }

  }
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
  //luxs = ((log10(R2) - 4.5533) / -3.1576);
  luxs = pow(10, luxs);
  if (address == 2)
    return 17 * luxs;

  return 2 * luxs;
}

//verifica se arduino nao esta a mandar muita informação há algum tempo


//função que calibra ambos os leds dos arduinos
void calibrar()
{
  if (address == 1)
  {
    digitalWrite(ledpin, HIGH);
  }
  else
  {
    digitalWrite(ledpin, LOW);
  }

  delay(5000);

  Serial.println(calc_luxs(analogRead(analogPin)));

  if ( address == 2)
  {
    digitalWrite(ledpin, HIGH);
  }
  else
  {
    digitalWrite(ledpin, LOW);
  }
  delay(5000);
  Serial.println(calc_luxs(analogRead(analogPin)));





}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{

  int index = 1;
  int start = 0;

  //string de data recebida
  String input;
  byte aux = 42;
  byte garbage;

  while (Wire.available() > 0) { // loop through all but the last

    if (aux == '$')
    {
      garbage = Wire.read();
      continue;
    }

    if (start == 0)
    {
      aux =  Wire.read(); //Read a character
      start = 1;
      continue;
    }

    if (aux == '#')
    {
      inData1[index] = Wire.read();
      index++; // Increment where to write next
      inData1[index] = '\0'; // Null terminate the string
    }
    else
    {
      inData[index] = Wire.read();
      index++; // Increment where to write next
      inData[index] = '\0'; // Null terminate the string
    }
  }
  //Serial.println("ReceivedData");
  //Serial.println(inData);
  //Serial.println(index);
  //  Serial.println(size(howMany))

  if (aux == 'O')
  { //se recebi um O, reenvio um R e calibro
    Serial.println("O");
    flag = 1;
  }
  else if (aux == 'R')
  {
    Serial.println("R");
    flag = 2;
  }
  // significa que o consensus do outro nó acabou e que este device deverá ser os novos comandos e calcular novos valores
  else if (aux == '%')
  {
    flag = 3;
    flag_cons=0;
  }
  // significa que este arduino deverá enviar informação para o raspberry
  else if (aux == '#')
  {
    //achoq ue é melhor separar as flags para  não interromper o consensus
    flag_arduinos = 4;
    //   Serial.println("indata ")
    /*
      for (int i = 0; i < 10; i++)
      {
       inData1[i] = inData[i];
      }*/
    // inData[1] = 'l';
    //  Serial.println(inData1[1]);

  } else if (aux == 'z')
  {
    flag = 0;
    flag_cons = 1;
  }
  //significa que este é o arduino que esta a receber os valor totais para fazer a soma T potencia conforto e n me lembro o que é mais
  /* else if (inData[0] == '$')
    {
      //flag_calc = 1 ;
    }*/
}


void calibrar1()
{

  Serial.println("CalibFunc");
  analogWrite(ledpin, 0);
  delay(200);
  analogWrite(ledpin, 255);


  delay(200);
  analogWrite(ledpin, 0);
  delay(200);

  if (address == 1)
  {
    analogWrite(ledpin, 255);
  }
  else
  {
    analogWrite(ledpin, 0);
  }

  delay(500);

  if (address == 1)
  {
    kself = (calc_luxs(analogRead(analogPin))) / 100.0;
    Serial.print("valor de luxs calculado: ");
    Serial.println(calc_luxs(analogRead(analogPin)));
    Serial.println(analogRead(analogPin));

  }
  else
  {
    kmutuo = (calc_luxs(analogRead(analogPin))) / 100.0;
    Serial.print("valor de luxs calculado: ");
    Serial.println(calc_luxs(analogRead(analogPin)));
    Serial.println(analogRead(analogPin));

  }

  delay(500);

  if (address == 2)
  {
    analogWrite(ledpin, 255);
  }
  else
  {
    analogWrite(ledpin, 0);
  }

  delay(500);

  if (address == 2)
  {
    kself = (calc_luxs(analogRead(analogPin))) / 100.0;
    Serial.print("valor de luxs calculado: ");
    Serial.println(calc_luxs(analogRead(analogPin)));
    Serial.println(analogRead(analogPin));

  }
  else
  {
    kmutuo = (calc_luxs(analogRead(analogPin))) / 100.0;
    Serial.print("valor de luxs calculado: ");
    Serial.println(calc_luxs(analogRead(analogPin)));
    Serial.println(analogRead(analogPin));

  }

  delay(500);

  analogWrite(ledpin, 0);
  delay(500);


  o1 = calc_luxs(analogRead(analogPin));
  Serial.println(kself);
  Serial.println(kmutuo);
  k11 = kself;
  declive = 255 / (k11 * 100);
  k12 = kmutuo;
Serial.println(k11);
Serial.println(k12);
Serial.println(k11+k12);
Serial.println(200*(k11+k12));

Serial.println((200*(k11+k12))/3);
  //lux_max = 170 / declive;
  //lux_min = 85 / declive;
  //achei que se notava pouco a diferenca na luminusidade por isso repensei nos limites

  lux_max = (300 *(k11+k12))/4;
  lux_min = (100 *(k11+k12))/4;
  Serial.println(lux_max);
  Serial.println(lux_min);
  L1 = lux_min;




}


void iteracao()
{

  float z11, z12;
  float mini = 0;

  //node 1
  float d11_best = -1;
  float d12_best = -1;

  float min_best_1 = 100000;

  float sol_unconstrained = 1, sol_boundary_linear = 1, sol_boundary_0 = 1, sol_boundary_100 = 1;
  float sol_linear_0 = 1, sol_linear_100 = 1;
  /*
    if(address == 2)
     {
    //        node2_change(y[1],y[0]);
      float temp;
      temp = y1[0];
      y1[0] = y1[1];
      y1[1] = temp;

      temp = d1_av[0];
      d1_av[0] = d1_av[1];
      d1_av[1] = temp;

    //      node2_change(d1_av[1],d1_av[0]);
     }
  */

  /*
      Serial.print(d1_av[0]);
      Serial.print("      ");
      Serial.println(d1_av[1]);

      Serial.print(y1[0]);
      Serial.print("      ");
      Serial.println(y1[1]);*/
  z11 = -c1 - y1[0] + rho * d1_av[0];
  z12 = -y1[1] +  rho * d1_av[1];
  /*
    Serial.print(z11);
    Serial.print("      ");
    Serial.println(z12);
  */
  float u1 = o1 - L1;
  float u2 = 0;
  float u3 = 100;
  float p11 = 1 / (rho + q1);
  float p12 = 1 / rho;


  float n = k11 * k11 * p11 + k12 * k12 * p12;
  float w1 = -k11 * p11 * z11 - k12 * z12 * p12;
  float w2 = -z11 * p11;
  float w3 = z11 * p11;


  //compute unconstrained minimum
  float d11u = p11 * z11;
  float d12u = p12 * z12;

  //guardar valores
  float best_d11[50];
  float best_d12[50] ;

  //check feasibility of unconstrained minimum using local constraints
  if (d11u < 0)
  {
    sol_unconstrained = 0;
  }

  if (d11u > 100)
  {
    sol_unconstrained = 0;
  }

  if (k11 * d11u + k12 * d12u < L1 - o1)
  {
    sol_unconstrained = 0;
  }


  //compute function value and if best store new optimum
  if (sol_unconstrained)
  {
    mini = 0.5 * q1 * sq(d11u) + c1 * d11u + y1[0] * (d11u - d1_av[0]) + y1[1] * (d12u - d1_av[1]) + rho / 2 * sq(d11u - d1_av[0]) + rho / 2 * sq(d12u - d1_av[1]);

    if (mini < min_best_1)
    {
      d11_best = d11u;
      d12_best = d12u;
      min_best_1 = mini;
    }
  }
  /*
    Serial.print("unconstrained value : ");
    Serial.print(mini);
    Serial.println(d11_best);
  */

  //compute minimum constrained to linear boundary
  float d11bl = p11 * z11 + p11 * k11 / n * (w1 - u1);
  float d12bl = p12 * z12 + p12 * k12 / n * (w1 - u1);


  //check feasibility of minimum constrained to linear boundary
  if (d11bl < 0)
  {
    sol_boundary_linear = 0;
  }


  if (d11bl > 100)
  {
    sol_boundary_linear = 0;
  }


  //compute function value and if best store new optimum
  if (sol_boundary_linear)
  {
    mini = 0.5 * q1 * sq(d11bl) + c1 * d11bl + y1[0] * (d11bl - d1_av[0]) + y1[1] * (d12bl - d1_av[1]) + rho / 2 * sq(d11bl - d1_av[0]) + rho / 2 * sq(d12bl - d1_av[1]);




    if (mini < min_best_1)
    {
      d11_best = d11bl;
      d12_best = d12bl;
      min_best_1 = mini;
    }
  }

  // Serial.print("boundary_linear: ");
  //  Serial.print(mini);
  //Serial.println(d11_best);

  //compute minimum constrained to boundary
  float d11b0 = 0;
  float d12b0 = p12 * z12;


  //check feasibility of minimum constrained to 0 boundary
  //aqui alguma coisa estranha
  if (d11b0 > 100)
  {
    sol_boundary_0 = 0;
  }
  if (k11 * d11b0 + k12 * d12b0 < L1 - o1)
  {
    sol_boundary_0 = 0;
  }


  //compute function value and if the best store new optimum
  if (sol_boundary_0)
  {
    mini = 0.5 * q1 * sq(d11b0) + c1 * d11b0 + y1[0] * (d11b0 - d1_av[0]) + y1[1] * (d12b0 - d1_av[2]) + rho / 2 * sq(d11b0 - d1_av[0]) + rho / 2 * sq(d12b0 - d1_av[1]);

    if (mini < min_best_1)
    {
      d11_best = d11b0;
      d12_best = d12b0;
      min_best_1 = mini;
    }
  }

  //Serial.print("min_boundary: ");
  // Serial.print(mini);
  // Serial.println(d11_best);

  //compute minimum constrained to 100 boundary
  float d11b100 = 100;
  float d12b100 = p12 * z12;

  //check feasibility of minimum constrained to 100 boundary
  if (d11b0 < 0)
  {
    sol_boundary_100 = 0;
  }
  if (k11 * d11b100 + k12 * d12b100 < L1 - o1)
  {
    sol_boundary_100 = 0;
  }


  //compute function value and if best store new optimum
  if (sol_boundary_100)
  {
    mini = 0.5 * q1 * sq(d11b100) + c1 * d11b100 + y1[0] * (d11b100 - d1_av[0]) + y1[1] * (d12b100 - d1_av[1]) + rho / 2 * sq(d11b100 - d1_av[0]) + rho / 2 * sq(d12b100 - d1_av[1]);

    if (mini < min_best_1)
    {
      d11_best = d11b100;
      d12_best = d12b100;
      min_best_1 = mini;
    }
  }

  //Serial.print("min_boundary100: ");
  //Serial.print(mini);
  //Serial.println(d11_best);

  //compute minimum constrained to linear and zero boundary
  float common = (rho + q1) / ((rho + q1) * n - k11 * k11); //ou float?
  float det1 = common;
  float det2 = -k11 * common;
  float det3 = det2;
  float det4 = n * (rho + q1) * common;




  float x1 = det1 * w1 + det2 * w2;
  float x2 = det3 * w1 + det4 * w2;
  float v1 = det1 * u1 + det2 * u2; //u2 = 0 so this can be simplified
  float v2 = det3 * u1 + det4 * u2; //u2 = 0 so this can be simplified
  float d11l0 = p11 * z11 + p11 * k11 * (x1 - v1) + p11 * (x2 - v2);
  float d12l0 = p12 * z12 + p12 * k12 * (x1 - v1);

  //check feasibility
  if (d11l0 > 100)
  {
    sol_linear_0 = 0;
  }


  //compute function value and if best store new optimum
  if (sol_linear_0)
  {
    mini = 0.5 * q1 * sq(d11l0) + c1 * d11l0 + y1[0] * (d11l0 - d1_av[0]) + y1[1] * (d12l0 - d1_av[1]) + rho / 2 * sq(d11l0 - d1_av[0]) + rho / 2 * sq(d12l0 - d1_av[1]);
    // Serial.print("valmin_linear_0: ");
    //Serial.println(mini);
    if (mini < min_best_1)
    {
      d11_best = d11l0;
      d12_best = d12l0;
      min_best_1 = mini;
    }

  }

  //Serial.print("min_linear_0: ");
  //Serial.print(mini);
  //Serial.println(d11_best);

  //compute minimum constrained to linear and 100 boundary
  common = (rho + q1) / ((rho + q1) * n - k11 * k11); //ou float?
  det1 = common;
  det2 = k11 * common;
  det3 = det2;
  det4 = n * (rho + q1) * common;
  x1 = det1 * w1 + det2 * w3;
  x2 = det3 * w1 + det4 * w3;
  v1 = det1 * u1 + det2 * u3;
  v2 = det3 * u1 + det4 * u3;
  float d11l100 = p11 * z11 + p11 * k11 * (x1 - v1) - p11 * (x2 - v2);
  float d12l100 = p12 * z12 + p12 * k12 * (x1 - v1);


  //check feasibility
  if (d11l100 < 0)
  {
    sol_linear_100 = 0;
  }


  //compute function value and if best store new optimum
  if (sol_linear_100)
  {
    float  mini = 0.5 * q1 * sq(d11l100) + c1 * d11l100 + y1[0] * (d11l100 - d1_av[0]) + y1[1] * (d12l100 - d1_av[1]) + rho / 2 * sq(d11l100 - d1_av[0]) + rho / 2 * sq(d12l100 - d1_av[1]);

    if (mini < min_best_1)
    {
      d11_best = d11u;
      d12_best = d12u;
      min_best_1 = mini;

    }
  }
  //Serial.print("min_linear_100");
  //Serial.print(mini);
  //    Serial.println(d11_best);

  //store data and save for next cycle
  //best_d11[i] = d11_best;
  //best_d12[i] = d12_best;
  float d1[] = {};
  d1[0] = d11_best;
  d1[1] = d12_best;
  /*
        if(address == 2)
         {
    //      node2_change(y[1],y[0]);
    //        node2_change(d1_av[1],d1_av[0]);
          temp = y1[1];
          y1[1] = y1[2];
          y1[2] = temp;

          temp = d1_av[1];
          d1_av[1] = d1_av[2];
          d1_av[2] = temp;

         }*/
  //compute average with available knowledge
  d1_av[0] = (d1[0] + d_copy[0]) / 2;
  d1_av[1] = (d1[1] + d_copy[1]) / 2;
  //update local lagrangian
  y1[0] = y1[0] + rho * (d1[0] - d1_av[0]);
  y1[1] = y1[1] + rho * (d1[1] - d1_av[1]);
  //send node 1 solution to neighboors
  //d1_copy = d1;
  //dunno if this works

  //  Serial.print(y1[0]);
  // Serial.print("  y1/2    ");
  //Serial.println(y1[1]);
  //Serial.print(d1_av[0]);
  //Serial.print("  d1_av    ");
  //Serial.println(d1_av[1]);

  d_copy[0] = d1[0];
  d_copy[1] = d1[1];


  //mensagem a enviar é o d1_copy cada um dos nós processará de forma diferente

  //os valores de d1 vêem em float por isso o que vou fazer é transformar esse valor em dois bytes 16-> 65536 valor maximo transmitido
  // o que vamos fazer aqui é usar os 5 digitos que temos para representar em que vamos ter 2 para parte inteira e 2 decimal
  // a outra hipotese é usar 255 e usar apenas uma casa decimal mas acho mal não suficiente
  buffer[1] = int(d1[1]);
  buffer[2] = int((d1[1] - int(d1[1])) * 100);
  /*Serial.print("valor float   ");
    Serial.println(buffer[2]);*/
  if (buffer[2] == 0)
  {
    buffer[2] = ZERO;
  }
  buffer[3] = int(d1[0]);
  buffer[4] = int((d1[0] - int(d1[0])) * 100);
  if (buffer[4] == 0)
  {
    buffer[4] = ZERO;
  }
  buffer[5] = 0;
  //set's up the mensage to send to the other node
  // Serial.println("data calculate, results presented below: will take 3-4s to reach other device informations that follows is whole value, int part decimal part");
  // Serial.println("self");
  //Serial.println(d1[0]);
  // Serial.println(buffer[1]);
  //Serial.println(buffer[2]);
  //  Serial.println("other");
  // Serial.println(d1[1]);
  // Serial.println(buffer[3]);
  // Serial.println(buffer[4]);
  //analogWrite(ledpin, d1[0]);
  // Serial.print("leitura de luxs actuais     ");
  // Serial.println(calc_luxs(analogRead(analogPin)));
  //delay(200);

  //transmitir a informação necessária para que o outro arduino comele um novo ciclo do consensus
  transmit(buffer, address_aux, 6);




}


void transmit(byte * message, int address_dest, int tamanho)
{
  Wire.beginTransmission(address_dest); // transmit to device #8
  Wire.write(message, tamanho);       // sends five bytes
  Wire.endTransmission();    // stop transmitting
  //Serial.println("SentData");

}

void SerialInputs()
{

  int index = 0;

  //string de data recebida
  String input;

  //vetor de chars recebidos
  char rpiData[10] = "";

  //se utilizador escreveu algo
  if (Serial.available() > 0)
  {

    //le o que o utilizador escreveu
    while (Serial.available() > 0)
    {

      if (index < 9) // One less than the size of the array
      {
        rpiData[index] =  Serial.read(); //Read a character
        index++; // Increment where to write next
        rpiData[index] = '\0'; // Null terminate the string
      }

    }

    //converte vetor de chars em string
    input = String(rpiData);

    switch (rpiData[0])
    {
      case 'g':
        {

          request[0] = '#';
          request[1] = address;//address à qual todos os arduinos devem responder para que se faça os calculo de totais
          request[2] = rpiData[1];
          Serial.println(rpiData[1]);
          request[3] = 0;
          if (address == rpiData[2] - 'a')
          {
            flag_arduinos = 4;
            for (int i = 0; i < 10; i++)
              inData1[i] = request[i];
          } else
          {
            Serial.println("msg to onther arduino g case");

            transmit(request, rpiData[2] - 'a', 4);
          }

          break;

        }
      case 's':
        {

          request[0] = '#';
          request[1] = 's';
          request[2] = rpiData[1];
          request[3] = 0;
          if (address == rpiData[2] - 'a')
          {
            flag_arduinos = 4;
            for (int i = 0; i < 10; i++)
              inData1[i] = request[i];
          } else
            transmit(request, rpiData[2] - 'a', 4);
          break;
        }
      case 'r':
        {
          delay(1000);
          Serial.println("we are goint to reset the system pls w8 a few seconds whiel recalibration occcurs");
          Wire.beginTransmission(address_aux); // transmit to device
          Wire.write("O");
          Wire.endTransmission();
          // verificar
          flag = 2;
          on = 0;
        }
    }
  }
}


void recta_luxs()
{
  analogWrite(ledpin, 200);

  delay(1000);
  float luxs = 0;
  luxs =  calc_luxs(analogRead(analogPin));
  /*Serial.print(calc_luxs(analogRead(analogPin)));
    Serial.print(' ');
    Serial.println(200); */
  declive = 200 / luxs;
  //Serial.println(declive);
  delay(1000);


}

void shift_left(float current_luxs)
{
  last_luxs[lastMeasure % tamluxs] = current_luxs;
  lastMeasure++;
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
  usat = u;
  /*
    if (u > lookUp[255])
    {
      usat = lookUp[255];
    }else if (u< lookUp[0])
    {

      usat = lookUp[0];
    }*/
  if (u > 255 / declive)
  {
    usat = 255 / declive;
  } else if (u < 0)
  {

    usat = 0;
  }



  //apenas e diferentde de 0 se estiver saturado
  wind = usat - u;

  //adicionar a proxima iteracao integradora o anti windup para o sistema nao integrar bue
  integ += wind * K2 * 1.5;

  //procura valor na lookup table
  // pwm = search (u);
  // pwm = declive * u ;

  /*
    //write to pin pwm, if feedforward is on add that as well
    if (FFD == true)
      //analogWrite(led, search(reference + u));
      analogWrite(led, declive * (reference + u));
    else
      analogWrite(led, pwm);*/

  //write to pin pwm, if feedforward is on add that as well

  pwm = (declive * u) + d1[0] * 255 / 100;

  if (pwm < 0)
    pwm = 0;
  if (pwm > 255)
    pwm = 255;

  analogWrite(ledpin, pwm);


  //faz set das variaveis para o proximo loop
  y_ant = y;
  i_ant = integ;
  e_ant = e;


  //prints pedidos pelo prof
  //Serial.print(reference);
  // Serial.print(" ; ");
  //Serial.println(average());
  // Serial.print(" ; ");
  /*//if (FFD == true)
    // Serial.print(((search(reference+u))/255)* 100);
    //Serial.print(((declive*(reference+u))/255)* 100);
    //else
    Serial.print((pwm/255)* 100);
    Serial.println(" %");
    Serial.println(u);*/


  //Serial.print("pwmvalue : ");
  //Serial.println(pwm);
}





