#include <Arduino.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include "ServerInfo.h"
#include "EmonLib.h"

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
WebSocketsClient webSocket;
StaticJsonDocument<200> doc;

// ============================================== millis()&if()

unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long previousMillis_end1 = 0;
unsigned long previousMillis_end2 = 0;
unsigned long echoPeriod1 = 200;
unsigned long echoPeriod2 = 200;
unsigned long led_millis_prev;
unsigned long curr_millis;

int m1 = 0, m2 = 0;

bool mode_debug = false;

//작동 시작 조건
float CH1_Current_Wash;
float CH2_Current_Wash;
unsigned int CH1_Flow_Wash;
unsigned int CH2_Flow_Wash;
float Ch1_Current_Dry;
float Ch2_Current_Dry;

//작동 종료 조건
unsigned int CH1_EndDelay_Wash;
unsigned int CH2_EndDelay_Wash;
unsigned int CH1_EndDelay_Dry;
unsigned int CH2_EndDelay_Dry;

bool CH1_Mode = false;
bool CH2_Mode = false;
bool CH1_CurrStatus = 1;
bool CH2_CurrStatus = 1;
int CH1_Cnt = 1;
int CH2_Cnt = 1;
bool CH1_Live = true;
bool CH2_Live = true;

String Device_Name = "OSJ_";
String ap_ssid;
String ap_passwd;
String serial_no;
String auth_id;
String auth_passwd;
String CH1_DeviceNo;
String CH2_DeviceNo;

