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
bool pronto = false;

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
  
}


void inicio()
{
  //calibrar();
  Serial.print("ola");
}
  
void loop() {
  Wire.beginTransmission(address_aux); // transmit to device #8
 Wire.write("x is ");        // sends five bytes
  Wire.write(address);              // sends one byte
  Wire.endTransmission();    // stop transmitting

  
  delay(100);

  if(millis() - temp > 300)
  {
    pronto = false;
  }
  Serial.print(pronto);
  
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
    
  delay(250);

  Serial.println(calc_luxs(analogRead(analogPin)));
  
  if(address == 2)
  {
    digitalWrite(ledpin, HIGH);
  }
  else
  {
    digitalWrite(ledpin, LOW);
  }
    
  Serial.println(calc_luxs(analogRead(analogPin)));
  
  
  
  
  
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  bool pronto_antes = pronto;
  while (1 < Wire.available()) { // loop through all but the last
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
    pronto = true;
    temp = millis();
  }

  if(pronto_antes == false && pronto == true)
  {
    inicio();
  }
  
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
}
