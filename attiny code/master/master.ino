#define RF69_COMPAT 1
#include <SoftwareSerial.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib

#define rfm12bId 31                    // 31 = listen for all nodes on same network group
#define RETRY_PERIOD 10                // How soon to retry (in milliseconds) if ACK didn't come in
#define RETRY_LIMIT 5                  // Maximum number of times to retry
#define ACK_TIME 50                    // Number of milliseconds to wait for an ack
SoftwareSerial mySerial(10, 8);        // RX, TX
const int numChars = 10;               // buffer size
char receivedChars[numChars];          // an array to store the received data
uint8_t KEY[] = "ABCDABCDABCDABCD";    // encryption key

typedef struct {
  int sensorId;
  int data;
} AktorPayload;

AktorPayload payload;

void setup() {
  rf12_initialize(rfm12bId, RF12_868MHZ, 210);
  //rf12_encrypt(KEY); //not working yet https://github.com/jcw/jeelib/issues/100

  mySerial.begin(9600);
  //mySerial.println("NOW REAADY");
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

//unfortunately it is only possible to read chars (or bytes)? -> awkward convertion to int
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

      payload.sensorId = sensorIdInt;
      payload.data = dataInt;
      
      rfwrite(); // Send data to aktor
    }
  }
}


static void rfwrite() {
  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    while (!rf12_canSend())
      rf12_recvDone();
    rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload);
    byte acked = waitForAck();
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


