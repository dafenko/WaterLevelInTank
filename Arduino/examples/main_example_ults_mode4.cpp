/* Example hwo to handle mode 4 of ultrasound sensor */

#include <Arduino.h>
#include <SoftwareSerial.h>
#define rxPin 2
#define txPin 3
 
SoftwareSerial jsnSerial(rxPin, txPin);

void getDistance(){
  unsigned int distance;
  byte startByte, h_data, l_data, sum = 0;
  byte buf[3];
  
  startByte = (byte)jsnSerial.read();
  if(startByte == 255){
    jsnSerial.readBytes(buf, 3);
    h_data = buf[0];
    l_data = buf[1];
    sum = buf[2];
    distance = (h_data<<8) + l_data;
    if((( startByte + h_data + l_data)&0xFF) != sum){
      Serial.println("Invalid result");
      Serial.println(h_data);
      Serial.println(l_data);
      Serial.println(sum);
    }
    else{
      Serial.print("Distance [mm]: "); 
      Serial.println(distance);
    } 
  } 
  else return;
}

void setup() {
  jsnSerial.begin(9600);
  Serial.begin(9600);
}
 
void loop() {
    jsnSerial.write(0x01);
    delay(50);
  if(jsnSerial.available()){
    getDistance();
  }
}
