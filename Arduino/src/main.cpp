#include <Arduino.h>

//  Created and Designed by Manusha Ramanayake 
//  Email   : ramanayakemanusha@gmail.com


//Transmitter  and Sensor Unit

# include <SoftwareSerial.h>
// # include "avr8-stub.h"
# include "app_api.h"

SoftwareSerial HC12(4, 5);

long distance_old = 0;
int Time = 0;
int trig = 3; //trigger pin of ultrasonic sensor
int echo = 2; //Echo pin of ultrasonic sensor
int senzor_id = 1;
int height_of_tank = 200;
int OUTLIER_THRESHOLD=5;
int INLIER_THRESHOLD=5;

void setup() {
//  debug_init();
  //To calibrate the  sensor delete the comment symbols in the following code
  Serial.begin(9600);
  HC12.begin(9600);
  pinMode(trig, OUTPUT);
  digitalWrite(trig, LOW);
  pinMode(echo,  INPUT);
}

void Transmit_data(int senzor_id, int distance) {
  int crc_sum = senzor_id + distance;
  String message = String(senzor_id) + "," + String(distance) + "," + String(crc_sum);
  HC12.println(message);
  Serial.println(message); 
}

void Send_data(int distance) {

   //This sends data every 100 ms
  if (Time > 1000) {
    Serial.println("Sending because time");
    Transmit_data(senzor_id, distance);
    Time = 0;

    return;
  }

  //This sends data instantly when tank water level is changing
  if (distance != distance_old) {   
    Transmit_data(senzor_id, distance);
    distance_old = distance;
  }

  return;
}

int getDistance() {
  // Standerd code for getting value from the ultrasonic  sensor
  digitalWrite(trig, LOW); 
  delayMicroseconds(5);
  digitalWrite(trig,  HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  return(pulseIn(echo, HIGH, 30000)*0.034/2);   //this gets the value in centimeters
}

int getAverageDistance(int numSamples) {
  long sum = 0;
  for (int i = 0; i < numSamples; i++) {
    int distance = getDistance();
    sum += distance;
    delay(50);  // Small delay between readings
  }
  return sum / numSamples;
}

void  loop() {
  Time++;

  int distance = getAverageDistance(30);
  if ((0 < distance) && (distance < height_of_tank)) {
    //Serial.println(distance);
    Send_data(distance);
  }
  else 
  {
    //Serial.println("Wrong reading " + String(distance));
    distance_old = distance;
  }
  
} 

