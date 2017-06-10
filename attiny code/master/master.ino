#define RF69_COMPAT 1
#include <SoftwareSerial.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib

#define RETRY_LIMIT 5       // Maximum number of times to retry
#define RETRY_PERIOD 80
#define ACK_TIME 100
SoftwareSerial mySerial(10, 8); // RX, TX
const byte numChars = 10;
char receivedChars[10]; // an array to store the received data
uint8_t KEY[] = "ABCDABCDABCDABCD"; // encryption key


typedef struct {
  int sensorId;
  int data;
} Payload;

Payload payload;

void setup() {
  rf12_initialize(31, RF12_868MHZ, 210); // Initialise the RFM12B, 31 = All Nodes
  //rf12_encrypt(KEY); //not working yet https://github.com/jcw/jeelib/issues/100

  mySerial.begin(9600);
  mySerial.println("NOW REAADY");
}

void loop() {
  readFromRaspberry();
  readFromRadio();
}

void readFromRadio() {
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
    //sucessfull reading according to nathan

    if (rf12_len > 8 || rf12_len < 6 || rf12_len % 2 != 0) {
      //for sure not our package / invalid
      return;
    }

    for (int i = 0; i < rf12_len; i++) {
      mySerial.print(rf12_data[i], BYTE);
    }

    if (RF12_WANTS_ACK) {
      rf12_sendStart(RF12_ACK_REPLY, 0, 0);
    }
  }
}

void readFromRaspberry() { //https://forum.arduino.cc/index.php?topic=288234.0
  static byte ndx = 0;
  char endMarker = 'a';
  char rc;

  while (mySerial.available() > 0) {
    rc = mySerial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;

      char receivedId[5];
      char receivedData[1];
      for (int i = 0; i < 5; i++) {
        receivedId[i] = receivedChars[i];
      }
      receivedData[0] = receivedChars[5];

      int sensorIdInt = atoi(receivedId);
      int dataInt = atoi(receivedData);

      char tmpEcho[5];
      char tmpEcho2[1];
      itoa(sensorIdInt, tmpEcho, 10);
      itoa(dataInt, tmpEcho2, 10);
      mySerial.print("echo back: ");
      mySerial.print(tmpEcho);
      mySerial.print(" ");
      mySerial.println(tmpEcho2);

      payload.sensorId = sensorIdInt;
      payload.data = dataInt;
      rfwrite(); // Send data to aktor
    }
  }
}

static void rfwrite() {
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    rf12_sleep(-1);
    while (!rf12_canSend()) {
      rf12_recvDone();
    }
    rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload);
    rf12_sendWait(2);
    byte acked = waitForAck();
    rf12_sleep(0);
    if (acked) {
      return;
    }

    Sleepy::loseSomeTime(RETRY_PERIOD);
  }
}

static byte waitForAck() {
  MilliTimer ackTimer;
  while (!ackTimer.poll(ACK_TIME)) {
    if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | 31))
      return 1;
  }
  return 0;
}


