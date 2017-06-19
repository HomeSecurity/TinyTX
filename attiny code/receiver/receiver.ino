#define RF69_COMPAT 1
#include <JeeLib.h> // https://github.com/jcw/jeelib

#define SENSOR_ID 5215
#define TYPE 2001
#define rfm12bId 31
#define RETRY_PERIOD 100 // How soon to retry (in milliseconds) if ACK didn't come in
#define RETRY_LIMIT 20   // Maximum number of times to retry
#define ACK_TIME 100     // Number of milliseconds to wait for an ack
#define SW_PIN 7         // Aktor pin
#define LED_PIN 3         

typedef struct {
  int zero;
  int sensorId;
  int type;
  int voltage;
} Registration;

Registration registration;

void setup () {
  Serial.begin(9600);
  rf12_initialize(rfm12bId, RF12_868MHZ, 210); // Initialise the RFM12B

  Serial.println("Receiver started");
  Serial.println("Waiting for data");
  Serial.println("-----------------------------");

  registration.zero = 0;
  registration.sensorId = SENSOR_ID;
  registration.type = TYPE;
  registration.voltage = readVcc();

  rfwrite(); // Send registration via RF

  pinMode(SW_PIN, OUTPUT);
  digitalWrite(SW_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void enableLED(int duration){
  digitalWrite(LED_PIN, HIGH);
  delay(duration);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) { //sucessfull reading according to nathan
    byte len = rf12_len;
    if (len != 4) {
      Serial.print("Invalid message of length ");
      Serial.println((int) len);
      return;
    }

    Serial.print("successfully read a package ");

    //unsigned! (0-65536)
    unsigned int readings[len / 2];
    for (byte i = 0; i < len; i += 2) {
      byte byte1 = (256 + rf12_data[i]) % 256;
      byte byte2 = (256 + rf12_data[i + 1]) % 256;
      unsigned int result = 256 * byte2 + byte1;
      readings[i / 2] = result;
    }
    
    Serial.print(readings[0]);
    Serial.print(" ");
    Serial.println(readings[1]);

    if (readings[0] != SENSOR_ID) {
      Serial.print("Message targeted for sensorId ");
      Serial.print(readings[0]);
      Serial.print(" does not match ");
      Serial.print(SENSOR_ID);
      Serial.println(" ... skipping");
      return; //message not for this sensor; skip handling & ack !
    }

    //message targeted for this aktor
    
    enableLED(100);

    if (readings[1] == 1) {
      Serial.println("State set to LOW");
      digitalWrite(SW_PIN, LOW);
    } else {
      Serial.println("State set to HIGH");
      digitalWrite(SW_PIN, HIGH);
    }

    if (RF12_WANTS_ACK) {
      rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      Serial.println("ACK sent");
    }
  }
}

static void rfwrite() {
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    rf12_sleep(-1);
    while (!rf12_canSend()) {
      rf12_recvDone();
    }
    rf12_sendStart(RF12_HDR_ACK, &registration, sizeof registration);
    //rf12_sendWait(2);
    byte acked = waitForAck();
    rf12_sleep(0);
    if (acked) {
      return;
    }

    Sleepy::loseSomeTime(RETRY_PERIOD);
  }
}

// Wait a few milliseconds for proper ACK
static byte waitForAck() {
  MilliTimer ackTimer;
  while (!ackTimer.poll(ACK_TIME)) {
    if (rf12_recvDone() && rf12_crc == 0 && rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | rfm12bId))
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
