#include <Wire.h>

#define analogPin 1
#define n_iter 25

//Resistencia do circuito do LDR
#define r1 10000.0
#define rh 0.01

float kself, kmutuo;
int ledpin = 9;

int pin_verif = 2;
int address, address_aux;
long  temp;
int flag = 0;
bool on = 0;
float ext_illum = 0;
float min_best[n_iter];
byte buffer[8] ;
int address_sender;


//System
float k11 = 2, k12 = 1;
float L1 = 150, o1 = 30;
//float K[][2] = {{k11, k12}, {k21, k22}};
//float L[][1] = {{L1}, {L2}};
//float o[][1] = {{o1}, {o2}};

//Cost function
float c1 = 1, c2 = 1;
float c[] = {c1, c2};
float q1 = 0.0, q2 = 0.0;
//float Q[][2] = {{q1, 0}, {0, q2}};

//Consensus variables
float rho = 0.01;
//node 1
float d1[] = {0, 0};
float d1_av[] = {0, 0};
float d1_copy[] = {0, 0};
float y1[] = {0, 0};
float k1[] = {k11, k12};

//necessário? i think so
//node 2
/*
float d2[] = {0, 0};
float d2_av[] = {0, 0};
float y2[] = {0, 0};
float k2[] = {k21, k22};*/
float* duty_fruty;


void setup() {

  // define os endereços do arduino segundo o estado do pin
  // define initial message to send to other arduinos



  if (digitalRead(pin_verif) == HIGH)
  {
    address = 2;
    address_aux = 1;
  }
  else
  {
    address = 1;
    address_aux = 2;
  }
  // formato da mensagem será '#-address-mensagetosend'it is currently used 1 byte per item, the mensage to send can be changed to type of data -value, or if it is knowned only the values
  //aquando de receber a mesagem tranformar o valor de byte em inteiro  int(buffer[2])
  buffer[0] = '#';
  buffer[1] = '0' + address;
  buffer[2] = 123;





  Wire.begin(address); // join i2c bus (address optional for master)
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output

  Wire.beginTransmission(address_aux); // transmit to device
  Wire.write("O");
  Wire.endTransmission();    // stop transmitting
  Serial.println("SentData");
  //example of how it should be read the message
  Serial.println(int(buffer[2]));

}