bool wifi_fail = 1;

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

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("AP Connected");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print("WiFi connected ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  WiFi.disconnect(true);
  Serial.print("WiFi Lost. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  wifi_fail = 1;
  WiFi.begin(ap_ssid, ap_passwd);
}

const char *WiFiStatusCode(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "WL_NO_SHIELD";
  case WL_IDLE_STATUS:
    return "WL_IDLE_STATUS";
  case WL_NO_SSID_AVAIL:
    return "WL_NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "WL_SCAN_COMPLETED";
  case WL_CONNECTED:
    return "WL_CONNECTED";
  case WL_CONNECT_FAILED:
    return "WL_CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "WL_CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "WL_DISCONNECTED";
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
  {
    Serial.printf("[WSc] Connected to url: %s\n", payload);
    if (CH1_Live == true)
    {
      SendStatus(1, CH1_CurrStatus);
    }
    if (CH2_Live == true)
    {
      SendStatus(2, CH2_CurrStatus);
    }
  }
  break;
  case WStype_TEXT:
  {
    Serial.printf("[WSc] get text: %s\n", payload);
    deserializeJson(doc, payload);
    String title = doc["title"];
    if (title == "GetData")
    {
      StaticJsonDocument<500> MyStatus;
      MyStatus["title"] = "GetData";
      if (mode_debug)
      {
        MyStatus["debug"] = "No";
      }
      else
      {
        MyStatus["debug"] = "Yes";
      }
      if (CH1_Mode)
      {
        MyStatus["ch1_mode"] = "Wash";
      }
      else
      {
        MyStatus["ch1_mode"] = "Dry";
      }
      if (CH2_Mode)
      {
        MyStatus["ch2_mode"] = "Wash";
      }
      else
      {
        MyStatus["ch2_mode"] = "Dry";
      }
      if (CH1_CurrStatus)
      {
        MyStatus["ch1_status"] = "Not Working";
      }
      else
      {
        MyStatus["ch1_status"] = "Working";
      }
      if (CH2_CurrStatus)
      {
        MyStatus["ch2_status"] = "Not Working";
      }
      else
      {
        MyStatus["ch2_status"] = "Working";
      }
      MyStatus["ch1_current_wash"] = CH1_Current_Wash;
      MyStatus["ch2_current_wash"] = CH2_Current_Wash;
      MyStatus["ch1_flow_wash"] = CH1_Flow_Wash;
      MyStatus["ch2_flow_wash"] = CH2_Flow_Wash;  
      MyStatus["ch1_current_dry"] = Ch1_Current_Dry;
      MyStatus["ch1_current_dry"] = Ch2_Current_Dry;  
      MyStatus["ch1_enddelay_wash"] = CH1_EndDelay_Wash;
      MyStatus["ch2_enddelay_wash"] = CH2_EndDelay_Wash;
      MyStatus["ch1_enddelay_dry"] = CH1_EndDelay_Dry;
      MyStatus["ch2_enddelay_dry"] = CH2_EndDelay_Dry;
      MyStatus["ch1_deviceno"] = CH1_DeviceNo;
      MyStatus["ch2_deviceno"] = CH2_DeviceNo;
      MyStatus["ch1_current"] = Amps_TRMS1;
      MyStatus["ch2_current"] = Amps_TRMS2;
      MyStatus["ch1_flow"] = l_hour1;
      MyStatus["ch2_flow"] = l_hour2;
      MyStatus["ch1_drain"] = WaterSensorData1;
      MyStatus["ch2_drain"] = WaterSensorData2;
      MyStatus["wifi_ssid"] = ap_ssid;
      MyStatus["wifi_rssi"] = WiFi.RSSI();
      MyStatus["wifi_ip"] = WiFi.localIP().toString();
      MyStatus["mac"] = WiFi.macAddress();
      String MyStatus_String;
      serializeJson(MyStatus, MyStatus_String);
      webSocket.sendTXT(MyStatus_String);
    }
  }
  break;
  case WStype_BIN:
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

int SendStatus(int ch, bool status)
{
  if (ch == 1 && CH1_Live == false)
    return 1;
  if (ch == 2 && CH2_Live == false)
    return 1;
  if (WiFi.status() == WL_CONNECTED && webSocket.isConnected() == true)
  {
    StaticJsonDocument<100> CurrStatus;
    CurrStatus["title"] = "Update";
    if (ch == 1)
      CurrStatus["id"] = CH1_DeviceNo;
    if (ch == 2)
      CurrStatus["id"] = CH2_DeviceNo;
    CurrStatus["state"] = status;
    String CurrStatus_String;
    serializeJson(CurrStatus, CurrStatus_String);
    webSocket.sendTXT(CurrStatus_String);
    return 0;
  }
  else
  {
    Serial.println("SendStatus Fail - No Server Connection");
    return 1;
  }
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
  ct1.current(PIN_CT1, 30.7);
  ct2.current(PIN_CT2, 30.7);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW1), flow1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW2), flow2, FALLING);
  SetDefaultVal();
  mode_debug = digitalRead(PIN_DEBUG);
  CH1_Mode = digitalRead(PIN_CH1_MODE);
  CH2_Mode = digitalRead(PIN_CH2_MODE);
  if(mode_debug == 0)
  {
    Serial.println("YOU ARE IN THE DEBUG MODE !!!");
  }
  Serial.print("CH1 : ");
  Serial.println(CH1_Mode);
  Serial.print("CH2 : ");
  Serial.println(CH2_Mode);
  WiFi.disconnect(true);
  for (int i = 0; i < 30; i++) // 센서 안정화
  {
    ct1.calcIrms(1480);
    ct2.calcIrms(1480);
  }
  Serial.print("Boot Heap : ");
  Serial.println(ESP.getFreeHeap());
  digitalWrite(PIN_STATUS, HIGH);
  if (ap_ssid == "")
  {
    Serial.println("Skip WiFi Setting Due to No SSID");
  }
  else
  {
    Serial.print("Connecting to WiFi .. ");
    Serial.println(ap_ssid);
    if (ap_passwd == "")
    {
      WiFi.begin(ap_ssid);
    }
    else
    {
      WiFi.begin(ap_ssid, ap_passwd);
    }

    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    int wifi_timeout = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print('.');
      digitalWrite(PIN_STATUS, LOW);
      delay(100);
      digitalWrite(PIN_STATUS, HIGH);
      delay(100);
      wifi_timeout++;
      if (wifi_timeout > 25)
      {
        Serial.println("Skip WiFi Connection Due to Timeout");
        break;
      }
    }
  }
}

