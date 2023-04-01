#include <Filters.h>
#include <SPI.h>
#include <EEPROM.h>
#include "RF24.h"

#define ACS_Pin1 A0
#define ACS_Pin2 A1
#define WaterSensorPin1 5
#define WaterSensorPin2 6
#define modeDebugPin 9
#define modeDryerPin1 8
#define modeDryerPin2 7
#define FlowSensorPin1 3
#define FlowSensorPin2 2
// #define ACS_LED1 4
// #define ACS_LED2 5
// #define water_LED1 6
// #define water_LED2 7
// #define flow_LED1 A4
// #define flow_LED2 A5

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
char rfa[30];
byte DeviceNum_1;
byte DeviceNum_2;
// ============================================== millis()&if()
unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long endPeriod1 = 65000;
unsigned long previousMillis_end1 = 0;
unsigned long endPeriod2 = 65000;
unsigned long endPeriod_dryer1 = 5000;
unsigned long endPeriod_dryer2 = 5000;
unsigned long previousMillis_end2 = 0;
int m1 = 0, m2 = 0;
String SerialData;
int dex, dex1, dexc, end;

bool mode_debug = false;
bool mode_dryer1 = false;
bool mode_dryer2 = false;

int cnt1 = 1;
int cnt2 = 1;
// ======================================= 비접촉수위센서
int WaterSensorData1 = 0;
int WaterSensorData2 = 0;

