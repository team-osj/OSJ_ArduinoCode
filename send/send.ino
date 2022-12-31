#include <Filters.h>
#include <SPI.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1

float ACS_Value1;
float ACS_Value2;
float testFrequency = 60;
float windowLength = 40.0 / testFrequency;

RF24 radio(8, 10); //CE, SS

uint8_t address1[6] = "00001"; //송신 주소
uint8_t address2[6] = "10002";

float intercept = 0;
float slope = 0.0455;

float   Amps_TRMS1;
float   Amps_TRMS2;

unsigned long printPeriod   = 1000;
unsigned   long previousMillis = 0;

unsigned long pre1 = 0, pre2 = 0;

void setup() {
  Serial.begin( 9600 );
  pinMode(ACS_Pin1, INPUT);
  pinMode(ACS_Pin2, INPUT);
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_LOW); 
  radio.openWritingPipe(address1); //송신하는 주소
  radio.stopListening();
}

void   loop() {
  RunningStatistics inputStats1;
  RunningStatistics inputStats2;
  
  inputStats1.setWindowSecs( windowLength );
  inputStats2.setWindowSecs( windowLength );

  while ( true ) {
    ACS_Value1 = analogRead(ACS_Pin1);
    ACS_Value2 = analogRead(ACS_Pin2);

    inputStats1.input(ACS_Value1);
    inputStats2.input(ACS_Value2);

    if ((unsigned long)(millis() - previousMillis) >= printPeriod)   {
      previousMillis = millis();  
      Amps_TRMS1 = intercept + slope * inputStats1.sigma();
      Amps_TRMS2 = intercept + slope * inputStats2.sigma();

      Serial.print( "\   Amps1: " );
      Serial.print( Amps_TRMS1 , 4);
      Serial.print( "\   Amps2: " );
      Serial.println( Amps_TRMS2 , 4);

      if(Amps_TRMS1 > 1){//A0이 켜짐
        char text[] = "1";
        radio.write(text,sizeof(text));
      }
      else if(millis() - pre1 > 1000){ //탈수 여부 확인
        char text[] = "2";
        radio.write(text,sizeof(text)); //A0이 꺼짐
      }
      else  {
        pre1 = millis();
      }
      if(Amps_TRMS2 > 1){//A1이 켜짐
        char text[] = "3";
        radio.write(text,sizeof(text));
      }
      else if(millis() - pre2 > 1000){ //탈수 여부 확인
        char text[] = "2";
        radio.write(text,sizeof(text)); //A1이 꺼짐
      }
      else  {
        pre2 = millis();
      }
      
    }
  }
}
