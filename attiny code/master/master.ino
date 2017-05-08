#define RF69_COMPAT 1
#include <SoftwareSerial.h>
#include <JeeLib.h> // https://github.com/jcw/jeelib

SoftwareSerial mySerial(10, 8); // RX, TX
const byte numChars = 20;
char receivedChars[numChars]; // an array to store the received data
uint8_t KEY[] = "ABCDABCDABCDABCD"; // encryption key

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
      mySerial.print("echo back: ");
      mySerial.println(receivedChars);
    }
  }
}

