#include <Arduino.h>

//  Created and Designed by Manusha Ramanayake 
//  Email   : ramanayakemanusha@gmail.com


//Transmitter  and Sensor Unit

# include <SoftwareSerial.h>
# include "avr8-stub.h"
# include "app_api.h"

SoftwareSerial HC12(4, 5);

//Change  the values below to calibrate the meter
int MinLevel = 24; //This is the sensor  reading when the tank is empty
int MaxLevel = 102; //This is the sensor reading  when the tank is full

long dist_1 = 0;
long dist_2 =  0;
int Time = 0;
int trig = 3; //trigger pin of ultrasonic sensor
int echo = 2; //Echo pin of ultrasonic sensor

void setup() {
  debug_init();
//To calibrate the  sensor delete the comment symbols in the following code
  //Serial.begin(9600);
  HC12.begin(9600);
 pinMode(trig, OUTPUT);
 digitalWrite(trig, LOW);
 pinMode(echo,  INPUT);
}


void Send_data(int dist_1){
//maps the sensor values to a number  between 0 and 100
 int sent = map (dist_1, MinLevel, MaxLevel, 100, 0);

  //This sends data every 100 ms
 if (Time == 100) {  
  HC12.write(sent);
  Time = 0;
 }
 //This sends data instantly when tank water level is changing
  if (dist_1 != dist_2) {   
    HC12.write(sent);
    dist_2 = dist_1;
  }
 
 //To calibrate the sensor delete the comment symbols in the following  code
 /* Serial.print("    sensor read- ");
  Serial.print(dist_1);
  Serial.print("    sent value - ");
  Serial.print(sent);
  Serial.print('\n'); */
}


void  loop() {
 
  Time++;
  // Standerd code for getting value from the ultrasonic  sensor
  digitalWrite(trig, LOW); 
  delayMicroseconds(5);
  digitalWrite(trig,  HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  dist_1  = ( pulseIn(echo, HIGH, 100))*0.034/2;   //this gets the value in centimeters
  
  if (dist_1 < MaxLevel+100){  //change if needed
  Send_data(dist_1);}
  
  //debuging code
  //Serial.print(dist_1);
  // Serial.print('\n');
  // delay(500);
} 