void loop()
{
  curr_millis = millis();
  if (WiFi.status() == WL_CONNECTED)
  {
    webSocket.loop();
    if (wifi_fail == 1)
    {
      digitalWrite(PIN_STATUS, HIGH);
      wifi_fail = 0;
      String ip = WiFi.localIP().toString();
      configTime(3600 * timeZone, 0, ntpServer);
      printLocalTime();
      webSocket.beginSSL(Server_domain, Server_port, Server_url);
      char HeaderData[35];
      sprintf(HeaderData, "HWID: %s\r\nCH1: %s\r\nCH2: %s", serial_no.c_str(), CH1_DeviceNo.c_str(), CH2_DeviceNo.c_str());
      webSocket.setExtraHeaders(HeaderData);
      webSocket.setAuthorization(auth_id.c_str(), auth_passwd.c_str());
      webSocket.onEvent(webSocketEvent);
    }
  }
  else
  {
    wifi_fail = 1;
    if (curr_millis - led_millis_prev >= 100)
    {
      led_millis_prev = curr_millis;
      digitalWrite(PIN_STATUS, !digitalRead(PIN_STATUS));
    }
  }

  if (mode_debug)
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
      Serial.println(Amps_TRMS1, 3);
      Serial.print("Water1 : ");
      Serial.println(WaterSensorData1);
      Serial.print("L/h1 : ");
      Serial.println(l_hour1);
      Serial.print("CT2 : ");
      Serial.println(Amps_TRMS2, 3);
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
      Status_Judgment(Amps_TRMS1, WaterSensorData1, l_hour1, CH1_Cnt, m1, previousMillis_end1, 1);
    }
    else
    {
      Dryer_Status_Judgment(Amps_TRMS1, CH1_Cnt, m1, previousMillis_end1, 1);
    }

    if (CH2_Mode)
    {
      Status_Judgment(Amps_TRMS2, WaterSensorData2, l_hour2, CH2_Cnt, m2, previousMillis_end2, 2);
    }
    else
    {
      Dryer_Status_Judgment(Amps_TRMS2, CH2_Cnt, m2, previousMillis_end2, 2);
    }
  }
  //=======================================================================디버그 모드
  else
  {
    curr_millis = millis();
    bool led_status = 0;
    if (curr_millis - led_millis_prev >= 100)
    {
      led_millis_prev = curr_millis;
      if (led_status == 1)
      {
        led_status = 0;
        digitalWrite(PIN_CH1_LED, HIGH);
        digitalWrite(PIN_CH2_LED, LOW);
      }
      else if (led_status == 0)
      {
        led_status = 1;
        digitalWrite(PIN_CH1_LED, LOW);
        digitalWrite(PIN_CH2_LED, HIGH);
      }
    }
    if (Serial.available())
    {
      int dex, dex1, dexc, end;
      String SerialData = Serial.readStringUntil('\n');
      dex = SerialData.indexOf('+');
      dex1 = SerialData.indexOf('"');
      end = SerialData.length();
      String AT_Command = SerialData.substring(dex + 1, dex1);
      if (!(AT_Command.compareTo("HELP")))
      {
      }
      else if (!(AT_Command.compareTo("SENSDATA_START")))
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

int SETNUM(String sd, int dex1, int end, int channel)
{
  if (channel == 1)
  {
    CH1_DeviceNo = sd.substring(dex1 + 1, end - 1);
    Serial.print("CH1_DeviceNo = ");
    Serial.println(CH1_DeviceNo);
    preferences.putString("CH1_DeviceNo", CH1_DeviceNo);
  }
  else if (channel == 2)
  {
    CH2_DeviceNo = sd.substring(dex1 + 1, end - 1);
    Serial.print("CH2_DeviceNo = ");
    Serial.println(CH2_DeviceNo);
    preferences.putString("CH2_DeviceNo", CH2_DeviceNo);
  }
  return 0;
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
    Serial.print("cksghd_ch1 : ");
  }
  else if (Channel_int == 2)
  {
    preferences.putBool("CH2_Live", Number_int);
    Serial.print("cksghd_ch2 : ");
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
    preferences.putUInt("CH1_EndDelay_Wash", Time_int);
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
    preferences.putUInt("CH1_EndDelay_Dry", Time_int);
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
void Dryer_Status_Judgment(float Amps_TRMS, int cnt, int m, unsigned long previousMillis_end, int ChannelNum)
{
  if (ChannelNum == 1 && Amps_TRMS > Ch1_Current_Dry)
  {
    if (cnt == 1) // 건조기 동작 시작
    {
      CH1_Cnt = 0;
      digitalWrite(PIN_CH1_LED, HIGH);
      CH2_CurrStatus = 0;
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Dryer Started");
      SendStatus(ChannelNum, 0);
    }
    m1 = 1;
  }
  else if (ChannelNum == 2 && Amps_TRMS > Ch2_Current_Dry)
  {
    if (cnt == 1) // 건조기 동작 시작
    {
      CH2_Cnt = 0;
      digitalWrite(PIN_CH2_LED, HIGH);
      CH2_CurrStatus = 0;
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Dryer Started");
      SendStatus(ChannelNum, 0);
    }
    m2 = 1;
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
      if (ChannelNum == 1)
        m1 = 0;
      if (ChannelNum == 2)
        m2 = 0;
    }
    else if (cnt)
      ;
    else if (ChannelNum == 1 && millis() - previousMillis_end >= CH1_EndDelay_Dry) // CH1 건조기 동작 종료
    {
      Serial.println("CH1 Dryer Ended");
      SendStatus(1, 1);
      CH1_Cnt = 1;
      digitalWrite(PIN_CH1_LED, LOW);
      CH1_CurrStatus = 1;
    }
    else if (ChannelNum == 2 && millis() - previousMillis_end >= CH2_EndDelay_Dry) // CH2 건조기 동작 종료
    {
      Serial.println("CH2 Dryer Ended");
      SendStatus(2, 1);
      CH2_Cnt = 1;
      digitalWrite(PIN_CH2_LED, LOW);
      CH2_CurrStatus = 1;
    }
  }
}

void Status_Judgment(float Amps_TRMS, int WaterSensorData, unsigned int l_hour, int cnt, int m, unsigned long previousMillis_end, int ChannelNum)
{
  if (Amps_TRMS > 0.5 || WaterSensorData || l_hour > 50)
  {
    if (cnt == 1) // 세탁기 동작 시작
    {
      if (ChannelNum == 1)
      {
        CH1_Cnt = 0;
        digitalWrite(PIN_CH1_LED, HIGH);
        CH1_CurrStatus = 0;
      }
      if (ChannelNum == 2)
      {
        CH2_Cnt = 0;
        digitalWrite(PIN_CH2_LED, HIGH);
        CH2_CurrStatus = 0;
      }
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Washer Started");
      SendStatus(ChannelNum, 0);
    }
    if (ChannelNum == 1)
      m1 = 1;
    if (ChannelNum == 2)
      m2 = 1;
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
      if (ChannelNum == 1)
        m1 = 0;
      if (ChannelNum == 2)
        m2 = 0;
    }
    else if (cnt)
      ;
    else if (ChannelNum == 1 && millis() - previousMillis_end >= CH1_EndDelay_Wash) // CH1 세탁기 동작 종료
    {
      Serial.println("CH1 Washer Ended");
      SendStatus(1, 1);
      CH1_Cnt = 1;
      digitalWrite(PIN_CH1_LED, LOW);
      CH1_CurrStatus = 1;
    }
    else if (ChannelNum == 2 && millis() - previousMillis_end >= CH2_EndDelay_Wash) // CH2 세탁기 동작 종료
    {
      Serial.println("CH2 Washer Ended");
      SendStatus(2, 1);
      CH2_Cnt = 1;
      digitalWrite(PIN_CH2_LED, LOW);
      CH2_CurrStatus = 1;
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

  CH1_Current_Wash = preferences.getFloat("CH1_Current_Wash", 0.2);
  CH2_Current_Wash = preferences.getFloat("CH2_Current_Wash", 0.2);
  CH1_Flow_Wash = preferences.getUInt("CH1_Flow_Wash", 50);
  CH2_Flow_Wash = preferences.getUInt("CH2_Flow_Wash", 50);
  Ch1_Current_Dry = preferences.getFloat("Ch1_Current_Dry", 0.5);
  Ch2_Current_Dry = preferences.getFloat("Ch2_Current_Dry", 0.5);

  CH1_EndDelay_Wash = preferences.getUInt("CH1_EndDelay_Wash", 10);
  CH2_EndDelay_Wash = preferences.getUInt("CH2_EndDelay_Wash", 10);
  CH1_EndDelay_Dry = preferences.getUInt("CH1_EndDelay_Dry", 10);
  CH2_EndDelay_Dry = preferences.getUInt("CH2_EndDelay_Dry", 10);

  CH1_Live = preferences.getBool("CH1_Live", true);
  CH2_Live = preferences.getBool("CH2_Live", true);

  Device_Name = Device_Name + serial_no;
  WiFi.setHostname(Device_Name.c_str());
  Serial.print("My Name Is :");
  Serial.println(Device_Name);
  Serial.print("CH1 : ");
  Serial.print(CH1_DeviceNo);
  Serial.print(" CH2 : ");
  Serial.println(CH2_DeviceNo);
  if (auth_id == "" || auth_passwd == "")
  {
    Serial.println("NO AUTH CODE!!! YOU NEED TO CONFIG SERVER AUTHENTICATION BY AT+SET_AUTH_ID AND AT+SET_AUTH_PASSWD IN DEBUG MODE!!!");
  }
  if (ap_ssid == "")
  {
    Serial.println("NO WIFI SSID!!! YOU NEED TO CONFIG WIFI BY AT+SETAP_SSID AND AT+SETAP_PASSWD IN DEBUG MODE!!!");
  }
  CH1_EndDelay_Wash *= 10000;
  CH2_EndDelay_Wash *= 10000;
  CH1_EndDelay_Dry *= 1000;
  CH2_EndDelay_Dry *= 1000;
  Serial.print("Ch1_Current_Dry : ");
  Serial.print(Ch1_Current_Dry);
  Serial.print(" Ch2_Current_Dry : ");
  Serial.print(Ch2_Current_Dry);
  Serial.print(" CH1_Delay_WSH : ");
  Serial.print(CH1_EndDelay_Wash);
  Serial.print(" CH2_Delay_WSH : ");
  Serial.print(CH2_EndDelay_Wash);
  Serial.print(" CH1_Delay_Dry : ");
  Serial.print(CH1_EndDelay_Dry);
  Serial.print(" CH2_Delay_Dry : ");
  Serial.println(CH2_EndDelay_Dry);
}

void NETWORK_INFO()
{
  Serial.print("Name = ");
  Serial.println(Device_Name);
  Serial.println(WiFiStatusCode(WiFi.status()));
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("RSSI = ");
    Serial.println(WiFi.RSSI());
    String ip = WiFi.localIP().toString();
    Serial.printf("Local IP = %s\r\n", ip.c_str());
  }
  Serial.print("MAC = ");
  Serial.println(WiFi.macAddress());
  Serial.print("SSID = ");
  Serial.println(ap_ssid);
  Serial.print("PASSWORD = ");
  Serial.print(ap_passwd);
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}