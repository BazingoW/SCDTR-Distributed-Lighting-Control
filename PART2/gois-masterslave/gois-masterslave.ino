// Wire Master Writer
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>


int pin_verif = 2;
int address, address_aux;

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



  
void loop() {
  Wire.beginTransmission(address_aux); // transmit to device #8
  Wire.write("x is ");        // sends five bytes
  Wire.write(address);              // sends one byte
  Wire.endTransmission();    // stop transmitting

  
  delay(500);

  
}


// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  while (1 < Wire.available()) { // loop through all but the last
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
}
