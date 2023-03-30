#include <Filters.h>
#include <SPI.h>
#include <EEPROM.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1
#define WaterSensorPin1 5
#define WaterSensorPin2 6
#define modePin 9
#define FlowSensorPin1 3
#define FlowSensorPin2 2
//#define ACS_LED1 4
//#define ACS_LED2 5
//#define water_LED1 6
//#define water_LED2 7
//#define flow_LED1 A4
//#define flow_LED2 A5

//=============================================== 전류측정
float ACS_Value1;
float ACS_Value2;
float testFrequency = 60;
float windowLength = 40.0 / testFrequency;
float intercept = 0;
float slope = 0.0455;
float Amps_TRMS1;
float Amps_TRMS2;

// ============================================== RF통신
RF24 radio(8, 10); //CE, SS
uint8_t address[6] = {0}; //송신 주소
char text[30];

// ============================================== millis()&if()
unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long endPeriod1 = 65000;
unsigned long previousMillis_end1 = 0;
unsigned long endPeriod2 = 65000;
unsigned long previousMillis_end2 = 0;
int m1 = 0, m2 = 0;
String inString;
int dex, dex1, dexc, end;

bool mode = false;

int cnt1 = 1;
int cnt2 = 1;
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
  pinMode(modePin, INPUT_PULLUP);
  //pinMode(ACS_LED1, OUTPUT);
  //pinMode(ACS_LED2, OUTPUT);
  //pinMode(water_LED1,OUTPUT);
  //pinMode(water_LED2,OUTPUT);
  //pinMode(flow_LED1,OUTPUT);
  //pinMode(flow_LED2,OUTPUT);
  pinMode(FlowSensorPin1, INPUT);
  pinMode(FlowSensorPin2, INPUT);
  digitalWrite(FlowSensorPin1, HIGH); //선택적 내부 풀업
  digitalWrite(FlowSensorPin2, HIGH);
  attachInterrupt(0, flow1, RISING); //인터럽트(라이징)
  attachInterrupt(1, flow2, RISING);
  sei(); //인터럽트 사용가능
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(address); //송신하는 주소
  radio.stopListening();

  //================================================ EEPROM & debugMode
  byte HIByte = EEPROM.read(1); // read(주소)
  byte LOByte = EEPROM.read(2); // read(주소)
  int EPR = word(HIByte, LOByte);
  String EPRS = String(EPR);
  for (int i = 0; i < 5; i++)  address[i] = EPRS[i];
  address[5] = '\0';
  Serial.print("adress : ");
  Serial.println(EPR);
  byte number = EEPROM.read(3);
  int numberi = int(number);
  Serial.print("number : ");
  Serial.println(number);
  //mode = digitalRead(modePin);
}

void loop() {
  if (digitalRead(modePin)) {
    RunningStatistics inputStats1;
    RunningStatistics inputStats2;

    inputStats1.setWindowSecs( windowLength );
    inputStats2.setWindowSecs( windowLength );
    while (1) {
      ACS_Value1 = analogRead(ACS_Pin1);
      ACS_Value2 = analogRead(ACS_Pin2);

      inputStats1.input(ACS_Value1);
      inputStats2.input(ACS_Value2);

      Amps_TRMS1 = intercept + slope * inputStats1.sigma();
      Amps_TRMS2 = intercept + slope * inputStats2.sigma();

      if (previousMillis > millis()) previousMillis = millis();
      if (millis() - previousMillis >= printPeriod) {
        previousMillis = millis();

        WaterSensorData1 = digitalRead(WaterSensorPin1);
        WaterSensorData2 = digitalRead(WaterSensorPin2);

        l_hour1 = (flow_frequency1 * 60 / 7.5); //L/hour계산
        l_hour2 = (flow_frequency2 * 60 / 7.5);

        flow_frequency1 = 0; //변수 초기화
        flow_frequency2 = 0;
        Serial.print("ACS1 : ");
        Serial.println(Amps_TRMS1);
        Serial.print("Water1 : ");
        Serial.println(WaterSensorData1);
        Serial.print("L/h1 : ");
        Serial.println(l_hour1);
        Serial.print("ACS2 : ");
        Serial.println(Amps_TRMS2);
        Serial.print("Water2 : ");
        Serial.println(WaterSensorData2);
        Serial.print("L/h2 : ");
        Serial.println(l_hour2);
        Serial.print("Time : ");
        Serial.println(millis());
        Serial.println();

      }

      if (Amps_TRMS1 > 0.5 || WaterSensorData1  || l_hour1 > 100) {
        if (cnt1 == 1) {
          byte number = EEPROM.read(3);
          int numberi = int(number);
          String numbers = String(numberi);
          text[0] = '0';
          numbers.toCharArray(&text[1], numbers.length());
          radio.write(&text, sizeof(text));

          cnt1 = 0;
          Serial.println(text);
        }
        m1 = 1;
      }
      else {
        if (previousMillis_end1 > millis())  previousMillis_end1 = millis();
        if (m1) {
          previousMillis_end1 = millis();
          m1 = 0;
        }
        else if (cnt1);
        else if (millis() - previousMillis_end1 >= endPeriod1) {
          byte number = EEPROM.read(3);
          int numberi = int(number);
          String numbers = String(numberi);
          text[0] = '1';
          numbers.toCharArray(&text[1], numbers.length());
          radio.write(&text, sizeof(text));

          cnt1 = 0;
          Serial.println(text);
        }
      }

      if (Amps_TRMS2 > 0.5 || WaterSensorData2 || l_hour2 > 100) {
        if (cnt2 == 1) {
          byte number = EEPROM.read(4);
          int numberi = int(number);
          String numbers = String(numberi);
          text[0] = '0';
          numbers.toCharArray(&text[1], numbers.length());
          radio.write(&text, sizeof(text));

          cnt1 = 0;
          Serial.println(text);
        }
        m2 = 1;
      }
      else {
        if (previousMillis_end2 > millis())  previousMillis_end2 = millis();
        if (m2) {
          previousMillis_end2 = millis();
          m2 = 0;
        }
        else if (cnt2);
        else if (millis() - previousMillis_end2 >= endPeriod2 ) {
          byte number = EEPROM.read(4);
          int numberi = int(number);
          String numbers = String(numberi);
          text[0] = '1';
          numbers.toCharArray(&text[1], numbers.length());
          radio.write(&text, sizeof(text));

          cnt1 = 0;
          Serial.println(text);
        }
      }
    }
  }
  //=======================================================================디버그 모드
  else{
    if (Serial.available()) {
    inString = Serial.readStringUntil('\n');
    dex = inString.indexOf('+');
    dex1 = inString.indexOf('"');

    end = inString.length();
    String a = inString.substring(dex + 1, dex1);
    if (RFSET(a));
    else if (SENSDATA_START(a));
    else if (RFSEND(a));
    else if (SEND(a));
    else if(SETNUM(a));
    else if(NOWSTATE(a));
    else Serial.println("unknown commend");
  }
  }
}

