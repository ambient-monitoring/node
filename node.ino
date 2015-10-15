#include <LowPower.h>
#include <VirtualWire.h>
#include <dht.h>

dht DHT;

//#define DEBUG 1
#define DHT22_PIN 1

const int led_pin = 0;
const int transmit_pin = 7;
const int light_pin = 3;  //define a pin for Photo resistor

/*
 * TODO
 * 
 * - reduce interval to send once a minute
 * - sleep for a short random time to prevent collisions
 * - schematic
 * - fix light level reading
 * - signal errors using LED
 */

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void setup()
{
  pinMode(led_pin, OUTPUT);

  // show we are alive
  for (int i = 0; i < 10; i++) {
    digitalWrite(led_pin, HIGH);
    delay(250);
    digitalWrite(led_pin, LOW);
    delay(250);
  }

  vw_set_tx_pin(transmit_pin);
  vw_setup(2000);
}

void loop()
{
  int chk = DHT.read22(DHT22_PIN);

  switch (chk)
  {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:
    case DHTLIB_ERROR_TIMEOUT:
    default:
      return;
  }

  int id = CHANGE_ME;
  double humidity = DHT.humidity;
  double temp = DHT.temperature;
  long vcc = readVcc();
  long light = analogRead(light_pin);

  char msg[30];
  char str_temp[6];
  char str_hum[6];
  
  /* 4 is mininum width, 2 is precision; float value is copied onto str_temp*/
  dtostrf(temp, 4, 2, str_temp);
  dtostrf(humidity, 4, 2, str_hum);

  sprintf(msg, "%d,%s,%s,%d,%d", id, str_temp, str_hum, vcc, light);

  digitalWrite(led_pin, HIGH); // Flash a light to show transmitting

  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait until the whole message is gone

  digitalWrite(led_pin, LOW);

#if defined(DEBUG)
  delay(1000);
#else
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
#endif
}

