#include <SPI.h>
#include "RF24.h"

RF24 radio(4, 5); //CE = 4, SS = 5

uint8_t address1[6] = "00001";
uint8_t address2[6] = "10002";

int cnt = 1;

float i = 0.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); //PC와 아두이노간 통신라인
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);
  radio.openReadingPipe(0, address1); //파이프 주소 넘버 ,저장할 파이프 주소
  radio.openReadingPipe(1, address2);
  radio.startListening();
  Serial.println("ok");
}

void loop() {
  // put your main code here, to run repeatedly:
  byte pipe;
  if (radio.available(&pipe)) {
    //RF무선모듈쪽으로 뭔가 수신된값이 존재한다면~
    char text[30];
    radio.read(text, sizeof(text));
    //Serial.print(pipe);
    //Serial.print(",");
    Serial.println(text);
    //String Data(text);
    //int data;
    //data = Data.toInt();
    //Serial.println(data);
  }
  
}
