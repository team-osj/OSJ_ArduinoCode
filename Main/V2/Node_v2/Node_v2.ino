#include <Filters.h>
#include <SPI.h>
#include <EEPROM.h>
#include <time.h>
#include <stdlib.h>
#include "RF24.h"
#include <string.h>

#define ACS_Pin1 A4
#define ACS_Pin2 A5
#define DrainSensorPin1 A0
#define DrainSensorPin2 A1
#define modeDebugPin 9
#define Ch1_Mode A2
#define Ch2_Mode A3
#define FlowSensorPin1 3
#define FlowSensorPin2 2
#define dataPin 5
#define clockPin 7
#define latchPin 6
#define nrf_ldo 4

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

RF24 radio(8, 10);        // CE, SS
uint8_t address[6] = {0}; // 송신 주소
char RadioData[30];
char echoRadioData1[30];
char echoRadioData2[30];
char rfa[30];
byte DeviceNum_1;
byte DeviceNum_2;
const uint8_t num_channels = 126;
uint8_t values[num_channels];

// ============================================== millis()&if()

unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long endPeriod;
unsigned long previousMillis_end1 = 0;
unsigned long endPeriod_dryer;
unsigned long previousMillis_end2 = 0;
unsigned long echoPeriod1 = 200;
unsigned long echoPeriod2 = 200;
unsigned long echoMillis1 = 0;
unsigned long echoMillis2 = 0;
unsigned long heartBeatMillis = 0;

int m1 = 0, m2 = 0;
String SerialData;
int dex, dex1, dexc, end;

bool mode_debug = false;
bool mode_dryer1 = false;
bool mode_dryer2 = false;

int cnt1 = 1;
int cnt2 = 1;

int echoFlag1 = 0;
int echoFlag2 = 0;

// ======================================= 비접촉수위센서
int WaterSensorData1 = 0;
int WaterSensorData2 = 0;

//======================================== 유량센서

volatile int flow_frequency1; // 유량센서 펄스 측정
volatile int flow_frequency2; // 유량센서 펄스 측정
unsigned int l_hour1;         // L/hour
unsigned int l_hour2;         // L/hour

//---------------------------- LED_pin

#define LED_FLOW1 2
#define LED_FLOW2 5
#define LED_DRAIN1 1
#define LED_DRAIN2 4
#define LED_Current1 0
#define LED_Current2 3
byte data;

//======================================== 함수

void flow1() // 인터럽트 함수
{
  flow_frequency1++;
}

void flow2() // 인터럽트 함수
{
  flow_frequency2++;
}

