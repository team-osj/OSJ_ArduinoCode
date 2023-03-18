#include <Filters.h>
#include <SPI.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1
#define WaterSensorPin1 2
#define WaterSensorPin2 3
//#define FlowSensorPin1 = 2
//#define FlowSensorPin2 = 3
//======================================== 전류측정
float ACS_Value1;
float ACS_Value2;
float testFrequency = 60;
float windowLength = 40.0 / testFrequency;

float   Amps_TRMS1;
float   Amps_TRMS2;

float intercept = 0;
float slope = 0.0455;
//======================================== RF24통신
RF24 radio(8, 10); //CE, SS

uint8_t address1[6] = "00001"; //송신 주소
uint8_t address2[6] = "10002";

char text;
// ======================================= millis()함수&if등등
unsigned long printPeriod = 500;
unsigned long previousMillis = 0;

// ======================================= 비접촉수위센서
int WaterSensorData1 = 0;
int WaterSensorData2 = 0;

//======================================== 유량센서
volatile int flow_frequency1; // 유량센서 펄스 측정
volatile int flow_frequency2; // 유량센서 펄스 측정
unsigned int l_hour1; // L/hour
unsigned int l_hour2; // L/hour

//======================================== 함수
void flow1() // 인터럽트 함수
{
   flow_frequency1++;
}

void flow2() // 인터럽트 함수
{
   flow_frequency2++;
}

void setup() {
  Serial.begin( 115200 );
  pinMode(ACS_Pin1, INPUT);
  pinMode(ACS_Pin2, INPUT);
  pinMode(WaterSensorPin1, INPUT);
  pinMode(WaterSensorPin2, INPUT);
  //pinMode(FlowSensorPin1, INPUT);
  //pinMode(FlowSensorPin2, INPUT);
  //digitalWrite(FlowSensorPin1, HIGH); //선택적 내부 풀업
  //digitalWrite(FlowSensorPin2, HIGH); 
  //attachInterrupt(0, flow1, RISING); //인터럽트(라이징)
  //attachInterrupt(1, flow2, RISING);
  //sei(); //인터럽트 사용가능 
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

      //l_hour1 = (flow_frequency1 * 60 / 7.5); //L/hour계산
      //l_hour2 = (flow_frequency2 * 60 / 7.5);

      //flow_frequency1 = 0; //변수 초기화
      //flow_frequency2 = 0;
      
      if(Amps_TRMS1 > 0.15){ //전류 ON/OFF
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
      if(WaterSensorData1){ //비접촉수위센서 ON/OFF
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
      /*if(l_hour1 > 50){ //유량센서 ON/OFF
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));
      if(l_hour2 > 50){
        text = '1';
      }
      else{
        text = '0';
      }
      radio.write(&text, sizeof(text));*/
    }
  }
}
