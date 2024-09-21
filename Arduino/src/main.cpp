#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

const int ult_sensonr_rx = 2;          // Trigger pin of ultrasonic sensor
const int ult_sensor_tx = 3;          // Echo pin of ultrasonic sensor

// Constants and Variables
SoftwareSerial HC12(4, 5);   // HC-12 TX Pin 4, RX Pin 5
SoftwareSerial jsnSerial(ult_sensonr_rx, ult_sensor_tx);

const int power_radio = 8;

const int senzor_id = 1;
const int height_of_tank = 200;
const float alpha = 0.1;     // EMA Smoothing factor (between 0 and 1)
long distance_old = 0;
float ema_value = 0;         // Initial EMA value

volatile int f_wdt=1;
// I need to sleep it for 72 seconds
volatile int sleep_counts = 0;
const int max_sleep_rounds = 3;
const int max_measurements = 5;
const int max_transmit_loops = 10;

// Watchdog Interrupt Service. This is executed when watchdog timed out.
ISR(WDT_vect) {
	if(f_wdt == 0) {
		// here we can implement a counter the can set the f_wdt to true if
		// the watchdog cycle needs to run longer than the maximum of eight
		// seconds.
    sleep_counts++;
    if (sleep_counts >= max_sleep_rounds) {
      f_wdt=1;
      sleep_counts = 0;
    } 
	}
}

// Enters the arduino into sleep mode.
void enterSleep(void)
{
	// There are five different sleep modes in order of power saving:
	// SLEEP_MODE_IDLE - the lowest power saving mode
	// SLEEP_MODE_ADC
	// SLEEP_MODE_PWR_SAVE
	// SLEEP_MODE_STANDBY
	// SLEEP_MODE_PWR_DOWN - the highest power saving mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();

	// Now enter sleep mode.
	sleep_mode();

	// The program will continue from here after the WDT timeout

	// First thing to do is disable sleep.
	sleep_disable();

	// Re-enable the peripherals.
	power_all_enable();
}

// Setup the Watch Dog Timer (WDT)
void setupWatchDogTimer() {
	// The MCU Status Register (MCUSR) is used to tell the cause of the last
	// reset, such as brown-out reset, watchdog reset, etc.
	// NOTE: for security reasons, there is a timed sequence for clearing the
	// WDE and changing the time-out configuration. If you don't use this
	// sequence properly, you'll get unexpected results.

	// Clear the reset flag on the MCUSR, the WDRF bit (bit 3).
	MCUSR &= ~(1<<WDRF);

	// Configure the Watchdog timer Control Register (WDTCSR)
	// The WDTCSR is used for configuring the time-out, mode of operation, etc

	// In order to change WDE or the pre-scaler, we need to set WDCE (This will
	// allow updates for 4 clock cycles).

	// Set the WDCE bit (bit 4) and the WDE bit (bit 3) of the WDTCSR. The WDCE
	// bit must be set in order to change WDE or the watchdog pre-scalers.
	// Setting the WDCE bit will allow updates to the pre-scalers and WDE for 4
	// clock cycles then it will be reset by hardware.
	WDTCSR |= (1<<WDCE) | (1<<WDE);

	/**
	 *	Setting the watchdog pre-scaler value with VCC = 5.0V and 16mHZ
	 *	WDP3 WDP2 WDP1 WDP0 | Number of WDT | Typical Time-out at Oscillator Cycles
	 *	0    0    0    0    |   2K cycles   | 16 ms
	 *	0    0    0    1    |   4K cycles   | 32 ms
	 *	0    0    1    0    |   8K cycles   | 64 ms
	 *	0    0    1    1    |  16K cycles   | 0.125 s
	 *	0    1    0    0    |  32K cycles   | 0.25 s
	 *	0    1    0    1    |  64K cycles   | 0.5 s
	 *	0    1    1    0    |  128K cycles  | 1.0 s
	 *	0    1    1    1    |  256K cycles  | 2.0 s
	 *	1    0    0    0    |  512K cycles  | 4.0 s
	 *	1    0    0    1    | 1024K cycles  | 8.0 s
	*/
	WDTCSR  = (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
	// Enable the WD interrupt (note: no reset).
	WDTCSR |= _BV(WDIE);
}

void powerOnDevices() {
  // turn on radio
  pinMode(power_radio, OUTPUT);
  digitalWrite(power_radio, HIGH); 
}

void setup() {
  Serial.println("Initialising...");
  powerOnDevices(); delay(100);

  Serial.begin(9600);        // Initialize Serial communication at 9600 baud
  HC12.begin(9600);          // Initialize HC-12 communication at 9600 baud

	setupWatchDogTimer();
	Serial.println("Initialisation complete."); delay(100);

  jsnSerial.begin(9600);
}

float getEMA(int newReading, float prevEMA, float alpha) {
  return alpha * newReading + (1 - alpha) * prevEMA;
}

unsigned int getDistance(){
  //Serial.println("getDistance");
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
    }
    else{
      Serial.print("Distance [mm]: "); 
      Serial.println(distance);
	  return distance;
    } 
  } 

  return 0;
}

int getDistanceExt() {
  //Serial.println("getDistanceExt");
   
  jsnSerial.write(0x01); 
  while (jsnSerial.available() == 0){
	jsnSerial.write(0x01);  
	delay(50);
  }

  int distance = round(getDistance()/10); // convert to cm and round
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

void measureAndTransmit() {
  int raw_distance = getDistanceExt();         // Get raw distance from the sensor
  //ema_value = getEMA(raw_distance, ema_value, alpha);  // Apply EMA filter

  //int filtered_distance = round(ema_value); // Round EMA to the nearest integer
  int filtered_distance = raw_distance;

  Serial.println("filtered distance" +  filtered_distance);
  if (filtered_distance > 0 && filtered_distance < height_of_tank) {
    for (int i=0; i<max_transmit_loops; i++) {
      Transmit_data(senzor_id, filtered_distance);           // Send the filtered distance data
      delay(100);
    }
  }
}

void loop() {
  // Wait until the watchdog have triggered a wake up.
	if(f_wdt != 1) {
		return;
	}

	// clear the flag so we can run above code again after the MCU wake up
	f_wdt = 0;

	// Re-enter sleep mode.
  Serial.println("entering sleep");
	enterSleep();
  Serial.println("waking up");

  // measure after waking up
  //for (int i = 0; i < max_measurements; i++) {
  // powerOnDevices();
  delay(100);
  measureAndTransmit();
  //}
}
