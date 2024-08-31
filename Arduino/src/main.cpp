#include <Arduino.h>
#include <SoftwareSerial.h>

// Constants and Variables
SoftwareSerial HC12(4, 5);   // HC-12 TX Pin 4, RX Pin 5

const int trig = 3;          // Trigger pin of ultrasonic sensor
const int echo = 2;          // Echo pin of ultrasonic sensor
const int power_radio = 8;
const int power_ult_sensor_1 = 6;
const int power_ult_sensor_2 = 7;

const int senzor_id = 1;
const int height_of_tank = 200;
const float alpha = 0.1;     // EMA Smoothing factor (between 0 and 1)
long distance_old = 0;
int Time = 0;
float ema_value = 0;         // Initial EMA value

void powerOnDevices() {
  // turn on radio
  pinMode(power_radio, OUTPUT);
  digitalWrite(power_radio, HIGH); 
  // turn on ultrasound sensor
  pinMode(power_ult_sensor_1, OUTPUT);
  pinMode(power_ult_sensor_2, OUTPUT);
  digitalWrite(power_ult_sensor_1, HIGH); 
  digitalWrite(power_ult_sensor_2, HIGH); 

  delay(100);  // Adjust as needed
}

void setup() {
  powerOnDevices();

  Serial.begin(9600);        // Initialize Serial communication at 9600 baud
  HC12.begin(9600);          // Initialize HC-12 communication at 9600 baud
  pinMode(trig, OUTPUT);     
  digitalWrite(trig, LOW);   // Set trigger pin to LOW
  pinMode(echo, INPUT);      // Set echo pin as input
}

float getEMA(int newReading, float prevEMA, float alpha) {
  return alpha * newReading + (1 - alpha) * prevEMA;
}

int getDistance() {
  digitalWrite(trig, LOW); 
  delayMicroseconds(5);
  digitalWrite(trig, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  long duration = pulseIn(echo, HIGH, 30000);  // Timeout after 30ms
  int distance = (duration * 0.034) / 2;       // Convert duration to distance (cm)

  // Return distance only if it's within a reasonable range
  if (distance > 0 && distance < height_of_tank) {
    return distance;
  } else {
    return height_of_tank; // Return a maximum value if out of range
  }
}

void Transmit_data(int senzor_id, int distance) {
  int crc_sum = senzor_id + distance;
  String message = String(senzor_id) + "," + String(distance) + "," + String(crc_sum);
  HC12.println(message);
  Serial.println(message); 
}

void Send_data(int distance) {
  // Send data every 100 ms or instantly if the distance changes
  if (Time > 10 || distance != distance_old) {
    Transmit_data(senzor_id, distance);
    Time = 0;
    distance_old = distance;
  }
}

void loop() {
  Time++;
  
  int raw_distance = getDistance();         // Get raw distance from the sensor
  ema_value = getEMA(raw_distance, ema_value, alpha);  // Apply EMA filter

  int filtered_distance = round(ema_value); // Round EMA to the nearest integer

  if (filtered_distance > 0 && filtered_distance < height_of_tank) {
    Send_data(filtered_distance);           // Send the filtered distance data
  }

  delay(100);  // Adjust as needed
}
