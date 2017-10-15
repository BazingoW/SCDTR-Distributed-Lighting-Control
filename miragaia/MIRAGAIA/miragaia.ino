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
#define tamluxs 5

int led = 9;
int analogPin = 1;
int val = 0;
int brightness = 0;
float tensao = 0.0;
float R2 = 0.0;
float r1 = 10000.0;
int contador = 0;
int primeiro = 0;
int contador2 = 0;
float luxs = 0;
int i = 0;
float vetor[256];

int ilum_min = 3;
float last_luxs[tamluxs];

//novas variaveis de controlo

float K1, K2, Kp, Ki, b, y, e , k, p,u, y_ant = 0, i_ant = 0, e_ant = 0, T;


//função que calcula a média dos luxs
float average()
{
  float somatodos = 0, media = 0;
  int i;

  for (i = 0; i < tamluxs; i++)
  {
    somatodos = last_luxs[i] + somatodos;
  }

  media = somatodos / tamluxs;

  return media;
}



//função que calcula o valor de luxs
float calc_luxs(int val)
{

  tensao = (5 * val) / 1023.0;
  R2 = ((r1 * 5) / tensao) - r1;

  luxs = ((log10(R2) - 4.8451) / -0.7186);
  luxs = pow(10, luxs);

  return luxs;

}

//função que preenche look up table
void calibration()
{
  for (i = 0; i < 256; i++)
  {
    delay(50);              // wait for a second

    analogWrite(led, brightness);



    val = analogRead(analogPin);

    brightness = brightness + 1;


    luxs = calc_luxs(val);

    vetor[i] = luxs;

    //Serial.print("tensao ");
    //Serial.println(tensao);

    //Serial.print("VAL ");
    //Serial.println(val);

    //Serial.print("LUXS ");
    Serial.print(luxs);
    Serial.print(' ');

    Serial.println(brightness);


    //Serial.println(R2);

  }

  Serial.print("calibration feita");

}


void shift_left(float current_luxs)
{
  for (int i = 1; i < tamluxs; i++)
  {
    last_luxs[i - 1] = last_luxs[i];
  }
  last_luxs[tamluxs - 1] = current_luxs;
}

void controlo()
{
  val = analogRead(analogPin);
  
  y = calc_luxs(val);
  
  e = ilum_min - y;
  
  p = K1*ilum_min-Kp*y;

  k = i_ant + K2*(e + e_ant);

  u = p + k;

  analogWrite(led, u);

  y_ant = y;
  i_ant = k;
  e_ant = e;
    
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.

  int i;
  for (i = 0; i < tamluxs; i++)
  {
    last_luxs[i] = -1;
  }

  K1 = Kp*b;
  K2 = Kp*Ki*T/2;

  
  
  Serial.begin(9600);
  analogWrite(led, brightness);
  calibration();

}


// the loop function runs over and over again forever
void loop() {
  analogWrite(led, 228);   // turn the LED on (HIGH is the voltage level)

  delay(25);
  val = analogRead(analogPin);

  luxs = calc_luxs(val);

  shift_left(luxs);
  
  //Serial.println(luxs);
  Serial.println(average());


}
