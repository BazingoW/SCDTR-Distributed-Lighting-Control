/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the Uno and
  Leonardo, it is attached to digital pin 13. If you're unsure what
  pin the on-board LED is connected to on your Arduino model, check
  the documentation at http://www.arduino.cc

  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
 */
 int led = 9;
int analogPin = 1;
int val = 0;
int brightness = 0;
float mira = 0.0;
float R2 = 0.0;
float r1 = 10000.0;
int contador = 0;
int primeiro = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
  
  Serial.begin(9600);
  
}

// the loop function runs over and over again forever
void loop() {
  //analogWrite(led, brightness);   // turn the LED on (HIGH is the voltage level)
  
  
  
  delay(100);              // wait for a second

  analogWrite(led, contador);
  contador += 10;
  
  if(contador > 250)
  {
    primeiro = 1;
    contador = 0;
  }


  val=analogRead(analogPin);
  
  /*brightness=map(val,0,1023,0,255);
  brightness = 255 - brightness;

  mira = (5*brightness)/255.0;
  R2 = ((r1*5)/mira) - r1;*/

  //Serial.println(mira); 
  if(primeiro == 0)
  
  Serial.println(val); 
  //Serial.println(R2); 

  
  
}
