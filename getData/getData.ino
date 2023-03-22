#include <SPI.h>
#include "RF24.h"

#define valueCnt 6 //엑셀에 찍을 값 갯수

RF24 radio(4, 5); //CE = 4, SS = 5

uint8_t address1[6] = "00001";
uint8_t address2[6] = "10002";

int cnt = 0;

float i = 0.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); //PC와 아두이노간 통신라인
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);
  radio.openReadingPipe(0, address1); //파이프 주소 넘버 ,저장할 파이프 주소
  radio.openReadingPipe(1, address2);
  radio.startListening();
  Serial.println("CLEARDATA"); //처음에 데이터 리셋
  Serial.println("LABEL,No.,AMP1"); //엑셀 첫행 데이터 이름 설정
  Serial.println("LABEL,No.,AMP2"); //엑셀 첫행 데이터 이름 설정
}

void loop() {
  // put your main code here, to run repeatedly:
  byte pipe;
  if (radio.available(&pipe)) {
    //RF무선모듈쪽으로 뭔가 수신된값이 존재한다면~
    char text[30];
    radio.read(text, sizeof(text));
    if (cnt % valueCnt == 0) {
      Serial.print("DATA,"); //데이터 행에 데이터를 받겠다는 말
      Serial.print(i); // No. 데이터를 출력
      i += 0.5;
      Serial.print(","); // ,로 데이터를 구분하고 엑셀에는 셀을 구분
      Serial.print(text); // Distance 데이터 출력
    }
    else {
      Serial.print(","); // ,로 데이터를 구분하고 엑셀에는 셀을 구분
      if((cnt % valueCnt) == (valueCnt - 1))
      Serial.println(text); // Distance 데이터 출력
      else
      Serial.print(text);
    }
    cnt++;
  }
}
