#include <SPI.h>
#include "RF24.h"

RF24 radio(4, 5); // CE : 4, CSN : 5

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
