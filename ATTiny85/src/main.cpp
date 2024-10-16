#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <SoftwareSerial.h>


byte saveADCSRA;                      // variable to save the content of the ADC for later. if needed.
volatile byte counterWD = 0;            // Count how many times WDog has fired. Used in the timing of the 
                                           // loop to increase the delay before the LED is illuminated. For example,
                                        // if WDog is set to 1 second TimeOut, and the counterWD loop to 10, the delay
                                        // between LED illuminations is increased to 1 x 10 = 10 seconds

// device specific
const int senzor_id = 1;
const int height_of_tank = 200;

const int hc12_tx = PB1;
const int hc12_rx = PB0;
const int ult_sensonr_rx = PB3;  // Pin 0 for ultrasonic sensor RX
const int ult_sensor_tx = PB4;   // Pin 1 for ultrasonic sensor TX
const int power_radio = PB2; 
const int max_transmit_loops = 10;
const int max_sleep_rounds = 9;

SoftwareSerial HC12(hc12_tx, hc12_rx);   // HC-12 TX on Pin 2, RX on Pin 3
SoftwareSerial jsnSerial(ult_sensonr_rx, ult_sensor_tx);

void resetWatchDog ()
{
  MCUSR = 0;
  WDTCR = bit ( WDCE ) | bit ( WDE ) | bit ( WDIF ); // allow changes, disable reset, clear existing interrupt
  WDTCR = bit ( WDIE ) | bit ( WDP2 )| bit ( WDP1 ); // set WDIE ( Interrupt only, no Reset ) and 1 second TimeOut
                                                     
  wdt_reset ();                            // reset WDog to parameters
  
} // end of resetWatchDog ()

void sleepNow ()
{
  set_sleep_mode ( SLEEP_MODE_PWR_DOWN ); // set sleep mode Power Down
  saveADCSRA = ADCSRA;                    // save the state of the ADC. We can either restore it or leave it turned off.
  ADCSRA = 0;                             // turn off the ADC
  power_all_disable ();                   // turn power off to ADC, TIMER 1 and 2, Serial Interface
  
  noInterrupts ();                        // turn off interrupts as a precaution
  resetWatchDog ();                       // reset the WatchDog before beddy bies
  sleep_enable ();                        // allows the system to be commanded to sleep
  interrupts ();                          // turn on interrupts
  
  sleep_cpu ();                           // send the system to sleep, night night!

  sleep_disable ();                       // after ISR fires, return to here and disable sleep
  power_all_enable ();                    // turn on power to ADC, TIMER1 and 2, Serial Interface
  
  // ADCSRA = saveADCSRA;                 // turn on and restore the ADC if needed. Commented out, not needed.
  
} // end of sleepNow ()

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
void setup ()
{
  resetWatchDog ();                     // do this first in case WDog fires
  pinMode ( power_radio, OUTPUT );           // I could put to INPUT between sleep_enable() and interrupts()
                                        // to save more power, then to OUTPUT in the ISR after wdt_disable()
  HC12.begin(9600);   // HC-12 at 9600 baud
  jsnSerial.begin(9600); // Ultrasonic sensor at 9600 baud

} // end of setup

void loop ()
{
  if ( counterWD == max_sleep_rounds )                 // if the WDog has fired max_sleep_rounds times......
  {
    digitalWrite ( power_radio, HIGH );        // flash the led on for 100ms
    measureAndTransmit();                      // measure and transmit
    digitalWrite ( power_radio, LOW );
    counterWD = 0;                        // reset the counterWD for another 10 WDog firings
  } // end of if counterWD
  
  sleepNow();                          // then set up and enter sleep mode
  
} // end of loop ()


ISR ( WDT_vect )
{
  wdt_disable ();                           // until next time....
  counterWD ++;                             // increase the WDog firing counter. Used in the loop to time the flash
                                            // interval of the LED. If you only want the WDog to fire within the normal 
                                            // presets, say 2 seconds, then comment out this command and also the associated
                                            // commands in the if ( counterWD..... ) loop, except the 2 digitalWrites and the
                                            // delay () commands.
} // end of ISR