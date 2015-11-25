#include <LowPower.h>
#include <VirtualWire.h>
#include <dht.h>

dht DHT;

//#define DEBUG 1

const int dht22_pin = 1;
const int led_pin = A0;
const int transmit_pin = 7;

// packet counter
int counter = 0;

/*
 * TODO
 *
  * - schematic
  * - signal errors using LED
 */

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(MUX5) | _BV(MUX0);

  // Wait for Vref to settle
  delay(2);

  // Start conversion
  ADCSRA |= _BV(ADSC);

  // measuring
  while (bit_is_set(ADCSRA, ADSC));

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  result = 1125300L / result;

  // Vcc in millivolts
  return result;
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

  randomSeed(analogRead(A5));

  vw_set_tx_pin(transmit_pin);
  vw_setup(2000);
}

void loop()
{
  // read values from DHT22 sensor
  int chk = DHT.read22(dht22_pin);

  // check read status, abort if error
  switch (chk)
  {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:
    case DHTLIB_ERROR_TIMEOUT:
    default:
      return;
  }

  // sensor id, change it before flashing
  int id = CHANGE_ME;

  // humidity
  double humidity = DHT.humidity;

  // temperature
  double temp = DHT.temperature;

  // Vcc voltage (useful to see remaining battery capacity)
  int vcc = readVcc();

  // convert everything to strings because the VirtualWire library can only send strings
  char msg[30];
  char str_temp[6];
  char str_hum[6];

  /* 4 is mininum width, 1 is precision; float value is copied onto str_temp*/
  dtostrf(temp, 3, 1, str_temp);
  dtostrf(humidity, 3, 1, str_hum);

  // throw everything in one big string
  sprintf(msg, "th,%d,%s,%s,%d,%d", id, str_temp, str_hum, vcc, counter);

#if defined(DEBUG)
  // flash a light to show when transmission occurs
  digitalWrite(led_pin, HIGH);
#endif

  // send the string
  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait until the whole message is gone

#if defined(DEBUG)
  digitalWrite(led_pin, LOW);
#endif

  // don't let the packet counter go over 0-99
  if (counter >= 99) {
    counter = 0;
  } else {
    counter++;
  }

#if defined(DEBUG)
  delay(1000);
#else
  // sleep randomly between 1...3 x 8 seconds.
  int randNumber = (int) random(1, 4);

  for (int i = 0; i < randNumber; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
#endif
}