void setup()
{

  srand((unsigned int)time(NULL));

  Serial.begin(115200);
  pinMode(ACS_Pin1, INPUT);
  pinMode(ACS_Pin2, INPUT);
  pinMode(DrainSensorPin1, INPUT);
  pinMode(DrainSensorPin2, INPUT);
  pinMode(modeDebugPin, INPUT_PULLUP);
  pinMode(Ch1_Mode, INPUT_PULLUP);
  pinMode(Ch2_Mode, INPUT_PULLUP);
  pinMode(FlowSensorPin1, INPUT);
  pinMode(FlowSensorPin2, INPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(nrf_ldo, OUTPUT);
  digitalWrite(FlowSensorPin1, HIGH); // 선택적 내부 풀업
  digitalWrite(FlowSensorPin2, HIGH);
  attachInterrupt(0, flow1, RISING); // 인터럽트(라이징)
  attachInterrupt(1, flow2, RISING);
  sei();         // 인터럽트 사용가능
  radio_Init();

  //================================================ EEPROM & debugMode

  SetDefaultVal();
  mode_debug = digitalRead(modeDebugPin);
  mode_dryer1 = digitalRead(Ch1_Mode);
  mode_dryer2 = digitalRead(Ch2_Mode);

  Serial.println(mode_debug);


  radio.openWritingPipe(address); // 송신하는 주소
}

void loop()
{
  if (mode_debug)
  {
    RunningStatistics inputStats1;
    RunningStatistics inputStats2;

    inputStats1.setWindowSecs(windowLength);
    inputStats2.setWindowSecs(windowLength);
    while (1)
    {
      ACS_Value1 = analogRead(ACS_Pin1);
      ACS_Value2 = analogRead(ACS_Pin2);

      inputStats1.input(ACS_Value1);
      inputStats2.input(ACS_Value2);

      Amps_TRMS1 = intercept + slope * inputStats1.sigma();
      Amps_TRMS2 = intercept + slope * inputStats2.sigma();

      if (previousMillis > millis())
        previousMillis = millis();
      if(millis() - heartBeatMillis >=300000){
        heartBeatMillis = millis();
        heartBeat(DeviceNum_1);
        heartBeat(DeviceNum_2);
        Serial.println("heartBeat");
      }
      if (millis() - previousMillis >= printPeriod)
      {
        if (Amps_TRMS1 > 0.5) {
          bitWrite(data, LED_Current1, 1); 
          Serial.println("Amps1");
        }
        else{
          bitWrite(data, LED_Current1, 0);
          Serial.println("AmpsNO1");
        }
        
        if (Amps_TRMS2 > 0.5) {
          bitWrite(data, LED_Current2, 1); 
          Serial.println("Amps2");
        }
        else{
          bitWrite(data, LED_Current2, 0); 
        }
        
        if (WaterSensorData1) {
          bitWrite(data, LED_DRAIN1, 1); 
        }
        else{
          bitWrite(data, LED_DRAIN1, 0);
        }
        
        if (WaterSensorData2) {
          bitWrite(data, LED_DRAIN2, 1); 
          Serial.println("Water2");
        }
        else{
          bitWrite(data, LED_DRAIN2, 0);
        }
        
        if (l_hour1 > 500) {
          
          bitWrite(data, LED_FLOW1, 1);
          Serial.println("hour1");
        }
        else{
          bitWrite(data, LED_FLOW1, 0);
        }
        
        if (l_hour2 > 500) {
          bitWrite(data, LED_FLOW2, 1);  
          Serial.println("hour2");
        }
        else{
          bitWrite(data, LED_FLOW2, 0);  
        }
        digitalWrite(latchPin, 0);
        shiftOut(dataPin, clockPin, LSBFIRST, data); 
        digitalWrite(latchPin, 1);
        Serial.println(data);
        previousMillis = millis();

        WaterSensorData1 = digitalRead(DrainSensorPin1);
        WaterSensorData2 = digitalRead(DrainSensorPin2);

        l_hour1 = (flow_frequency1 * 60 / 7.5); // L/hour계산
        l_hour2 = (flow_frequency2 * 60 / 7.5);

        flow_frequency1 = 0; // 변수 초기화
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

      if (mode_dryer1)
        Status_Judgment(Amps_TRMS1, WaterSensorData1, l_hour1, cnt1, m1, previousMillis_end1, DeviceNum_1, 1);
      else
        Dryer_Status_Judgment(Amps_TRMS1, cnt1, m1, DeviceNum_1, previousMillis_end1, 1);


      if (mode_dryer2)
        Status_Judgment(Amps_TRMS2, WaterSensorData2, l_hour2, cnt2, m2, previousMillis_end2, DeviceNum_2, 2);
      else
        Dryer_Status_Judgment(Amps_TRMS2, cnt2, m2, DeviceNum_2 , previousMillis_end2, 2);

      /*if (echoFlag1) {
        if (millis() - echoMillis1 >= echoPeriod1) {
          echoFlag1 = 0;
          radio.write(&echoFlag1, sizeof(echoFlag1));
        }
      }
      if (echoFlag2) {
        if (millis() - echoMillis1 >= echoPeriod2) {
          echoFlag2 = 0;
          radio.write(&echoFlag2, sizeof(echoFlag2));
        }
      }*/

    }
  }
  //=======================================================================디버그 모드
  else
  {
    //digitalWrite(nrf_ldo, HIGH);
    //Serial.println("haha");
    if (Serial.available())
    {
      Serial.println("hehe");
      SerialData = Serial.readStringUntil('\n');
      dex = SerialData.indexOf('+');
      dex1 = SerialData.indexOf('"');
      
      Serial.println(SerialData);

      end = SerialData.length();
      String AT_Command = SerialData.substring(dex + 1, dex1);
      Serial.println(AT_Command);
      if (RFSET(AT_Command))
        ;
      else if (SENSDATA_START(AT_Command))
        ;
      else if (NRFREAD_START(AT_Command))
         ;
      else if (RFSEND(AT_Command))
        ;
      else if (UPDATE(AT_Command))
        ;
      else if (SETNUM(AT_Command))
        ;
      else if (NOWSTATE(AT_Command))
        ;
      else if (SETDEL_WSH(AT_Command))
        ;
      else if (SETDEL_DRY(AT_Command))
        ;
      else
        Serial.println("ERROR:Unknown command");
    }
  }
}

int RFSET(String command)
{
  Serial.println("RF~");
  if (!(command.compareTo("RFSET")))
  {
    Serial.print("AT+OK ");
    String rf = SerialData.substring(dex1 + 1, end - 1);
    rf = rf + '0';
    for (int i = 0; i < 5; i++)
    {
      EEPROM.write(i + 1, rf[i]);
    }
    for (int i = 0; i < 5; i++)
    {
      address[i] = EEPROM.read((i + 1));
    }
    for (int i = 0; i < 5; i++)
    {
      Serial.print(address[i]);
    }
    Serial.println();
    delay(100);
    software_Reset();
    return 1;
  }

  else
  {
    return 0;
  }
}

int SENSDATA_START(String command)
{
  Serial.println("SENS~");
  int cnt = 0;
  int num = 0;
  if (!(command.compareTo("SENSDATA_START")))
  {
    Serial.println("AT+OK SENSDATA_START");
    Serial.println("Waiting for 30 sec...");
    Serial.println("CLOSE SERIAL PORT AND OPEN EXCEL LOG PROGRAM");
    delay(30000);
    Serial.println("CLEARDATA"); //처음에 데이터 리셋
    Serial.println("LABEL,No.,AMP1,Drain1,L/h1,AMP2,Drain2,L/h2"); //엑셀 첫행 데이터 이름 설정
    RunningStatistics inputStats1;
    RunningStatistics inputStats2;

    inputStats1.setWindowSecs(windowLength);
    inputStats2.setWindowSecs(windowLength);
    while (1)
    {
      if (Serial.available())
      {
        SerialData = Serial.readStringUntil('\n');
        dex = SerialData.indexOf('+');
        dex1 = SerialData.indexOf('"');
        end = SerialData.length();
        command = SerialData.substring(dex + 1, dex1);
        if (!(command.compareTo("SENSDATA_END")))
        {
          break;
        }
      }

      if (cnt >= 500)
      {
        ACS_Value1 = analogRead(ACS_Pin1);
        ACS_Value2 = analogRead(ACS_Pin2);

        inputStats1.input(ACS_Value1);
        inputStats2.input(ACS_Value2);

        Amps_TRMS1 = intercept + slope * inputStats1.sigma();
        Amps_TRMS2 = intercept + slope * inputStats2.sigma();

        WaterSensorData1 = digitalRead(DrainSensorPin1);
        WaterSensorData2 = digitalRead(DrainSensorPin2);

        l_hour1 = (flow_frequency1 * 60 / 7.5);
        l_hour2 = (flow_frequency2 * 60 / 7.5);

        flow_frequency1 = 0;
        flow_frequency2 = 0;
        num++;
        cnt = 0;
        Serial.print("DATA,");
        Serial.print(num);
        Serial.print(",");
        Serial.print(Amps_TRMS1);
        Serial.print(",");
        Serial.print(WaterSensorData1);
        Serial.print(",");
        Serial.print(l_hour1);
        Serial.print(",");
        Serial.print(Amps_TRMS2);
        Serial.print(",");
        Serial.print(WaterSensorData2);
        Serial.print(",");
        Serial.print(l_hour2);
        //Serial.print("Time : ");
        //Serial.println(millis());
        Serial.println();
      }
      cnt++;
      delay(1);
    }
    return 1;
  }

  else
  {
    return 0;
  }
}

int RFSEND(String command)
{
  
  Serial.println("RFSEND~");
  if (!(command.compareTo("RFSEND")))
  {
    radio.powerUp();
    Serial.print("AT+OK ");
    String RF_data = SerialData.substring(dex1 + 1, end);
    char RadioData[30] = {0};
    RF_data.toCharArray(RadioData, RF_data.length());
    radio.write(&RadioData, sizeof(RadioData));
    Serial.println(RadioData);
    radio.powerDown();
    return 1;
  }
  else
  {
    return 0;
  }
}

int UPDATE(String command)
{
  Serial.println("UP~");
  if (!(command.compareTo("UPDATE")))
  {
    Serial.print("AT+OK ");
    String RF_data = SerialData.substring(dex1 + 1, end - 1);
    char RadioData[30] = {0};
    RF_data.toCharArray(RadioData, RF_data.length());
    radio.write(&RadioData, sizeof(RadioData));
    return 1;
  }
  else
  {
    return 0;
  }
}

int SETNUM(String command)
{
  Serial.println("SETNUM~");
  if (!(command.compareTo("SETNUM")))
  {
    Serial.print("AT+OK ");
    dexc = SerialData.indexOf(',');
    String Channel = SerialData.substring(dex1 + 1, dexc);
    String Number = SerialData.substring(dexc + 1, end);
    int Number_int = Number.toInt();
    int Channel_int = Channel.toInt();
    if (Channel_int == 1)
    {
      EEPROM.write(6, Number_int);
      Serial.print("insert1 : ");
    }
    else if (Channel_int == 2)
    {
      EEPROM.write(7, Number_int);
      Serial.print("insert2 : ");
    }
    Serial.println(Number_int);
    return 1;
  }
  return 0;
}

int SETDEL_WSH(String command)
{
  Serial.println("SETWSH~");
  if (!(command.compareTo("SETDEL_WSH")))
  {
    Serial.print("AT+OK ");
    String Time = SerialData.substring(dex1 + 1, end);
    int Time_int = Time.toInt();
    if (Time_int < 255) {
      EEPROM.write(8, Time_int);
      Serial.print("TimeSet : ");
      Serial.println(Time_int);
    }
    else {
      Serial.println("OverFlow");
    }
    return 1;
  }
  return 0;
}

int SETDEL_DRY(String command)
{
  Serial.println("SETDRY~");
  if (!(command.compareTo("SETDEL_DRY")))
  {
    Serial.print("AT+OK ");
    String Time = SerialData.substring(dex1 + 1, end);
    int Time_int = Time.toInt();
    if (Time_int < 255) {
      EEPROM.write(9, Time_int);
      Serial.print("TimeSet : ");
      Serial.println(Time_int);
    }
    else {
      Serial.println("OverFlow");
    }
    return 1;
  }
  return 0;
}

int NOWSTATE(String command)
{
  Serial.println("NOW~");
  if (!(command.compareTo("NOWSTATE")))
  {
    Serial.print("AT+OK ");
    SetDefaultVal();
    return 1;
  }
  return 0;
}

int NRFREAD_START(String command) {
  int cnt = 0;
  int num = 0;
  Serial.println("NRF~");
  if (!(command.compareTo("NRFREAD_START")))
  {
    int i = 0;
    while (i < num_channels) {
      Serial.print(i >> 4, HEX);
      ++i;
    }
    Serial.println();
    i = 0;
    while (i < num_channels) {
      Serial.print(i & 0xf, HEX);
      ++i;
    }
    Serial.println();
    const int num_reps = 100;
    bool constCarrierMode = 0;
    while (1)
    {
      if (Serial.available())
      {
        SerialData = Serial.readStringUntil('\n');
        dex = SerialData.indexOf('+');
        dex1 = SerialData.indexOf('"');
        end = SerialData.length();
        command = SerialData.substring(dex + 1, dex1);
        if (!(command.compareTo("NRFREAD_END")))
        {
          break;
        }
      }

      memset(values, 0, sizeof(values));

    // Scan all channels num_reps times
    int rep_counter = num_reps;
    while (rep_counter--) {
      int i = num_channels;
      while (i--) {
        // Select this channel
        radio.setChannel(i);

        // Listen for a little
        radio.startListening();
        delayMicroseconds(128);
        radio.stopListening();

        // Did we get a carrier?
        if (radio.testCarrier()) {
          ++values[i];
        }
      }
    }


    // Print out channel measurements, clamped to a single hex digit
    int i = 0;
    while (i < num_channels) {
      if (values[i])
        Serial.print(min(0xf, values[i]), HEX);
      else
        Serial.print(F("-"));

      ++i;
    }
    Serial.println();
    }
    return 1;
  }
  return 0;
}

void radio_Init(){
  digitalWrite(nrf_ldo, HIGH);
  radio.begin(); // 아두이노-RF모듈간 통신라인
  radio.setChannel(103); // 간섭 방지용 채널 변경
  radio.setPALevel(RF24_PA_MAX);

  radio.stopListening();
}

void software_Reset()
{
  asm volatile(" jmp 0");
}

//=================================================================Status_Judgment모드
void Dryer_Status_Judgment(float Amps_TRMS, int cnt, int m, int DeviceNum, unsigned long previousMillis_end, int ChannelNum) {
  if (Amps_TRMS > 0.3)
  {
    if (cnt == 1)
    {
      radio.powerUp();
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '0';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      if (ChannelNum == 1)  cnt1 = 0;
      if (ChannelNum == 2)  cnt2 = 0;
      Serial.println(RadioData);
      radio.powerDown();
    }
    if (ChannelNum == 1)  m1 = 1;
    if (ChannelNum == 2)  m2 = 1;
  }
  else
  {
    if (previousMillis_end > millis())
      previousMillis_end = millis();
    if (m)
    {
      if (ChannelNum == 1)
        previousMillis_end1 = millis();
      if (ChannelNum == 2)
        previousMillis_end2 = millis();
      if (ChannelNum == 1)  m1 = 0;
      if (ChannelNum == 2)  m2 = 0;
    }
    else if (cnt)
      ;
    else if (millis() - previousMillis_end >= endPeriod_dryer)
    {
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      radio.powerUp();
      RadioData[0] = '1';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      radio.powerDown();
      if (ChannelNum == 1) {
        cnt1 = 1;
        echoFlag1 = 1;
        echoPeriod1 = rand() % 300 + 200;
        echoMillis1 = millis();
        strcpy(echoRadioData1, RadioData);
      }
      if (ChannelNum == 2) {
        cnt2 = 1;
        echoFlag2 = 1;
        echoPeriod2 = rand() % 300 + 200;
        echoMillis2 = millis();
        strcpy(echoRadioData2, RadioData);
      }
      Serial.println(RadioData);
    }
  }
}
void Status_Judgment(float Amps_TRMS, int WaterSensorData, unsigned int l_hour, int cnt, int m, unsigned long previousMillis_end, byte DeviceNum, int ChannelNum) {
  if (Amps_TRMS > 0.5 || WaterSensorData || l_hour > 50)
  {
    if (cnt == 1)
    {
      radio.powerUp();
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '0';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      if (ChannelNum == 1)  cnt1 = 0;
      if (ChannelNum == 2)  cnt2 = 0;
      Serial.println(RadioData);
      radio.powerDown();
    }
    if (ChannelNum == 1)  m1 = 1;
    if (ChannelNum == 2)  m2 = 1;
  }
  else
  {
    if (previousMillis_end > millis())
      previousMillis_end = millis();
    if (m)
    {
      if (ChannelNum == 1)
        previousMillis_end1 = millis();
      if (ChannelNum == 2)
        previousMillis_end2 = millis();
      if (ChannelNum == 1)  m1 = 0;
      if (ChannelNum == 2)  m2 = 0;
    }
    else if (cnt)
      ;
    else if (millis() - previousMillis_end >= endPeriod)
    {
      radio.powerUp();
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '1';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      radio.powerDown();
      if (ChannelNum == 1) {
        cnt1 = 1;
        echoFlag1 = 1;
        echoPeriod1 = rand() % 300 + 200;
        echoMillis1 = millis();
        strcpy(echoRadioData1, RadioData);
      }
      if (ChannelNum == 2) {
        cnt2 = 1;
        echoFlag2 = 1;
        echoPeriod2 = rand() % 300 + 200;
        echoMillis2 = millis();
        strcpy(echoRadioData2, RadioData);
      }
      Serial.println(RadioData);
    }
  }
}

void heartBeat(int DeviceNum){
  Serial.println("live!");
  radio.powerUp();
  int DeviceNum_int = int(DeviceNum);
  String DeviceNum_str = String(DeviceNum_int);
  RadioData[0] = '2';
  DeviceNum_str += '0';
  DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
  radio.write(&RadioData, sizeof(RadioData));
  radio.powerDown();
}

void SetDefaultVal() {

  for (int i = 0; i < 5; i++)
  {
    address[i] = char(EEPROM.read((i + 1)));
    if (address[i] < '0') address[i] = '0';
  }

  Serial.print("address : ");
  for (int i = 0; i < 5; i++)
  {
    Serial.print(address[i]);
  }
  address[5] = '\0';
  Serial.println();
  DeviceNum_1 = EEPROM.read(6);
  if (DeviceNum_1 < 0) DeviceNum_1 = 1;
  DeviceNum_2 = EEPROM.read(7);
  if (DeviceNum_2 < 0) DeviceNum_2 = 2;
  Serial.print("number1 : ");
  Serial.println(DeviceNum_1);
  Serial.print("number2 : ");
  Serial.println(DeviceNum_2);
  Serial.println(EEPROM.read(8));
  endPeriod = EEPROM.read(8); //5000
  endPeriod *=  10000;
  if (endPeriod > 2540000 && endPeriod <= 0) endPeriod = 100000;
  endPeriod_dryer = EEPROM.read(9);//2000
  endPeriod_dryer *= 1000;
  if (endPeriod_dryer > 254000 && endPeriod_dryer <= 0) endPeriod_dryer = 10000;
  Serial.print("Time_WSH : ");
  Serial.println(endPeriod);
  Serial.print("Time_DRY : ");
  Serial.println(endPeriod_dryer);
}