void loop() {


  if (flag > 0)
  {
    if (flag == 1)
    {
      Wire.beginTransmission(address_aux); // transmit to device #8
      Wire.write("R");        // sends five bytes
      Wire.endTransmission();    // stop transmitting
      Serial.println("SentData");
      calibrar1();
      on = 1;

    }
    else if (flag == 2)
    {
      calibrar1();
      on = 1;
    }
    flag = 0;
  }


  if (on)
  {
    //Main Loop
    // insert in
    
   
     delay(1000);
     buffer[2] = 'K';// K k maiusculo representa o kself e o minusculo o mutu
     buffer[3] = k11;
     buffer[4] = 'k';
     buffer[5] = k12;
     buffer[6] = 'O';
     buffer[7] = o1;
    

      delay (1000);
     duty_fruty = iteracao();
      //transmit(buffer,address_aux);
     Serial.println(duty_fruty[0]);
     
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

  return luxs;
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

  if (address == 2)
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
void receiveEvent(int howMany) {

  int index = 0;

  //string de data recebida
  String input;
  int address_sender;
  char Label;

  //vetor de chars recebidos
  char inData[10] = "";

  while (Wire.available() > 0) { // loop through all but the last

    inData[index] =  Wire.read(); //Read a character
    index++; // Increment where to write next
    inData[index] = '\0'; // Null terminate the string


  }
  Serial.println("ReceivedData");
  Serial.println(inData);
  //  Serial.println(size(howMany))

  if (inData[0] == 'O')
  { //se recebi um O, reenvio um R e calibro
    flag = 1;
  }
  else if (inData[0] == 'R')
  {
    flag = 2;
  }/*
  else if (inData[0] == '#')//communicação recebida de outro arduino
   {
      address_sender = int(inData[1]);
      for (int i=2;i<=length(inData);i++)
      {
        if(!i%2)
        {
            Label = inData[i];  
        }else
        {
          switch(Label)
          {
            case: 'K'
            {
              k
            }
            }
          
        }


        
      }
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
    k11 = (calc_luxs(analogRead(analogPin)))/255.0;
    //k11 = analogRead(analogPin);

  }
  else
  {
    k12 = (calc_luxs(analogRead(analogPin)))/255.0;
    //k12 = analogRead(analogPin);
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
    k11 = (calc_luxs(analogRead(analogPin)))/255.0;
    //k11 = analogRead(analogPin);

  }
  else
  {
    k12 = (calc_luxs(analogRead(analogPin)))/255.0;
    //k12 = analogRead(analogPin);
  }

  delay(500);

  analogWrite(ledpin, 0);
   delay(500);

  o1 = (calc_luxs(analogRead(analogPin))) / 255.0;

  Serial.println(k11);
  Serial.println(k12);
  Serial.println(o1);


}


float* iteracao()
{

  int i;
  float min_best_1[50];
  float z11, z12;
  float min_unconstrained = 0;

  for (i = 0; i < 50; i++)
  {
    //node 1
    float d11_best = -1;
    float d12_best = -1;

    min_best_1[i] = 100000;

    float sol_unconstrained = 1, sol_boundary_linear = 1, sol_boundary_0 = 1, sol_boundary_100 = 1;
    float sol_linear_0 = 1, sol_linear_100 = 1;


    z11 = -c1 - y1[0] + rho * d1_av[0];
    z12 = -y1[1] +  rho * d1_av[1];


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
    int d11u = p11 * z11;
    int d12u = p12 * z12;


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


    //compute function value and if best store new optimum
    if (sol_unconstrained)
    {
      min_unconstrained = sq(0.5 * q1 * d11u) + c1 * d11u + y1[0] * (d11u - d1_av[0]) + y1[1] * (d12u - d1_av[1]) + rho / 2 * sq(d11u - d1_av[0]) + rho / 2 * sq(d12u - d1_av[1]);

      if (min_unconstrained < min_best_1[i])
      {
        d11_best = d11u;
        d12_best = d12u;
        min_best_1[i] = min_unconstrained;
      }
    }


    //compute minimum constrained to linear boundary
    int d11bl = p11 * z11 + p11 * k11 / n * (w1 - u1);
    int d12bl = p12 * z12 + p12 * k12 / n * (w1 - u1);


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
      float min_boundary_linear = 0.5 * q1 * sq(d11bl) + c1 * d11bl + y1[0] * (d11bl - d1_av[0]) + y1[1] * (d12bl - d1_av[1]) + rho / 2 * sq(d11bl - d1_av[0]) + rho / 2 * sq(d12bl - d1_av[1]);

      if (min_boundary_linear < min_best_1[i])
      {
        d11_best = d11bl;
        d12_best = d12bl;
        min_best_1[i] = min_boundary_linear;
      }
    }


    //compute minimum constrained to boundary
    int d11b0 = 0;
    int d12b0 = p12 * z12;


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
      float min_boundary_0 = 0.5 * q1 * sq(d11b0) + c1 * d11b0 + y1[0] * (d11b0 - d1_av[0]) + y1[1] * (d12b0 - d1_av[2]) + rho / 2 * sq(d11b0 - d1_av[0]) + rho / 2 * sq(d12b0 - d1_av[1]);

      if (min_boundary_0 < min_best_1[i])
      {
        d11_best = d11b0;
        d12_best = d12b0;
        min_best_1[i] = min_boundary_0;
      }
    }


    //compute minimum constrained to 100 boundary
    int d11b100 = 100;
    int d12b100 = p12 * z12;

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
      float min_boundary_100 = 0.5 * q1 * sq(d11b100) + c1 * d11b100 + y1[0] * (d11b100 - d1_av[0]) + y1[1] * (d12b100 - d1_av[1]) + rho / 2 * sq(d11b100 - d1_av[0]) + rho / 2 * sq(d12b100 - d1_av[1]);

      if (min_boundary_100 < min_best_1[i])
      {
        d11_best = d11b100;
        d12_best = d12b100;
        min_best_1[1] = min_boundary_100;
      }
    }


    //compute minimum constrained to linear and zero boundary
    int common = (rho + q1) / ((rho + q1) * n - k11 * k11); //ou float?
    int det1 = common;
    int det2 = -k11 * common;
    int det3 = det2;
    int det4 = n * (rho + q1) * common;
    int x1 = det1 * w1 + det2 * w2;
    int x2 = det3 * w1 + det4 * w2;
    int v1 = det1 * u1 + det2 * u2; //u2 = 0 so this can be simplified
    int v2 = det3 * u1 + det4 * u2; //u2 = 0 so this can be simplified
    int d11l0 = p11 * z11 + p11 * k11 * (x1 - v1) + p11 * (x2 - v2);
    int d12l0 = p12 * z12 + p12 * k12 * (x1 - v1);
    //check feasibility
    if (d11l0 > 100)
    {
      sol_linear_0 = 0;
    }


    //compute function value and if best store new optimum
    if (sol_linear_0)
    {
      float min_linear_0 = 0.5 * q1 * sq(d11l0) + c1 * d11l0 + y1[0] * (d11l0 - d1_av[0]) + y1[1] * (d12l0 - d1_av[1]) + rho / 2 * sq(d11l0 - d1_av[0]) + rho / 2 * sq(d12l0 - d1_av[1]);

      if (min_linear_0 < min_best_1[1])
      {
        d11_best = d11l0;
        d12_best = d12l0;
        min_best_1[i] = min_linear_0;
      }

    }


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
      float  min_linear_100 = 0.5 * q1 * sq(d11l100) + c1 * d11l100 + y1[0] * (d11l100 - d1_av[0]) + y1[1] * (d12l100 - d1_av[1]) + rho / 2 * sq(d11l100 - d1_av[0]) + rho / 2 * sq(d12l100 - d1_av[1]);

      if (min_linear_100 < min_best_1[1])
      {
        d11_best = d11u;
        d12_best = d12u;
        min_best_1[i] = min_linear_100;
      }
    }


    //store data and save for next cycle
    best_d11[i] = d11_best;
    best_d12[i] = d12_best;
    float d1[] = {d11_best, d12_best};


    //compute average with available knowledge
    d1_av[0] = (d1[0] + d1_copy[0]) / 2;
    d1_av[1] = (d1[1] + d1_copy[1]) / 2;
    //update local lagrangian
    y1[0] = y1[0] + rho * (d1[0] - d1_av[0]);
    y1[1] = y1[1] + rho * (d1[1] - d1_av[1]);
    //send node 1 solution to neighboors
    //d1_copy = d1;
    //dunno if this works
    d1_copy[0] = d1[0];
    d1_copy[1] = d1[1];
  }
return d1;
}


void transmit(byte* message, int address_dest)
{
  Wire.beginTransmission(address_dest); // transmit to device #8
  Wire.write(message, 3);       // sends five bytes
  Wire.endTransmission();    // stop transmitting
  Serial.println("SentData");

}


