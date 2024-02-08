#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "EmonLib.h"
#include <Preferences.h>
#include <nvs_flash.h>


#define PIN_STATUS 17
#define PIN_CH1_LED 18
#define PIN_CH2_LED 19
#define PIN_CH1_MODE 33
#define PIN_CH2_MODE 32
#define PIN_DEBUG 14
#define PIN_CT1 35
#define PIN_CT2 34
#define PIN_FLOW1 27
#define PIN_FLOW2 26
#define PIN_DRAIN1 23
#define PIN_DRAIN2 25

EnergyMonitor ct1;
EnergyMonitor ct2;
Preferences preferences;

// ============================================== millis()&if()
unsigned int endPeriod;
unsigned int endPeriod_dryer;

unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long previousMillis_end1 = 0;
unsigned long previousMillis_end2 = 0;
unsigned long echoPeriod1 = 200;
unsigned long echoPeriod2 = 200;
unsigned long echoMillis1 = 0;
unsigned long echoMillis2 = 0;
unsigned long heartBeatMillis = 0;

int m1 = 0, m2 = 0;

bool mode_debug = false;
bool CH1_Mode = false;
bool CH2_Mode = false;

int cnt1 = 1;
int cnt2 = 1;

int echoFlag1 = 0;
int echoFlag2 = 0;

bool CH1_Live = false;
bool CH2_Live = false;

String Device_Name = "OSJ_";
String ap_ssid;
String ap_passwd;
String serial_no;
String auth_id;
String auth_passwd;
String CH1_DeviceNo;
String CH2_DeviceNo;

// ======================================= 전류
float Amps_TRMS1;
float Amps_TRMS2;

// ======================================= 배수
int WaterSensorData1 = 0;
int WaterSensorData2 = 0;

//======================================== 유량

volatile int flow_frequency1; // 유량센서 펄스 측정
volatile int flow_frequency2; // 유량센서 펄스 측정
unsigned int l_hour1;         // L/hour
unsigned int l_hour2;         // L/hour

//======================================== 함수

void IRAM_ATTR flow1() // CH1 유량센서 인터업트
{
  flow_frequency1++;
}

void IRAM_ATTR flow2() // CH2 유량센서 인터업트
{
  flow_frequency2++;
}

void putString(const char *key, String value)
{
    Serial.print(key);
    Serial.print(" = ");
    Serial.println(value);
    preferences.putString(key, value);
}

void setup()
{
  preferences.begin("config", false);
  Serial.begin(115200);
  pinMode(PIN_STATUS, OUTPUT);         // STATUS
  pinMode(PIN_CH1_LED, OUTPUT);        // CH1
  pinMode(PIN_CH2_LED, OUTPUT);        // CH2
  pinMode(PIN_CH1_MODE, INPUT_PULLUP); // CH1 Mode
  pinMode(PIN_CH2_MODE, INPUT_PULLUP); // CH2 Mode
  pinMode(PIN_DEBUG, INPUT);           // Debug
  pinMode(PIN_CT1, INPUT);             // CT1
  pinMode(PIN_CT2, INPUT);             // CT2
  pinMode(PIN_FLOW1, INPUT);           // FLOW1
  pinMode(PIN_FLOW2, INPUT);           // FLOW2
  pinMode(PIN_DRAIN1, INPUT);          // DRAIN1
  pinMode(PIN_DRAIN2, INPUT);          // DRAIN2
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW1), flow1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW2), flow2, FALLING);

  //================================================ EEPROM & debugMode

  SetDefaultVal();
  mode_debug = digitalRead(PIN_DEBUG);
  CH1_Mode = digitalRead(PIN_CH1_MODE);
  CH2_Mode = digitalRead(PIN_CH2_MODE);
  Serial.print("DEBUG : ");
  Serial.println(mode_debug);
  Serial.print("CH1 : ");
  Serial.println(CH1_Mode);
  Serial.print("CH2 : ");
  Serial.println(CH2_Mode);
  ct1.current(PIN_CT1, 30.7);
  ct2.current(PIN_CT2, 30.7);
  for(int i=0;i<30;i++)
  {
    ct1.calcIrms(1480);
    ct2.calcIrms(1480);
  }
}

