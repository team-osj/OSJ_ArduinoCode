#include <Filters.h>
#include <SPI.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1
#define WaterSensorPin1 2
#define WaterSensorPin2 3

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
int WaterSensorData1 = 0;
int WaterSensorData2 = 0;

int cnt1 = 0;
int cnt2 = 0;
float Asum1 = 0;
float Asum2 = 0;
float Aavg1 = 0;
float Aavg2 = 0;

char cnt = '0';



void setup() {
  Serial.begin( 115200 );
  pinMode(ACS_Pin1, INPUT);
  pinMode(ACS_Pin2, INPUT);
  pinMode(WaterSensorPin1, INPUT);
  pinMode(WaterSensorPin2, INPUT);
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(address1); //송신하는 주소
  radio.stopListening();
}

void   loop() {
  RunningStatistics inputStats1;
  RunningStatistics inputStats2;

  inputStats1.setWindowSecs( windowLength );
  inputStats2.setWindowSecs( windowLength );
  while ( true ) {
    //for (int i = 0; i < 300; i++) {
    ACS_Value1 = analogRead(ACS_Pin1);
    ACS_Value2 = analogRead(ACS_Pin2);
    
    inputStats1.input(ACS_Value1);
    inputStats2.input(ACS_Value2);

    Amps_TRMS1 = intercept + slope * inputStats1.sigma();
    Amps_TRMS2 = intercept + slope * inputStats2.sigma();

    //Asum1 += Amps_TRMS1;
    //arr1[i] = Amps_TRMS1;
    //Asum1 += arr1[i];
    //}
    if (millis() - previousMillis >= printPeriod) {
      previousMillis = millis();
      //Aavg1 = Asum1 / 300;
      //Asum1 = 0;
      //String  AData = String(Amps_TRMS1);
      
      WaterSensorData1 = digitalRead(WaterSensorPin1);
      WaterSensorData2 = digitalRead(WaterSensorPin2);
      
      if(Amps_TRMS1 > 0.15){
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));
      if(Amps_TRMS2 > 0.15){
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));
      if(WaterSensorData1){
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));
      if(WaterSensorData2){
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));
      //Serial.println(AData);
      //Serial.println(WaterSensorData);
      
    }
  }
}
