#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

// Pin definitions for ATtiny85
const int ult_sensonr_rx = 0;  // Pin 0 for ultrasonic sensor RX
const int ult_sensor_tx = 1;   // Pin 1 for ultrasonic sensor TX
const int power_radio = 3;     // Pin 3 for power control of the HC-12

// Constants and Variables
SoftwareSerial HC12(2, 3);   // HC-12 TX on Pin 2, RX on Pin 3
SoftwareSerial jsnSerial(ult_sensonr_rx, ult_sensor_tx);

const int senzor_id = 1;
const int height_of_tank = 200;
const float alpha = 0.1;     // EMA Smoothing factor
long distance_old = 0;
float ema_value = 0;         // Initial EMA value

volatile int f_wdt = 1;
volatile int sleep_counts = 0;
const int max_sleep_rounds = 3;
const int max_measurements = 5;
const int max_transmit_loops = 10;

// Watchdog Interrupt Service Routine (ISR)
ISR(WDT_vect) {
  if (f_wdt == 0) {
    sleep_counts++;
    if (sleep_counts >= max_sleep_rounds) {
      f_wdt = 1;
      sleep_counts = 0;
    }
  }
}

// Enter ATtiny85 into sleep mode
void enterSleep(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Deep sleep mode
  sleep_enable();
  sleep_mode(); // Go to sleep
  sleep_disable(); // Wake up
  power_all_enable(); // Re-enable peripherals
}

// Setup Watchdog Timer (WDT)
void setupWatchDogTimer() {
  MCUSR &= ~(1 << WDRF); // Clear reset flag
  WDTCR |= (1 << WDCE) | (1 << WDE); // Set WDCE and WDE
  WDTCR = (1 << WDP2) | (1 << WDP1) | (1 << WDP0); // Set WDT to 8 seconds timeout
  WDTCR |= _BV(WDIE); // Enable WDT interrupt
}

void powerOnDevices() {
  // Turn on radio module
  pinMode(power_radio, OUTPUT);
  digitalWrite(power_radio, HIGH);
}

void setup() {
  // ATtiny85 doesn't have hardware serial, so we're using SoftwareSerial
  HC12.begin(9600);   // HC-12 at 9600 baud
  jsnSerial.begin(9600); // Ultrasonic sensor at 9600 baud
  
  setupWatchDogTimer();
}

float getEMA(int newReading, float prevEMA, float alpha) {
  return alpha * newReading + (1 - alpha) * prevEMA;
}

unsigned int getDistance() {
  unsigned int distance;
  byte startByte, h_data, l_data, sum = 0;
  byte buf[3];

  startByte = (byte)jsnSerial.read();
  if (startByte == 255) {
    jsnSerial.readBytes(buf, 3);
    h_data = buf[0];
    l_data = buf[1];
    sum = buf[2];
    distance = (h_data << 8) + l_data;
    if (((startByte + h_data + l_data) & 0xFF) != sum) {
      // Invalid checksum
    } else {
      return distance;
    }
  }
  return 0;
}

int getDistanceExt() {
  jsnSerial.write(0x01);
  while (jsnSerial.available() == 0) {
    jsnSerial.write(0x01);
    delay(50);
  }
  int distance = round(getDistance() / 10); // Convert to cm
  if (distance > 0 && distance < height_of_tank) {
    return distance;
  } else {
    return height_of_tank; // Return max value if out of range
  }
}

void Transmit_data(int senzor_id, int distance) {
  int crc_sum = senzor_id + distance;
  String message = String(senzor_id) + "," + String(distance) + "," + String(crc_sum);
  HC12.println(message);
}

void measureAndTransmit() {
  int raw_distance = getDistanceExt();
  int filtered_distance = raw_distance; // Apply EMA filter if necessary

  if (filtered_distance > 0 && filtered_distance < height_of_tank) {
    for (int i = 0; i < max_transmit_loops; i++) {
      Transmit_data(senzor_id, filtered_distance);
      delay(100);
    }
  }
}

void loop() {
  if (f_wdt != 1) {
    return; // Wait for WDT wakeup
  }

  f_wdt = 0; // Clear the flag for the next cycle

  enterSleep(); // Go to sleep and wake up via WDT

  delay(100); // Delay before measurement
  measureAndTransmit();
}
