#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 8); // RX, TX
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data

void setup()
{
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  mySerial.println("NOW REAADY");
}

void loop() {
  recvWithEndMarker();
}

//https://forum.arduino.cc/index.php?topic=288234.0
void recvWithEndMarker() {
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

