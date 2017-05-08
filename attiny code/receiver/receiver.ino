#define RF69_COMPAT 1
#include <JeeLib.h> // https://github.com/jcw/jeelib

void setup () {
  Serial.begin(9600);
  rf12_initialize(31, RF12_868MHZ, 210); // Initialise the RFM12B

  Serial.println("Receiver started");
  Serial.println("Waiting for data");
  Serial.println("-----------------------------");

  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
}

void loop() {
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) { //sucessfull reading according to nathan
    byte len = rf12_len;
    if (len > 10 || len < 6 || len % 2 != 0) { //TODO len > 8 (just debugging currently)
      Serial.print("Invalid message of length ");
      Serial.println((int) len);
      return;
    }

    Serial.println("successfully read a package");

    int readings[len / 2];
    for (byte i = 0; i < len; i += 2) {
      byte byte1 = rf12_data[i];
      byte byte2 = rf12_data[i + 1];
      int result = 256 * byte2 + byte1;
      readings[i / 2] = result;
    }

    if (readings[1] == 0) {
      digitalWrite(7, LOW);
    } else {
      digitalWrite(7, HIGH);
    }

    Serial.println("successfully parsed the package");

    if (RF12_WANTS_ACK) {
      rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      Serial.println("ack sent");
    }

    Serial.print("package: ");
    for (int i = 0; i < len / 2; i++) {
      Serial.print(readings[i]);
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("--------------------");
  }
}
