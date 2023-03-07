#include <Filters.h>
#include <SPI.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1

bool m1 = false; // true : 켜짐, false : 꺼짐
bool m2 = false;

float ACS_Value1;
float ACS_Value2;
float testFrequency = 60;
float windowLength = 40.0 / testFrequency;

float arr1[300] = {0};
float arr2[300] = {0};

RF24 radio(8, 10); //CE, SS

uint8_t address1[6] = "00001"; //송신 주소
uint8_t address2[6] = "10002";

float intercept = 0;
float slope = 0.0455;

float   Amps_TRMS1;
float   Amps_TRMS2;

unsigned long printPeriod = 500;
unsigned   long previousMillis = 0;

unsigned long pre1 = 0, pre2 = 0;
bool toggle1 = true;
bool toggle2 = true;

char text;

int cnt1 = 0;
int cnt2 = 0;
float Asum1 = 0;
float Asum2 = 0;
float Aavg1 = 0;
float Aavg2 = 0;

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
    for (int i = 0; i < 300; i++) {
      ACS_Value1 = analogRead(ACS_Pin1);
      ACS_Value2 = analogRead(ACS_Pin2);

      inputStats1.input(ACS_Value1);
      inputStats2.input(ACS_Value2);

      if ((unsigned long)(millis() - previousMillis) >= printPeriod)   {
        previousMillis = millis();
        Amps_TRMS1 = intercept + slope * inputStats1.sigma();
        Amps_TRMS2 = intercept + slope * inputStats2.sigma();
        arr1[i] = Amps_TRMS1;
        arr2[i] = Amps_TRMS2;
        Asum1 += arr1[i];
        Asum2 += arr2[i];

        /*Serial.print( "\   Amps1: " );
          Serial.print( Amps_TRMS1 , 4);
          Serial.print( "\   Amps2: " );
          Serial.println( Amps_TRMS2 , 4);*/
        String aMp1 = String(Amps_TRMS1, 4);
        String aMp2 = String(Amps_TRMS2, 4);
        char amp1[32] = {0};
        char amp2[32] = {0};
        aMp1.toCharArray(amp1, aMp1.length());
        aMp2.toCharArray(amp2, aMp2.length());

        //Serial.print(aMp);
        //Serial.print("  ");
        //Serial.println(amp);
        radio.write(amp1, sizeof(amp1));
        radio.write(amp2, sizeof(amp2));

        /*if (Amps_TRMS1 > 1 ) { //A0이 켜짐
          pre1 = millis();
          if (toggle1) {
            text = '1';
            radio.write(&text, sizeof(text));
            toggle1 = false;
          }
          }
          else if (millis() - pre1 > 350000 && !toggle1) { //탈수 여부 확인
          text = '2';
          radio.write(&text, sizeof(text)); //A0이 꺼짐
          toggle1 = true;
          }
          if (Amps_TRMS2 > 1 && toggle2) { //A1이 켜짐
          pre2 = millis();
          if (toggle2) {
            text = '3';
            radio.write(&text, sizeof(text));
            toggle2 = false;
          }
          }
          else if (millis() - pre2 > 350000 && !toggle2) { //탈수 여부 확인
          text = '4';
          radio.write(&text, sizeof(text)); //A1이 꺼짐
          toggle2 = true;
          }*/
      }
      
        Aavg1 = Asum1 / 300;
        Aavg2 = Asum2 / 300;
        Asum1 = 0;
        Asum2 = 0;
        if(Aavg1 > 0.2 && !m1){
         m1 = true;
         text = '1';
         radio.write(&text, sizeof(text)); //A0이 켜짐
        }
        else if(m1){
         m1 = false;
         ///cnt1++;
         text = '2';
         radio.write(&text, sizeof(text)); //A0이 꺼짐
        }
        if(Aavg2 > 0.2 && !m2){
         m2 = true;
         text = '3';
         radio.write(&text, sizeof(text)); //A1이 켜짐
        }
        else if(m2){
         m2 = false;
         ///cnt2++;
         text = '4';
         radio.write(&text, sizeof(text)); //A1이 꺼짐
        }
        /*/if(cnt1 == 1){
         text = '1';
         radio.write(&text, sizeof(text)); //A0이 켜짐
        }
        if(cnt1 == 7 && m1){
         text = '2';
         radio.write(&text, sizeof(text)); //A0이 꺼짐
         cnt1 = 0;
        }
        if(cnt2 == 1){
         text = '3';
         radio.write(&text, sizeof(text)); //A1이 켜짐
        }
        if(cnt2 == 7 && m2){
         text = '4';
         radio.write(&text, sizeof(text)); //A0이 꺼짐
         cnt2 = 0;
        }/*/
    }
  }
}
  //}
