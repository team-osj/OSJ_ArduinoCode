#include <SPI.h>
#include "RF24.h"

RF24 radio(7, 8); // CE : 7, CSN : 8

byte address[6] = "11111"; //5 byte


void setup() {
  Serial.begin(9600);
  radio.begin();
8u
  radio.setPALevel(RF24_PA_LOW);

  radio.openReadingPipe(1, address);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    char message[32] = "";
    radio.read(&message, sizeof(message));
    Serial.println(message);
  }
}
