#define RF69_COMPAT 1
#include <JeeLib.h>
#include <PinChangeInterrupt.h>
#include <avr/sleep.h>

ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

#define SENSOR_ID 9573
#define TYPE 1000
#define rfm12bId 31
#define RETRY_PERIOD 100    // How soon to retry (in milliseconds) if ACK didn't come in
#define RETRY_LIMIT 20       // Maximum number of times to retry
#define ACK_TIME 100        // Number of milliseconds to wait for an ack
#define SW_PIN 7         // Reed switch connected from ground to this pin (D10/ATtiny pin 13)
#define LED_1 3
#define LED_2 9

typedef struct {
  int sensorId;
  int data;
  int voltage;
} Payload;

typedef struct {
  int zero;
  int sensorId;
  int type;
  int voltage;
} Registration;

Payload payload;
Registration registration;

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  Serial.begin(9600);
  Serial.println("Starting...");
  rf12_initialize(rfm12bId, RF12_868MHZ, 210);
  rf12_sleep(0);

  pinMode(SW_PIN, INPUT);
  digitalWrite(SW_PIN, HIGH);
  attachPcInterrupt(SW_PIN, wakeUp, CHANGE);

  PRR = bit(PRTIM1); // only keep timer 0 going

  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC);

  payload.sensorId = SENSOR_ID;
  registration.zero = 0;
  registration.sensorId = SENSOR_ID;
  registration.type = TYPE;
  registration.voltage = readVcc();

  rfwrite(true); // Send registration via RF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
}

void enableLED(boolean led1, boolean led2, int duration) {
  if (led1) {
    digitalWrite(LED_1, HIGH);
  }
  if (led2) {
    digitalWrite(LED_2, HIGH);
  }
  delay(duration);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
}

void wakeUp() {
}

void loop() {
  Serial.println("kden");
  int switchState = digitalRead(SW_PIN);

  if (switchState == HIGH) {
    payload.data = 1;
  } else {
    payload.data = 0;
  }

  payload.voltage = readVcc();
  rfwrite(false); // Send data via RF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
}

//todo "Object" übergeben anstatt boolean ^^
static void rfwrite(boolean init) {
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    enableLED(true, false, 100);
    rf12_sleep(-1);
    while (!rf12_canSend()) {
      rf12_recvDone();
    }
    if (init) {
      rf12_sendStart(RF12_HDR_ACK, &registration, sizeof registration);
    } else {
      rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload);
    }
    //rf12_sendWait(2);
    byte acked = waitForAck();
    rf12_sleep(0);
    if (acked) {
      enableLED(false, true, 500);
      return;
    }

    Sleepy::loseSomeTime(RETRY_PERIOD);
  }
}

// Wait a few milliseconds for proper ACK
static byte waitForAck() {
  MilliTimer ackTimer;
  while (!ackTimer.poll(ACK_TIME)) {
    if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | rfm12bId))
      return 1;
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
long readVcc() {
  bitClear(PRR, PRADC); ADCSRA |= bit(ADEN); // Enable the ADC
  long result;
  // Read 1.1V reference against Vcc
#if defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate Vcc in mV
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
  return result;
}
