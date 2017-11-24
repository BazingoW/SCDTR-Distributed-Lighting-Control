// Wire Master Writer
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>

#define analogPin 1

//Resistencia do circuito do LDR
#define r1 10000.0

int ledpin = 9;

int pin_verif = 2;
int address, address_aux;
long  temp;
int flag = 0;
bool on=0;

void setup() {

  // define os endereços do arduino segundo o estado do pin
  if(digitalRead(pin_verif) == HIGH)
  {
    address = 2;
    address_aux = 1;
  }
  else
  {
    address = 1;
    address_aux = 2;
  }

  
  
  Wire.begin(address); // join i2c bus (address optional for master)
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output


  Wire.beginTransmission(address_aux); // transmit to device #8
  Wire.write("O");        // sends five bytes
  Wire.endTransmission();    // stop transmitting
  Serial.println("SentData");
}



void loop() {


if(flag>0)
{
if(flag==1)
{
  Wire.beginTransmission(address_aux); // transmit to device #8
  Wire.write("R");        // sends five bytes
  Wire.endTransmission();    // stop transmitting
  Serial.println("SentData");
calibrar1();
on=1;

}
else if(flag==2)
{
  calibrar1();
  on=1;
}
flag=0;
}


if(on)
{
  //Main Loop
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
  luxs = pow(10, luxs);

  return luxs;
}

//verifica se arduino nao esta a mandar muita informação há algum tempo


//função que calibra ambos os leds dos arduinos
void calibrar()
{
  if(address == 1)
  {
    digitalWrite(ledpin, HIGH);
  }
  else
  {
    digitalWrite(ledpin, LOW);
  }
    
  delay(5000);

  Serial.println(calc_luxs(analogRead(analogPin)));
  
  if(address == 2)
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

//vetor de chars recebidos
char inData[10] = "";

  while (Wire.available()>0) { // loop through all but the last

        inData[index] =  Wire.read(); //Read a character
        index++; // Increment where to write next
       // inData[index] = '\0'; // Null terminate the string
      
    
   }
   Serial.println("ReceivedData");
   Serial.println(inData);

if(inData[0]=='O')
    {//se recebi um O, reenvio um R e calibro
      flag=1;
    }
    else if(inData[0]=='R')
    {
     flag=2;
    }
   
}


void calibrar1()
{

   Serial.println("CalibFunc");
    digitalWrite(ledpin, LOW);
 delay(1000);
    digitalWrite(ledpin, HIGH);
  

   delay(1000);
    digitalWrite(ledpin, LOW);
   delay(1000);
 
  
  
  
  
  
}


