#include <SPI.h>
#include "RF24.h"
#define ACS712 A1

char toggle = '0';
int readValue;

RF24 radio(8, 10); // CE : 8, CSN : 10

byte address[6] = "11111"; //5 byte

void setup() {
  pinMode(ACS712, INPUT);
  Serial.begin(9600);
  radio.begin();

  radio.setPALevel(RF24_PA_LOW);

  radio.openWritingPipe(address);
  radio.stopListening();

}

void loop() {
  readValue = analogRead(ACS712);
  if (readValue > 800 && toggle == 0) {
    Serial.println(1);
    toggle = '1';
    radio.write(&toggle, sizeof(toggle));
  }
  if (readValue < 300 && toggle == 1) {
    if (is_die()) {
      Serial.println(0);
      toggle = '0';
      radio.write(&toggle, sizeof(toggle));
    }
  }
}

int is_die() {
  delay(10000);
  if (analogRead(A0) < 300)  return 1;
  return 0;
}
