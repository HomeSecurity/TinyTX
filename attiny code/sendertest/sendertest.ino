#define RF69_COMPAT 1
#include <SoftwareSerial.h>
#include <JeeLib.h>



#define SENSOR_ID 111
#define TYPE 1000
#define rfm12bId 31
#define RETRY_PERIOD 100    // How soon to retry (in milliseconds) if ACK didn't come in
#define RETRY_LIMIT 20       // Maximum number of times to retry
#define ACK_TIME 100        // Number of milliseconds to wait for an ack
#define SW_PIN 10         // Reed switch connected from ground to this pin (D10/ATtiny pin 13)
SoftwareSerial mySerial(10, 8); // RX, TX

typedef struct {
  int zero;
  int sensorId;
  int type;
  int voltage;
} Registration;

Registration registration;

void setup() {
  Serial.begin(9600);
  rf12_initialize(rfm12bId, RF12_868MHZ, 210);
  //rf12_sleep(0);
  registration.zero = 0;
  registration.sensorId = SENSOR_ID;
  registration.type = TYPE;
  registration.voltage = 2900;
  mySerial.begin(9600);
  mySerial.println("NOW REAADY");
}

void loop() {
  //mySerial.println("loop");
  rfwrite();
  delay(5000);
}


static void rfwrite() {
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    //enableLED(7, 200, 1);//--------
    //rf12_sleep(-1);
    while (!rf12_canSend())
      rf12_recvDone();
    rf12_sendStart(RF12_HDR_ACK, &registration, sizeof registration);
    //rf12_sendWait(2);
    byte acked = waitForAck();
    //rf12_sleep(0);
    if (acked) {
      Serial.println("now acked");
      //enableLED(8, 700, 1);//-----------
      return;
    }

    Sleepy::loseSomeTime(RETRY_PERIOD);
  }
  Serial.println("no ack");
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