int RFSET(String a) {
  String b = "RFSET";
  int result = a.compareTo(b);
  if (!result) {
    


    String rf = inString.substring(dex1 + 1, end - 1);
    int rfi = rf.toInt();
    byte hiByte = highByte(rfi);
    byte loByte = lowByte(rfi);
    EEPROM.write(1, hiByte); // write(주소, 값)
    EEPROM.write(2, loByte); // write(주소, 값)
    byte HiByte = EEPROM.read(1); // read(주소)
    byte LoByte = EEPROM.read(2); // read(주소)
    int epr = word(HiByte, LoByte);
    String eprs = String(epr);
    for (int i = 0; i < 5; i++){
      address[i] = eprs[i];
      Serial.print(address[i]);
    }
    software_Reset();
    Serial.println();
    return 1;
  }

  else {
    return 0;
  }
}

int SENSDATA_START(String a) {
  int cnt = 0;
  String b = "SENSDATA_START";
  int result = a.compareTo(b);
  if (!result) {
    while (1) {
      if (Serial.available()) {
        inString = Serial.readStringUntil('\n');
        dex = inString.indexOf('+');
        dex1 = inString.indexOf('"');
        end = inString.length();
        a = inString.substring(dex + 1, dex1);
        String c = "SENSDATA_END";
        if (!(a.compareTo(c))) {
          break;
        }
      }

      if (cnt >= 500) {
        cnt = 0;
        Serial.print("ACS1 : ");
        Serial.println(Amps_TRMS1);
        Serial.print("Water1 : ");
        Serial.println(WaterSensorData1);
        Serial.print("L/h1 : ");
        Serial.println(l_hour1);
        Serial.print("ACS2 : ");
        Serial.println(Amps_TRMS2);
        Serial.print("Water2 : ");
        Serial.println(WaterSensorData2);
        Serial.print("L/h2 : ");
        Serial.println(l_hour2);
        Serial.print("Time : ");
        Serial.println(millis());
        Serial.println();
      }
      cnt++;
      delay(1);
    }
    return 1;
  }

  else {
    return 0;
  }
}

int RFSEND(String a) {
  String b = "RFSEND";
  int result = a.compareTo(b);
  if (!result) {
    String rf = inString.substring(dex1 + 1, end);
    char text[30] = {0};
    rf.toCharArray(text, rf.length());
    radio.write(&text, sizeof(text));
    Serial.println(text);
    return 1;
  }
  else {
    return 0;
  }
}

int SEND(String a) {
  String b = "SEND";
  int result = a.compareTo(b);
  if (!result) {
    String rf = inString.substring(dex1 + 1, end - 1);
    char text[30] = {0};
    rf.toCharArray(text, rf.length());
    //대충 통신하는 내용
    radio.write(&text, sizeof(text));
    return 1;
  }
  else {
    return 0;
  }
}

int SETNUM(String a){
  String b = "SETNUM";
  int result = a.compareTo(b);
  if (!result) {
    dexc = inString.indexOf(',');
    String onOff = inString.substring(dex1 + 1, dexc - 1);
    String number = inString.substring(dexc + 1, end);
    char text[30] = {0};
    text[0] = onOff[0];
    number.toCharArray(&text[1], number.length());
    int numberi = number.toInt();
    if(text == '1')
      EEPROM.write(3, numberi);
    else if(text == '2')
      EEPROM.write(4, numberi);
    Serial.println(numberi);
    return 1;
  }
  return 0;
}

int NOWSTATE(String a){
  String b = "NOWSTATE";
  int result = a.compareTo(b);
  if (!result) {
    byte HIByte = EEPROM.read(1); // read(주소)
    byte LOByte = EEPROM.read(2); // read(주소)
    int EPR = word(HIByte, LOByte);
    byte number = EEPROM.read(3);
    int numberi = int(number);
    Serial.print("adress : ");
    Serial.println(EPR);
    Serial.print("number : ");
    Serial.println(number);
    return 1;
  }
  return 0;
}

void software_Reset(){
  asm volatile(" jmp 0");
}