void loop()
{
  if (mode_debug)
  {
    while (1)
    {

      Amps_TRMS1 = ct1.calcIrms(1480);
      Amps_TRMS2 = ct2.calcIrms(1480);

      if (previousMillis > millis())
        previousMillis = millis();
      if (millis() - previousMillis >= printPeriod)
      {
        previousMillis = millis();

        WaterSensorData1 = digitalRead(PIN_DRAIN1);
        WaterSensorData2 = digitalRead(PIN_DRAIN2);

        l_hour1 = (flow_frequency1 * 60 / 7.5); // L/hour계산
        l_hour2 = (flow_frequency2 * 60 / 7.5);

        flow_frequency1 = 0; // 변수 초기화
        flow_frequency2 = 0;
        Serial.print("CT1 : ");
        Serial.println(Amps_TRMS1);
        Serial.print("Water1 : ");
        Serial.println(WaterSensorData1);
        Serial.print("L/h1 : ");
        Serial.println(l_hour1);
        Serial.print("CT2 : ");
        Serial.println(Amps_TRMS2);
        Serial.print("Water2 : ");
        Serial.println(WaterSensorData2);
        Serial.print("L/h2 : ");
        Serial.println(l_hour2);
        Serial.print("Time : ");
        Serial.println(millis());
        Serial.println();
      }

      if (CH1_Mode)
      {
        Status_Judgment(Amps_TRMS1, WaterSensorData1, l_hour1, cnt1, m1, previousMillis_end1, CH1_DeviceNo, 1);
      }
      else
      {
        Dryer_Status_Judgment(Amps_TRMS1, cnt1, m1, CH1_DeviceNo, previousMillis_end1, 1);
      }

      if (CH2_Mode)
      {
        Status_Judgment(Amps_TRMS2, WaterSensorData2, l_hour2, cnt2, m2, previousMillis_end2, CH2_DeviceNo, 2);
      }
      else
      {
        Dryer_Status_Judgment(Amps_TRMS2, cnt2, m2, CH2_DeviceNo, previousMillis_end2, 2);
      }
    }
  }
  //=======================================================================디버그 모드
  else
  {
    if (Serial.available())
    {
      int dex, dex1, dexc, end;
      String SerialData = Serial.readStringUntil('\n');
      dex = SerialData.indexOf('+');
      dex1 = SerialData.indexOf('"');
      end = SerialData.length();
      String AT_Command = SerialData.substring(dex + 1, dex1);
      if (!(AT_Command.compareTo("SENSDATA_START")))
      {
        Serial.println("AT+OK SENSDATA_START");
      }
      else if (!(AT_Command.compareTo("SOCKET_SEND")))
      {
      }
      else if (!(AT_Command.compareTo("UPDATE")))
      {
        Serial.println("AT+OK UPDATE");
      }
      else if (!(AT_Command.compareTo("SET_CH1")))
      {
        Serial.println("AT+OK SET_CH1");
        SETNUM(SerialData, dex1, end, 1);
      }
      else if (!(AT_Command.compareTo("SET_CH2")))
      {
        Serial.println("AT+OK SET_CH2");
        SETNUM(SerialData, dex1, end, 2);
      }
      else if (!(AT_Command.compareTo("NOWSTATE")))
      {
        Serial.println("AT+OK NOWSTATE");
        NOWSTATE();
      }
      else if (!(AT_Command.compareTo("SETDEL_WSH")))
      {
        Serial.println("AT+OK SETDEL_WSH");
        SETDEL_WSH(SerialData, dex1, end);
      }
      else if (!(AT_Command.compareTo("SETDEL_DRY")))
      {
        Serial.println("AT+OK SETDEL_DRY");
        SETDEL_DRY(SerialData, dex1, end);
      }
      else if (!(AT_Command.compareTo("DBCKSGHD")))
      {
        Serial.println("AT+OK DBCKSGHD");
        DBCKSGHD(SerialData, dex1, dexc, end);
      }
      else if (!(AT_Command.compareTo("NETWORK_INFO")))
      {
        Serial.println("AT+OK NETWORK_INFO");
        NETWORK_INFO();
      }
      else if (!(AT_Command.compareTo("SETAP_SSID")))
      {
        Serial.println("AT+OK SETAP_SSID");
        putString("ap_ssid", SerialData.substring(dex1 + 1, end - 1));
      }
      else if (!(AT_Command.compareTo("SETAP_PASSWD")))
      {
        Serial.println("AT+OK SETAP_PASSWD");
        putString("ap_passwd", SerialData.substring(dex1 + 1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_SERIALNO")))
      {
        Serial.println("AT+OK SET_SERIALNO");
        putString("serial_no", SerialData.substring(dex1 + 1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_AUTH_ID")))
      {
        Serial.println("AT+OK SET_AUTH_ID");
        putString("AUTH_ID", SerialData.substring(dex1 + 1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_AUTH_PASSWD")))
      {
        Serial.println("AT+OK SET_AUTH_PASSWD");
        putString("AUTH_PASSWD", SerialData.substring(dex1 + 1, end - 1));
      }
      else if (!(AT_Command.compareTo("FORMAT_NVS")))
      {
        Serial.println("AT+OK FORMAT_NVS");
        nvs_flash_erase();
        nvs_flash_init();
        ESP.restart();
      }
      else if (!(AT_Command.compareTo("SHOWMETHEMONEY")))
      {
        Serial.println("AT+OK SHOWMETHEMONEY");
        Serial.print(ESP.getFreeHeap());
        Serial.println("Byte");
      }
      else if (!(AT_Command.compareTo("WHATTIMEISIT")))
      {
        Serial.println("AT+OK WHATTIMEISIT");
        printLocalTime();
      }
      else if (!(AT_Command.compareTo("REBOOT")))
      {
        Serial.println("AT+OK REBOOT");
        delay(500);
        ESP.restart();
      }
      else
      {
        Serial.println("ERROR:Unknown command");
      }
    }
  }
}

// int SENSDATA_START(String command)
// {
//   Serial.println("SENS~");
//   int cnt = 0;
//   int num = 0;
//   if (!(command.compareTo("SENSDATA_START")))
//   {
//     Serial.println("AT+OK SENSDATA_START");
//     Serial.println("Waiting for 30 sec...");
//     Serial.println("CLOSE SERIAL PORT AND OPEN EXCEL LOG PROGRAM");
//     delay(30000);
//     Serial.println("CLEARDATA"); //처음에 데이터 리셋
//     Serial.println("LABEL,No.,AMP1,Drain1,L/h1,AMP2,Drain2,L/h2"); //엑셀 첫행 데이터 이름 설정
//     RunningStatistics inputStats1;
//     RunningStatistics inputStats2;

//     inputStats1.setWindowSecs(windowLength);
//     inputStats2.setWindowSecs(windowLength);
//     while (1)
//     {
//       if (Serial.available())
//       {
//         SerialData = Serial.readStringUntil('\n');
//         dex = SerialData.indexOf('+');
//         dex1 = SerialData.indexOf('"');
//         end = SerialData.length();
//         command = SerialData.substring(dex + 1, dex1);
//         if (!(command.compareTo("SENSDATA_END")))
//         {
//           break;
//         }
//       }

//       if (cnt >= 500)
//       {
//         ACS_Value1 = analogRead(ACS_Pin1);
//         ACS_Value2 = analogRead(ACS_Pin2);

//         inputStats1.input(ACS_Value1);
//         inputStats2.input(ACS_Value2);

//         Amps_TRMS1 = intercept + slope * inputStats1.sigma();
//         Amps_TRMS2 = intercept + slope * inputStats2.sigma();

//         WaterSensorData1 = digitalRead(Drain_1);
//         WaterSensorData2 = digitalRead(Drain_2);

//         l_hour1 = (flow_frequency1 * 60 / 7.5);
//         l_hour2 = (flow_frequency2 * 60 / 7.5);

//         flow_frequency1 = 0;
//         flow_frequency2 = 0;
//         num++;
//         cnt = 0;
//         Serial.print("DATA,");
//         Serial.print(num);
//         Serial.print(",");
//         Serial.print(Amps_TRMS1);
//         Serial.print(",");
//         Serial.print(WaterSensorData1);
//         Serial.print(",");
//         Serial.print(l_hour1);
//         Serial.print(",");
//         Serial.print(Amps_TRMS2);
//         Serial.print(",");
//         Serial.print(WaterSensorData2);
//         Serial.print(",");
//         Serial.print(l_hour2);
//         //Serial.print("Time : ");
//         //Serial.println(millis());
//         Serial.println();
//       }
//       cnt++;
//       delay(1);
//     }
//     return 1;
//   }

//   else
//   {
//     return 0;
//   }
// }

int SETNUM(String SerialData, int dex1, int end, int channel)
{
  if (channel == 1)
  {
    CH1_DeviceNo = SerialData.substring(dex1 + 1, end - 1);
    Serial.print("CH1_DeviceNo = ");
    Serial.println(CH1_DeviceNo);
    preferences.putString("CH1_DeviceNo", CH1_DeviceNo);
  }
  else if (channel == 2)
  {
    CH2_DeviceNo = SerialData.substring(dex1 + 1, end - 1);
    Serial.print("CH2_DeviceNo = ");
    Serial.println(CH2_DeviceNo);
    preferences.putString("CH2_DeviceNo", CH2_DeviceNo);
  }
}

int DBCKSGHD(String SerialData, int dex1, int dexc, int end)
{
  dexc = SerialData.indexOf(',');
  String Channel = SerialData.substring(dex1 + 1, dexc);
  String Number = SerialData.substring(dexc + 1, end);
  int Number_int = Number.toInt();
  int Channel_int = Channel.toInt();
  if (Channel_int == 1)
  {
    preferences.putBool("CH1_Live", Number_int);
    Serial.print("cksghd1 : ");
  }
  else if (Channel_int == 2)
  {
    preferences.putBool("CH2_Live", Number_int);
    Serial.print("cksghd2 : ");
  }
  Serial.println(Number_int);
  return 0;
}

int SETDEL_WSH(String SerialData, int dex1, int end)
{
  String Time = SerialData.substring(dex1 + 1, end);
  int Time_int = Time.toInt();
  if (Time_int < 255)
  {
    preferences.putUInt("endPeriod", Time_int);
    Serial.print("TimeSet : ");
    Serial.println(Time_int);
  }
  else
  {
    Serial.println("OverFlow");
  }
  return 0;
}

int SETDEL_DRY(String SerialData, int dex1, int end)
{
  String Time = SerialData.substring(dex1 + 1, end);
  int Time_int = Time.toInt();
  if (Time_int < 255)
  {
    preferences.putUInt("endPeriod_dryer", Time_int);
    Serial.print("TimeSet : ");
    Serial.println(Time_int);
  }
  else
  {
    Serial.println("OverFlow");
  }
  return 0;
}

int NOWSTATE()
{
  SetDefaultVal();
  return 0;
}

//=================================================================Status_Judgment모드
void Dryer_Status_Judgment(float Amps_TRMS, int cnt, int m, String DeviceNum, unsigned long previousMillis_end, int ChannelNum) {
  if (Amps_TRMS > 0.3)
  {
    if (cnt == 1)
    {
      if (ChannelNum == 1)
      {
        cnt1 = 0;
        digitalWrite(PIN_CH1_LED, HIGH);
      }
      if (ChannelNum == 2)
      {
        cnt2 = 0;
        digitalWrite(PIN_CH2_LED, HIGH);
      }
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Dryer Started");
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
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Dryer Ended");
      if (ChannelNum == 1)
      {
        cnt1 = 1;
        digitalWrite(PIN_CH1_LED, LOW);
      }
      if (ChannelNum == 2)
      {
        cnt2 = 1;
        digitalWrite(PIN_CH2_LED, LOW);
      }
    }
  }
}
void Status_Judgment(float Amps_TRMS, int WaterSensorData, unsigned int l_hour, int cnt, int m, unsigned long previousMillis_end, String DeviceNum, int ChannelNum) {
  if (Amps_TRMS > 0.5 || WaterSensorData || l_hour > 50)
  {
    if (cnt == 1)
    {
      if (ChannelNum == 1)
      {
        cnt1 = 0;
        digitalWrite(PIN_CH1_LED, HIGH);
      }
      if (ChannelNum == 2)
      {
        cnt2 = 0;
        digitalWrite(PIN_CH2_LED, HIGH);
      }
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Washer Started");
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
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Washer Ended");
      if (ChannelNum == 1)
      {
        cnt1 = 1;
        digitalWrite(PIN_CH1_LED, LOW);
      }
      if (ChannelNum == 2)
      {
        cnt2 = 1;
        digitalWrite(PIN_CH2_LED, LOW);
      }
    }
  }
}

void SetDefaultVal()
{
  ap_ssid = preferences.getString("ap_ssid", "");
  ap_passwd = preferences.getString("ap_passwd", "");
  serial_no = preferences.getString("serial_no", "0");
  auth_id = preferences.getString("AUTH_ID", "");
  auth_passwd = preferences.getString("AUTH_PASSWD", "");
  CH1_DeviceNo = preferences.getString("CH1_DeviceNo", "1");
  CH2_DeviceNo = preferences.getString("CH2_DeviceNo", "2");
  endPeriod = preferences.getUInt("endPeriod", 10);
  endPeriod_dryer = preferences.getUInt("endPeriod_dryer", 10);
  CH1_Live = preferences.getBool("CH1_Live", false);
  CH2_Live = preferences.getBool("CH2_Live", false);
  Device_Name = Device_Name + serial_no;
  Serial.print("CH1 : ");
  Serial.print(CH1_DeviceNo);
  Serial.print(" CH2 : ");
  Serial.println(CH2_DeviceNo);
  endPeriod *= 10000;
  endPeriod_dryer *= 1000;
  Serial.print("Time_WSH : ");
  Serial.print(endPeriod);
  Serial.print(" Time_DRY : ");
  Serial.println(endPeriod_dryer);
}

void NETWORK_INFO()
{
    // Serial.print("Name = ");
    // Serial.println(Device_Name);
    // Serial.println(WiFiStatusCode(WiFi.status()));
    // if (WiFi.status() == WL_CONNECTED)
    // {
    //     Serial.print("RSSI = ");
    //     Serial.println(WiFi.RSSI());
    //     String ip = WiFi.localIP().toString();
    //     Serial.printf("Local IP = %s\r\n", ip.c_str());
    // }
    // Serial.print("MAC = ");
    // Serial.println(WiFi.macAddress());
    // Serial.print("SSID = ");
    // Serial.println(ap_ssid);
    // Serial.print("PASSWORD = ");
    // Serial.print(ap_passwd);
}

void printLocalTime()
{
    // struct tm timeinfo;
    // if (!getLocalTime(&timeinfo))
    // {
    //     Serial.println("Failed to obtain time");
    //     return;
    // }
    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}