//======================================== 유량센서
volatile int flow_frequency1; // 유량센서 펄스 측정
volatile int flow_frequency2; // 유량센서 펄스 측정
unsigned int l_hour1;         // L/hour
unsigned int l_hour2;         // L/hour

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

  Serial.begin(115200);
  pinMode(ACS_Pin1, INPUT);
  pinMode(ACS_Pin2, INPUT);
  pinMode(WaterSensorPin1, INPUT);
  pinMode(WaterSensorPin2, INPUT);
  pinMode(modeDebugPin, INPUT_PULLUP);
  pinMode(modeDryerPin1, INPUT_PULLUP);
  pinMode(modeDryerPin2, INPUT_PULLUP);
  // pinMode(ACS_LED1, OUTPUT);
  // pinMode(ACS_LED2, OUTPUT);
  // pinMode(water_LED1,OUTPUT);
  // pinMode(water_LED2,OUTPUT);
  // pinMode(flow_LED1,OUTPUT);
  // pinMode(flow_LED2,OUTPUT);
  pinMode(FlowSensorPin1, INPUT);
  pinMode(FlowSensorPin2, INPUT);
  digitalWrite(FlowSensorPin1, HIGH); // 선택적 내부 풀업
  digitalWrite(FlowSensorPin2, HIGH);
  attachInterrupt(0, flow1, RISING); // 인터럽트(라이징)
  attachInterrupt(1, flow2, RISING);
  sei();         // 인터럽트 사용가능
  radio.begin(); // 아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);

  radio.stopListening();

  //================================================ EEPROM & debugMode
  for (int i = 0; i < 5; i++)
  {
    address[i] = char(EEPROM.read((i + 1)));
  }

  Serial.print("address : ");
  for (int i = 0; i < 5; i++)
  {
    Serial.print(address[i]);
  }
  address[5] = '\0';
  Serial.println();
  DeviceNum_1 = EEPROM.read(6);
  DeviceNum_2 = EEPROM.read(7);
  Serial.print("number1 : ");
  Serial.println(DeviceNum_1);
  Serial.print("number2 : ");
  Serial.println(DeviceNum_2);
  mode_debug = digitalRead(modeDebugPin);
  mode_dryer1 = digitalRead(modeDryerPin1);
  mode_dryer2 = digitalRead(modeDryerPin2);


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
      if (millis() - previousMillis >= printPeriod)
      {
        previousMillis = millis();

        WaterSensorData1 = digitalRead(WaterSensorPin1);
        WaterSensorData2 = digitalRead(WaterSensorPin2);

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
        Status_Judgment(Amps_TRMS1, WaterSensorData1, l_hour1, cnt1, m1, previousMillis_end1, endPeriod1, DeviceNum_1, 1);
      else
        Dryer_Status_Judgment(Amps_TRMS1, cnt1, m1, DeviceNum_1,previousMillis_end1,endPeriod_dryer1, 1);
      if (!mode_dryer2)
        Status_Judgment(Amps_TRMS2, WaterSensorData2, l_hour2, cnt2, m2, previousMillis_end2, endPeriod2, DeviceNum_2, 2);
      else
        Dryer_Status_Judgment(Amps_TRMS2, cnt2, m2, DeviceNum_2 ,previousMillis_end2,endPeriod_dryer2, 2);

      // 채널 1번
      /*if (Amps_TRMS1 > 0.5 || WaterSensorData1 || l_hour1 > 100)
        {
        if (cnt1 == 1)
        {
          // byte number = EEPROM.read(6);
          int DeviceNum_1_int = int(DeviceNum_1);
          String DeviceNum_1_str = String(DeviceNum_1_int);
          RadioData[0] = '0';
          DeviceNum_1_str.toCharArray(&RadioData[1], DeviceNum_1_str.length());
          radio.write(&RadioData, sizeof(RadioData));

          cnt1 = 0;
          Serial.println(&RadioData[1]);
        }
        m1 = 1;
        }
        else
        {
        if (previousMillis_end1 > millis())
          previousMillis_end1 = millis();
        if (m1)
        {
          previousMillis_end1 = millis();
          m1 = 0;
        }
        else if (cnt1)
          ;
        else if (millis() - previousMillis_end1 >= endPeriod1)
        {
          // byte number = EEPROM.read(7);
          // int numberi = int(number);
          int DeviceNum_1_int = int(DeviceNum_1);
          String DeviceNum_1_str = String(DeviceNum_1_int);
          RadioData[0] = '1';
          DeviceNum_1_str.toCharArray(&RadioData[1], DeviceNum_1_str.length());
          radio.write(&RadioData, sizeof(RadioData));

          cnt1 = 0;
          Serial.println(&RadioData[1]);
        }
        }

        // 채널 2번
        if (Amps_TRMS2 > 0.5 || WaterSensorData2 || l_hour2 > 100)
        {
        if (cnt2 == 1)
        {
          // byte number = EEPROM.read(4);
          // int numberi = int(number);
          // String numbers = String(numberi);
          int DeviceNum_2_int = int(DeviceNum_2);
          String DeviceNum_2_str = String(DeviceNum_2_int);
          RadioData[0] = '0';
          DeviceNum_2_str.toCharArray(&RadioData[1], DeviceNum_2_str.length());
          radio.write(&RadioData, sizeof(RadioData));

          cnt2 = 0;
          Serial.println(RadioData);
        }
        m2 = 1;
        }
        else
        {
        if (previousMillis_end2 > millis())
          previousMillis_end2 = millis();
        if (m2)
        {
          previousMillis_end2 = millis();
          m2 = 0;
        }
        else if (cnt2)
          ;
        else if (millis() - previousMillis_end2 >= endPeriod2)
        {
          // byte number = EEPROM.read(4);
          // int numberi = int(number);
          // String numbers = String(numberi);
          int DeviceNum_2_int = int(DeviceNum_2);
          String DeviceNum_2_str = String(DeviceNum_2_int);
          RadioData[0] = '1';
          DeviceNum_2_str.toCharArray(&RadioData[1], DeviceNum_2_str.length());
          radio.write(&RadioData, sizeof(RadioData));

          cnt2 = 0;
          Serial.println(RadioData);
        }*/
      //}
    }
  }
  //=======================================================================디버그 모드
  else
  {
    if (Serial.available())
    {
      SerialData = Serial.readStringUntil('\n');
      dex = SerialData.indexOf('+');
      dex1 = SerialData.indexOf('"');

      end = SerialData.length();
      String AT_Command = SerialData.substring(dex + 1, dex1);
      if (RFSET(AT_Command))
        ;
      else if (SENSDATA_START(AT_Command))
        ;
      else if (RFSEND(AT_Command))
        ;
      else if (UPDATE(AT_Command))
        ;
      else if (SETNUM(AT_Command))
        ;
      else if (NOWSTATE(AT_Command))
        ;
      else
        Serial.println("ERROR:Unknown command");
    }
  }
}

