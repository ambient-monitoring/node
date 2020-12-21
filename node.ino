#include <LowPower.h>
#include <VirtualWire.h>
#include <dht.h>

dht DHT;

//#define DEBUG 1

#define SENSOR_ID CHANGE_ME

const int dht22_pin = PIN_PA1;
const int led_pin = PIN_PA0;
const int transmit_pin = PIN_PA7;

// packet counter
int counter = 0;

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0) ;
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
      // signal error
      for (int i = 0; i < 3; i++) {
        digitalWrite(led_pin, HIGH);
        delay(150);
        digitalWrite(led_pin, LOW);
        delay(150);
      }
#if defined(DEBUG)
      delay(2 * 1000);
#else
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
#endif
      return;
  }

  // sensor id, change it before flashing
  int id = SENSOR_ID;

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
  delay(5 * 1000);
#else
  // sleep randomly between 4...7 x 8 seconds.
  int randNumber = (int) random(4, 8);

  for (int i = 0; i < randNumber; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
#endif
}
