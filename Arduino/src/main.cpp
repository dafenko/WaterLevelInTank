#include <Arduino.h>

//  Created and Designed by Manusha Ramanayake 
//  Email   : ramanayakemanusha@gmail.com


//Transmitter  and Sensor Unit

# include <SoftwareSerial.h>
# include "avr8-stub.h"
# include "app_api.h"

SoftwareSerial HC12(4, 5);

int height_of_tank = 190;
int min_level_of_water = 30; //This is the sensor reading when the tank is empty
int max_level_of_water = height_of_tank - 25; //This is the sensor reading  when the tank is full


long level_of_water_old = 0;
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

void Transmit_data(int senzor_id, int percentage_of_full, int distance) {
  int crc_sum = senzor_id + percentage_of_full + distance;
  String message = String(1) + "," + String(percentage_of_full) + "," + String(distance) + "," + String(crc_sum);
  HC12.println(message); 
}

void Send_data(int distance) {

  int level_of_water = height_of_tank - distance;
  // maps the sensor values to a number  between 0 and 100
  int percentage_of_full = map (level_of_water, min_level_of_water, max_level_of_water, 0, 100);

  //This sends data every 100 ms
  if (Time == 1000) {
    Transmit_data(1, percentage_of_full, distance);
    Time = 0;
  }
 //This sends data instantly when tank water level is changing
  if (level_of_water != level_of_water_old) {   
    Transmit_data(1, percentage_of_full, distance);
    level_of_water_old = level_of_water;
  }
}

void  loop() {
  Time++;
  // Standerd code for getting value from the ultrasonic  sensor
  digitalWrite(trig, LOW); 
  delayMicroseconds(5);
  digitalWrite(trig,  HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  int distance  = ( pulseIn(echo, HIGH, 100))*0.034/2;   //this gets the value in centimeters

  if ((0 < distance) && (distance < height_of_tank)) {
    Send_data(distance);
  }
  
} 