int RFSET(String command)
{
  // String b = "RFSET";
  // int result = command.compareTo(b);
  if (!(command.compareTo("RFSET")))
  {
    Serial.print("AT+OK ");
    String rf = SerialData.substring(dex1 + 1, end - 1);
    rf = rf + '0';
    // byte Byte1 = rf[0];
    // byte Byte2 = rf[1];
    // byte Byte3 = rf[2];
    // byte Byte4 = rf[3];
    // byte Byte5 = rf[4];
    // EEPROM.write(1, Byte1); // write(주소, 값)
    // EEPROM.write(2, Byte2); // write(주소, 값)
    // EEPROM.write(3, Byte3); // write(주소, 값)
    // EEPROM.write(4, Byte4); // write(주소, 값)
    // EEPROM.write(5, Byte5); // write(주소, 값)
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
  int cnt = 0;
  // String b = "SENSDATA_START";
  // int result = command.compareTo(b);
  if (!(command.compareTo("SENSDATA_START")))
  {
    Serial.print("AT+OK ");
    while (1)
    {
      if (Serial.available())
      {
        SerialData = Serial.readStringUntil('\n');
        dex = SerialData.indexOf('+');
        dex1 = SerialData.indexOf('"');
        end = SerialData.length();
        command = SerialData.substring(dex + 1, dex1);
        // String c = "SENSDATA_END";
        // if (!(command.compareTo(c))) {
        //   break;
        // }
        if (!(command.compareTo("SENSDATA_END")))
        {
          break;
        }
      }

      if (cnt >= 500)
      {
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

  else
  {
    return 0;
  }
}

int RFSEND(String command)
{
  // String b = "RFSEND";
  // int result = command.compareTo(b);
  if (!(command.compareTo("RFSEND")))
  {
    Serial.print("AT+OK ");
    String RF_data = SerialData.substring(dex1 + 1, end);
    char RadioData[30] = {0};
    RF_data.toCharArray(RadioData, RF_data.length());
    radio.write(&RadioData, sizeof(RadioData));
    Serial.println(RadioData);
    return 1;
  }
  else
  {
    return 0;
  }
}

int UPDATE(String command)
{
  // String b = "SEND";
  // int result = command.compareTo(b);
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
  // String b = "SETNUM";
  // int result = command.compareTo(b);
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

int NOWSTATE(String command)
{
  // String b = "NOWSTATE";
  // int result = command.compareTo(b);
  if (!(command.compareTo("NOWSTATE")))
  {
    Serial.print("AT+OK ");
    for (int i = 0; i < 5; i++)
    {
      address[i] = EEPROM.read((i + 1));
    }
    Serial.print("Address : ");
    for (int i = 0; i < 5; i++)
    {
      Serial.print(address[i]);
    }
    Serial.println();
    // byte number1 = EEPROM.read(6);
    // byte number2 = EEPROM.read(7);
    Serial.print("DeviceNumber1 : ");
    Serial.println(DeviceNum_1);
    Serial.print("DeviceNumber2 : ");
    Serial.println(DeviceNum_2);
    return 1;
  }
  return 0;
}

void software_Reset()
{
  asm volatile(" jmp 0");
}

//=================================================================Status_Judgment모드
void Dryer_Status_Judgment(float Amps_TRMS, int cnt, int m, int DeviceNum, unsigned long previousMillis_end, unsigned long endPeriod, int ChannelNum) {
  if (Amps_TRMS > 0.5)
  {
    if (cnt == 1)
    {
      // byte number = EEPROM.read(4);
      // int numberi = int(number);
      // String numbers = String(numberi);
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '0';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      if (ChannelNum == 1)  cnt1 = 0;
      if (ChannelNum == 2)  cnt2 = 0;
      Serial.println(RadioData);
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
      // byte number = EEPROM.read(4);
      // int numberi = int(number);
      // String numbers = String(numberi);
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '1';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));

      if (ChannelNum == 1)  cnt1 = 1;
      if (ChannelNum == 2)  cnt2 = 1;
      Serial.println(RadioData);
    }
  }

}
void Status_Judgment(float Amps_TRMS, int WaterSensorData, unsigned int l_hour, int cnt, int m, unsigned long previousMillis_end, unsigned long endPeriod, byte DeviceNum, int ChannelNum) {
  if (Amps_TRMS > 0.5 || WaterSensorData || l_hour > 100)
  {
    if (cnt == 1)
    {
      // byte number = EEPROM.read(4);
      // int numberi = int(number);
      // String numbers = String(numberi);
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '0';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));
      if (ChannelNum == 1)  cnt1 = 0;
      if (ChannelNum == 2)  cnt2 = 0;
      Serial.println(RadioData);
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
      // byte number = EEPROM.read(4);
      // int numberi = int(number);
      // String numbers = String(numberi);
      int DeviceNum_int = int(DeviceNum);
      String DeviceNum_str = String(DeviceNum_int);
      RadioData[0] = '1';
      DeviceNum_str += '0';
      DeviceNum_str.toCharArray(&RadioData[1], DeviceNum_str.length());
      radio.write(&RadioData, sizeof(RadioData));

      if (ChannelNum == 1)  cnt1 = 1;
      if (ChannelNum == 2)  cnt2 = 1;
      Serial.println(RadioData);
    }
  }
}
