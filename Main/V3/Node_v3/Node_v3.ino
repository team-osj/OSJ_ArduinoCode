#include <Arduino.h>
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

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "FS.h"
#include <ESPmDNS.h>
#include <Update.h>
#include "manager_html.h"
#include "ok_html.h"
#include "time.h"

#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

//GPIO Define
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

AsyncWebServer server(80);
WiFiClient client;
DynamicJsonDocument jsonBuffer(2048);

// ============================================== millis()&if()
bool rebooting = false;

bool wifi_enable = true;
bool wifi_retry = true;

unsigned long printPeriod = 500;
unsigned long previousMillis = 0;
unsigned long previousMillis_end1 = 0;
unsigned long previousMillis_end2 = 0;
unsigned long echoPeriod1 = 200;
unsigned long echoPeriod2 = 200;
unsigned long led_millis_prev;
unsigned long curr_millis;
unsigned long server_retry_millis;

int m1 = 0, m2 = 0;

unsigned int timeSendFlag1 = 0;
unsigned int timeSendFlag2 = 0;

bool mode_debug = false;

bool led_status = 0;

//작동 시작 조건
float CH1_Curr_W;
float CH2_Curr_W;
unsigned int CH1_Flow_W;
unsigned int CH2_Flow_W;
float CH1_Curr_D;
float CH2_Curr_D;

//작동 종료 조건
unsigned int CH1_EndDelay_W;
unsigned int CH2_EndDelay_W;
unsigned int CH1_EndDelay_D;
unsigned int CH2_EndDelay_D;

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
  webSocket.disconnect();
  server_retry_millis = curr_millis;
  String ip = WiFi.localIP().toString();
  webSocket.beginSSL(Server_domain, Server_port, Server_url);
  char HeaderData[35];
  sprintf(HeaderData, "HWID: %s\r\nCH1: %s\r\nCH2: %s", serial_no.c_str(), CH1_DeviceNo.c_str(), CH2_DeviceNo.c_str());
  webSocket.setExtraHeaders(HeaderData);
  webSocket.setAuthorization(auth_id.c_str(), auth_passwd.c_str());
  webSocket.onEvent(webSocketEvent);
  MDNS.begin(Device_Name);
  Serial.printf("Host: http://%s.local/\n",Device_Name);
  setupAsyncServer();
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
        if (mode_debug)
        {
          if (CH1_Live == true)
          {
            SendStatus(1, CH1_CurrStatus);
          }
          if (CH2_Live == true)
          {
            SendStatus(2, CH2_CurrStatus);
          }
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
          StaticJsonDocument<600> MyStatus;
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
          MyStatus["CH1_Curr_W"] = CH1_Curr_W;
          MyStatus["CH2_Curr_W"] = CH2_Curr_W;
          MyStatus["CH1_Flow_W"] = CH1_Flow_W;
          MyStatus["CH2_Flow_W"] = CH2_Flow_W;
          MyStatus["CH1_Curr_D"] = CH1_Curr_D;
          MyStatus["CH2_Curr_D"] = CH2_Curr_D;
          MyStatus["CH1_EndDelay_W"] = CH1_EndDelay_W;
          MyStatus["CH2_EndDelay_W"] = CH2_EndDelay_W;
          MyStatus["CH1_EndDelay_D"] = CH1_EndDelay_D;
          MyStatus["CH2_EndDelay_D"] = CH2_EndDelay_D;
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
          MyStatus["fw_ver"] = build_date;
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
    {
      CurrStatus["id"] = CH1_DeviceNo;
      CurrStatus["type"] = timeSendFlag1;
      timeSendFlag1 = 0;
    }
    if (ch == 2)
    {
      CurrStatus["id"] = CH2_DeviceNo;
      CurrStatus["type"] = timeSendFlag2;
      timeSendFlag2 = 0;
    }
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

int SendLog(int ch, String log)
{
  if (ch == 1 && CH1_Live == false)
    return 1;
  if (ch == 2 && CH2_Live == false)
    return 1;
  if (WiFi.status() == WL_CONNECTED && webSocket.isConnected() == true)
  {
    DynamicJsonDocument LogData(1024);
    LogData["title"] = "Log";
    if (ch == 1)
      LogData["id"] = CH1_DeviceNo;
    if (ch == 2)
      LogData["id"] = CH2_DeviceNo;
    LogData["log"] = log;
    String LogData_String;
    serializeJson(LogData, LogData_String);
    webSocket.sendTXT(LogData_String);
    return 0;
  }
  else
  {
    Serial.println("SendLog Fail - No Server Connection");
    return 1;
  }
}

void setup()
{
  preferences.begin("config", false);
  Serial.begin(115200);
  Serial.print("FW_VER : ");
  Serial.println(build_date);
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
  if (mode_debug == 0)
  {
    Serial.println("YOU ARE IN THE DEBUG MODE !!!");
  }
  Serial.print("CH1_Mode : ");
  Serial.println(CH1_Mode);
  Serial.print("CH2_Mode : ");
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
  if(rebooting)
  {
    delay(100);
    ESP.restart();
  }
  // if (wifi_fail == 1)
  // {
  //   if(curr_millis - 200 >= server_retry_millis)
  //   {
  //     webSocket.disconnect();
  //     server_retry_millis = curr_millis;
  //     String ip = WiFi.localIP().toString();
  //     webSocket.beginSSL(Server_domain, Server_port, Server_url);
  //     char HeaderData[35];
  //     sprintf(HeaderData, "HWID: %s\r\nCH1: %s\r\nCH2: %s", serial_no.c_str(), CH1_DeviceNo.c_str(), CH2_DeviceNo.c_str());
  //     webSocket.setExtraHeaders(HeaderData);
  //     webSocket.setAuthorization(auth_id.c_str(), auth_passwd.c_str());
  //     webSocket.onEvent(webSocketEvent);
  //     MDNS.begin(Device_Name);
  //     Serial.printf("Host: http://%s.local/\n",Device_Name);
  //     setupAsyncServer();
  //   }
  // }
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(PIN_STATUS, HIGH);
    wifi_fail = 0;
    webSocket.loop();
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

      l_hour1 = (flow_frequency1 * 60/7.5); // L/hour계산
      l_hour2 = (flow_frequency2 * 60/7.5);

      flow_frequency1 = 0; // 변수 초기화
      flow_frequency2 = 0;
      /*Serial.print("CT1 : ");
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
      Serial.println();*/
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
      String AT_Command = SerialData.substring(dex+1, dex1);
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
      else if (!(AT_Command.compareTo("CH1_SETVAR")))
      {
        Serial.println("AT+OK CH1_SETVAR");
        CH1_SETVAR(SerialData, dex1, dexc, end);
      }
      else if (!(AT_Command.compareTo("CH2_SETVAR")))
      {
        Serial.println("AT+OK CH2_SETVAR");
        CH2_SETVAR(SerialData, dex1, dexc, end);
      }
      else if (!(AT_Command.compareTo("UPDATE")))
      {
        Serial.println("AT+OK UPDATE");
      }
      else if (!(AT_Command.compareTo("NETWORK_INFO")))
      {
        Serial.println("AT+OK NETWORK_INFO");
        NETWORK_INFO();
      }
      else if (!(AT_Command.compareTo("SETAP_SSID")))
      {
        Serial.println("AT+OK SETAP_SSID");
        putString("ap_ssid", SerialData.substring(dex1+1, end - 1));
      }
      else if (!(AT_Command.compareTo("SETAP_PASSWD")))
      {
        Serial.println("AT+OK SETAP_PASSWD");
        putString("ap_passwd", SerialData.substring(dex1+1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_SERIALNO")))
      {
        Serial.println("AT+OK SET_SERIALNO");
        putString("serial_no", SerialData.substring(dex1+1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_AUTH_ID")))
      {
        Serial.println("AT+OK SET_AUTH_ID");
        putString("AUTH_ID", SerialData.substring(dex1+1, end - 1));
      }
      else if (!(AT_Command.compareTo("SET_AUTH_PASSWD")))
      {
        Serial.println("AT+OK SET_AUTH_PASSWD");
        putString("AUTH_PASSWD", SerialData.substring(dex1+1, end - 1));
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
      }
      else if (!(AT_Command.compareTo("REBOOT")))
      {
        Serial.println("AT+OK REBOOT");
        delay(500);
        ESP.restart();
      }
      else
      {
        Serial.println("ERROR: Unknown command");
      }
    }
  }
}

int CH1_SETVAR(String SerialData, int dex1, int dexc, int end)
{
  dexc = SerialData.indexOf(',');
  String command = SerialData.substring(dex1+1, dexc);
  String Number = SerialData.substring(dexc+1, end - 1);
  if (command == "DeviceNo")
  {
    Serial.print("CH1_DeviceNo : ");
    Serial.println(Number);
    preferences.putString("CH1_DeviceNo", Number);
  }
  else if (command == "Current_Wash")
  {
    Serial.print("CH1_Curr_W : ");
    Serial.println(Number);
    float var = Number.toFloat();
    preferences.putFloat("CH1_Curr_W", var);
  }
  else if (command == "Flow_Wash")
  {
    Serial.print("CH1_Flow_W : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH1_Flow_W", var);
  }
  else if (command == "Current_Dry")
  {
    Serial.print("CH1_Curr_D : ");
    Serial.println(Number);
    float var = Number.toFloat();
    preferences.putFloat("CH1_Curr_D", var);
  }
  else if (command == "EndDelay_Wash")
  {
    Serial.print("CH1_EndDelay_W : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH1_EndDelay_W", var);
  }
  else if (command == "EndDelay_Dry")
  {
    Serial.print("CH1_EndDelay_D : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH1_EndDelay_D", var);
  }
  else if (command == "Enable")
  {
    Serial.print("CH1_Enable : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putBool("CH1_Live", var);
  }
  else
  {
    Serial.print("Command Not Found for : ");
    Serial.println(command);
  }
  return 0;
}

int CH2_SETVAR(String SerialData, int dex1, int dexc, int end)
{
  dexc = SerialData.indexOf(',');
  String command = SerialData.substring(dex1+1, dexc);
  String Number = SerialData.substring(dexc+1, end - 1);
  if (command == "DeviceNo")
  {
    Serial.print("CH2_DeviceNo : ");
    Serial.println(Number);
    preferences.putString("CH2_DeviceNo", Number);
  }
  else if (command == "Current_Wash")
  {
    Serial.print("CH2_Curr_W : ");
    Serial.println(Number);
    float var = Number.toFloat();
    preferences.putFloat("CH2_Curr_W", var);
  }
  else if (command == "Flow_Wash")
  {
    Serial.print("CH2_Flow_W : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH2_Flow_W", var);
  }
  else if (command == "Current_Dry")
  {
    Serial.print("CH2_Curr_D : ");
    Serial.println(Number);
    float var = Number.toFloat();
    preferences.putFloat("CH2_Curr_D", var);
  }
  else if (command == "EndDelay_Wash")
  {
    Serial.print("CH2_EndDelay_W : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH2_EndDelay_W", var);
  }
  else if (command == "EndDelay_Dry")
  {
    Serial.print("CH2_EndDelay_D : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putUInt("CH2_EndDelay_D", var);
  }
  else if (command == "Enable")
  {
    Serial.print("CH2_Enable : ");
    Serial.println(Number);
    int var = Number.toInt();
    preferences.putBool("CH2_Live", var);
  }
  else
  {
    Serial.print("Command Not Found for : ");
    Serial.println(command);
  }
  return 0;
}

int NOWSTATE()
{
  SetDefaultVal();
  return 0;
}

/*int dryer_prev_millis1 = 0;
int dryer_prev_millis2 = 0;
int dryer_cnt1 = 0;
int dryer_cnt2 = 0;*/
int json_log_flag1 = 0;
int json_log_flag2 = 0;
int json_log_flag1_c = 0;
int json_log_flag2_c = 0;
int json_log_flag1_f = 0;
int json_log_flag2_f = 0;
int json_log_flag1_w = 0;
int json_log_flag2_w = 0;
int json_log_millis1 = 0;
int json_log_millis2 = 0;
int json_log_cnt1 = 1;
int json_log_cnt2 = 1;
DynamicJsonDocument json_log1(1024);
DynamicJsonDocument json_log2(1024);

//건조기 동작 판단
void Dryer_Status_Judgment(float Amps_TRMS, int cnt, int m, unsigned long previousMillis_end, int ChannelNum)
{
  /*if (ChannelNum == 1 && Amps_TRMS > CH1_Curr_D && dryer_cnt1 == 0){ //전류가 흐르면 millis시작
  dryer_cnt1 = 1;
  dryer_prev_millis1 = millis();
  }
  if (ChannelNum == 1 && Amps_TRMS < CH1_Curr_D && dryer_cnt1 == 1){ //전류가 끊기면 cnt로 시작 취소
  dryer_cnt1 = 0;
  }

  if (ChannelNum == 2 && Amps_TRMS > CH2_Curr_D && dryer_cnt2 == 0){
  dryer_cnt2 = 1;
  dryer_prev_millis2 = millis();
  }
  if (ChannelNum == 2 && Amps_TRMS < CH2_Curr_D && dryer_cnt2 == 1){
  dryer_cnt2 = 0;
  }*/
  if (ChannelNum == 1 && Amps_TRMS < CH1_Curr_D && json_log_flag1) {
    if (json_log_flag1_c == 1) {
      json_log_flag1_c = 0;
      String json_log_cnt1_string = String(json_log_cnt1);
      json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
      json_log1[json_log_cnt1_string]["n"] = "C";
      json_log1[json_log_cnt1_string]["s"] = 0;
      if (json_log_cnt1 % 100 == 0) {
        String json_log_data1 = "";
        serializeJson(json_log1, json_log_data1);
        json_log1.clear();
        SendLog(1, json_log_data1);
      }
      json_log_cnt1++;
    }
  }
  if (ChannelNum == 2 && Amps_TRMS < CH2_Curr_D && json_log_flag2) {
    if (json_log_flag2_c == 1) {
      json_log_flag2_c = 0;
      String json_log_cnt2_string = String(json_log_cnt2);
      json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
      json_log2[json_log_cnt2_string]["n"] = "C";
      json_log2[json_log_cnt2_string]["s"] = 0;
      if (json_log_cnt2 % 100 == 0) {
        String json_log_data2 = "";
        serializeJson(json_log2, json_log_data2);
        json_log2.clear();
        SendLog(2, json_log_data2);
      }
      json_log_cnt2++;
    }
  }
  if (ChannelNum == 1 && Amps_TRMS > CH1_Curr_D)
  {
    if (json_log_flag1) {
      if (json_log_flag1_c == 0) {
        json_log_flag1_c = 1;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "C";
        json_log1[json_log_cnt1_string]["s"] = 1;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }
    }
    if (cnt == 1) // CH1 건조기 동작 시작
    {
      timeSendFlag1 = 1;
      json_log_flag1 = 1;
      json_log_cnt1 = 1;
      json_log_millis1 = millis();
      String local_time = "";
      json_log1["START"]["local_time"] = local_time;

      //dryer_cnt1 = 0;
      CH1_Cnt = 0;
      digitalWrite(PIN_CH1_LED, HIGH);
      CH1_CurrStatus = 0;
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Dryer Started");
      SendStatus(ChannelNum, 0);
    }
    m1 = 1;
  }
  else if (ChannelNum == 2 && Amps_TRMS > CH2_Curr_D)
  {
    if (json_log_flag2) {
      if (json_log_flag2_c == 0) {
        json_log_flag2_c = 1;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "C";
        json_log2[json_log_cnt2_string]["s"] = 1;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }
    }
    if (cnt == 1) // CH2 건조기 동작 시작
    {
      timeSendFlag2 = 1;

      json_log_flag2 = 1;
      json_log_cnt2 = 1;
      json_log_millis2 = millis();

      String local_time = "";
      json_log2["START"]["local_time"] = local_time;

      //dryer_cnt2 = 0;
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
    else if (ChannelNum == 1 && millis() - previousMillis_end >= CH1_EndDelay_D) // CH1 건조기 동작 종료
    {
      timeSendFlag1 = 1;

      json_log_flag1_c = 0;
      json_log_flag1 = 0;


      String local_time = "";
      json_log1["END"]["local_time"] = local_time;

      String json_log_data1 = "";
      serializeJson(json_log1, json_log_data1);
      //Serial.println(json_log_data1);
      json_log1.clear();
      Serial.println("CH1 Dryer Ended");
      SendStatus(1, 1);
      SendLog(1, json_log_data1);
      CH1_Cnt = 1;
      digitalWrite(PIN_CH1_LED, LOW);
      CH1_CurrStatus = 1;
    }
    else if (ChannelNum == 2 && millis() - previousMillis_end >= CH2_EndDelay_D) // CH2 건조기 동작 종료
    {
      timeSendFlag2 = 1;

      json_log_flag2_c = 0;
      json_log_flag2 = 0;


      String local_time = "";
      json_log2["END"]["local_time"] = local_time;

      String json_log_data2 = "";
      serializeJson(json_log2, json_log_data2);
      //Serial.println(json_log_data2);
      json_log2.clear();
      Serial.println("CH2 Dryer Ended");
      SendStatus(2, 1);
      SendLog(2, json_log_data2);
      CH2_Cnt = 1;
      digitalWrite(PIN_CH2_LED, LOW);
      CH2_CurrStatus = 1;
    }
  }
}

int se_prev_millis1 = 0;
int se_prev_millis2 = 0;
int se_cnt1 = 0;
int se_cnt2 = 0;

//세탁기 동작 판단
void Status_Judgment(float Amps_TRMS, int WaterSensorData, unsigned int l_hour, int cnt, int m, unsigned long previousMillis_end, int ChannelNum)
{
  if (ChannelNum == 1 && (Amps_TRMS > CH1_Curr_W || WaterSensorData || l_hour > CH1_Flow_W) && se_cnt1 == 0) { //세탁기가 동작하면 millis시작
    se_cnt1 = 1;
    se_prev_millis1 = millis();
  }
  if (ChannelNum == 1 && (Amps_TRMS < CH1_Curr_W && !WaterSensorData && l_hour < CH1_Flow_W) && se_cnt1 == 1) { // 전부 멈추면 시작 취소
    se_cnt1 = 0;
  }

  if (ChannelNum == 2 && (Amps_TRMS > CH2_Curr_W || WaterSensorData || l_hour > CH2_Flow_W) && se_cnt2 == 0) {
    se_cnt2 = 1;
    se_prev_millis2 = millis();
  }
  if (ChannelNum == 2 && (Amps_TRMS < CH2_Curr_W && !WaterSensorData && l_hour < CH2_Flow_W) && se_cnt2 == 1) {
    se_cnt2 = 0;
  }

  if (ChannelNum == 1) {
    if (json_log_flag1) {
      if (Amps_TRMS > CH1_Curr_W && json_log_flag1_c == 0) {
        json_log_flag1_c = 1;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "C";
        json_log1[json_log_cnt1_string]["s"] = 1;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }
      if (Amps_TRMS < CH1_Curr_W && json_log_flag1_c == 1) {
        json_log_flag1_c = 0;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "C";
        json_log1[json_log_cnt1_string]["s"] = 0;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }

      if (l_hour > CH1_Flow_W && json_log_flag1_f == 0) {
        json_log_flag1_f = 1;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "F";
        json_log1[json_log_cnt1_string]["s"] = 1;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }
      if (l_hour < CH1_Flow_W && json_log_flag1_f == 1) {
        json_log_flag1_f = 0;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "F";
        json_log1[json_log_cnt1_string]["s"] = 0;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }

      if (WaterSensorData && json_log_flag1_w == 0) {
        json_log_flag1_w = 1;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "W";
        json_log1[json_log_cnt1_string]["s"] = 1;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }
      if (!WaterSensorData && json_log_flag1_w == 1) {
        json_log_flag1_w = 0;
        String json_log_cnt1_string = String(json_log_cnt1);
        json_log1[json_log_cnt1_string]["t"] = millis() - json_log_millis1;
        json_log1[json_log_cnt1_string]["n"] = "W";
        json_log1[json_log_cnt1_string]["s"] = 0;
        if (json_log_cnt1 % 100 == 0) {
          String json_log_data1 = "";
          serializeJson(json_log1, json_log_data1);
          json_log1.clear();
          SendLog(1, json_log_data1);
        }
        json_log_cnt1++;
      }
    }
  }

  if (ChannelNum == 2) {
    if (json_log_flag2) {
      if (Amps_TRMS > CH2_Curr_W && json_log_flag2_c == 0) {
        json_log_flag2_c = 1;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "C";
        json_log2[json_log_cnt2_string]["s"] = 1;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }
      if (Amps_TRMS < CH2_Curr_W && json_log_flag2_c == 1) {
        json_log_flag2_c = 0;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "C";
        json_log2[json_log_cnt2_string]["s"] = 0;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }

      if (l_hour > CH2_Flow_W && json_log_flag2_f == 0) {
        json_log_flag2_f = 1;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "F";
        json_log2[json_log_cnt2_string]["s"] = 1;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }
      if (l_hour < CH2_Flow_W && json_log_flag2_f == 1) {
        json_log_flag2_f = 0;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "F";
        json_log2[json_log_cnt2_string]["s"] = 0;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }

      if (WaterSensorData && json_log_flag2_w == 0) {
        json_log_flag2_w = 1;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "W";
        json_log2[json_log_cnt2_string]["s"] = 1;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }
      if (!WaterSensorData && json_log_flag2_w == 1) {
        json_log_flag2_w = 0;
        String json_log_cnt2_string = String(json_log_cnt2);
        json_log2[json_log_cnt2_string]["t"] = millis() - json_log_millis2;
        json_log2[json_log_cnt2_string]["n"] = "W";
        json_log2[json_log_cnt2_string]["s"] = 0;
        if (json_log_cnt2 % 100 == 0) {
          String json_log_data2 = "";
          serializeJson(json_log2, json_log_data2);
          json_log2.clear();
          SendLog(2, json_log_data2);
        }
        json_log_cnt2++;
      }
    }
  }

  if (ChannelNum == 1 && millis() - se_prev_millis1 >= 500 && se_cnt1 == 1)
  {
    if (cnt == 1) // CH1 세탁기 동작 시작
    {
      timeSendFlag1 = 1;

      json_log_flag1 = 1;
      json_log_cnt1 = 1;
      json_log_millis1 = millis();

      String local_time = "";
      json_log1["START"]["local_time"] = local_time;

      se_cnt1 = 0;
      CH1_Cnt = 0;
      digitalWrite(PIN_CH1_LED, HIGH);
      CH1_CurrStatus = 0;
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Washer Started");
      SendStatus(ChannelNum, 0);
    }
    m1 = 1;
  }
  if (ChannelNum == 2 && millis() - se_prev_millis2 >= 500 && se_cnt2 == 1)
  {
    if (cnt == 1) // CH2 세탁기 동작 시작
    {
      timeSendFlag2 = 1;

      json_log_flag2 = 1;
      json_log_cnt2 = 1;
      json_log_millis2 = millis();

      String local_time = "";
      json_log2["START"]["local_time"] = local_time;

      se_cnt2 = 0;
      CH2_Cnt = 0;
      digitalWrite(PIN_CH2_LED, HIGH);
      CH2_CurrStatus = 0;
      Serial.print("CH");
      Serial.print(ChannelNum);
      Serial.println(" Washer Started");
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
    else if (ChannelNum == 1 && millis() - previousMillis_end >= CH1_EndDelay_W) // CH1 세탁기 동작 종료
    {
      timeSendFlag1 = 1;

      json_log_flag1_c = 0;
      json_log_flag1_f = 0;
      json_log_flag1_w = 0;
      json_log_flag1 = 0;
      String local_time = "";
      json_log1["END"]["local_time"] = local_time;

      String json_log_data1 = "";
      serializeJson(json_log1, json_log_data1);
      //Serial.println(json_log_data1);
      json_log1.clear();
      Serial.println("CH1 Washer Ended");
      SendStatus(1, 1);
      SendLog(1, json_log_data1);
      CH1_Cnt = 1;
      digitalWrite(PIN_CH1_LED, LOW);
      CH1_CurrStatus = 1;
    }
    else if (ChannelNum == 2 && millis() - previousMillis_end >= CH2_EndDelay_W) // CH2 세탁기 동작 종료
    {
      timeSendFlag2 = 1;

      json_log_flag2_c = 0;
      json_log_flag2_f = 0;
      json_log_flag2_w = 0;
      json_log_flag2 = 0;
      String local_time = "";
      json_log2["END"]["local_time"] = local_time;

      String json_log_data2 = "";
      serializeJson(json_log2, json_log_data2);
      //Serial.println(json_log_data2);
      json_log2.clear();
      Serial.println("CH2 Washer Ended");
      SendStatus(2, 1);
      SendLog(2, json_log_data2);
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

  CH1_Curr_W = preferences.getFloat("CH1_Curr_W", 0.2);
  CH2_Curr_W = preferences.getFloat("CH2_Curr_W", 0.2);
  CH1_Flow_W = preferences.getUInt("CH1_Flow_W", 50);
  CH2_Flow_W = preferences.getUInt("CH2_Flow_W", 50);
  CH1_Curr_D = preferences.getFloat("CH1_Curr_D", 0.5);
  CH2_Curr_D = preferences.getFloat("CH2_Curr_D", 0.5);

  CH1_EndDelay_W = preferences.getUInt("CH1_EndDelay_W", 10);
  CH2_EndDelay_W = preferences.getUInt("CH2_EndDelay_W", 10);
  CH1_EndDelay_D = preferences.getUInt("CH1_EndDelay_D", 10);
  CH2_EndDelay_D = preferences.getUInt("CH2_EndDelay_D", 10);

  CH1_Live = preferences.getBool("CH1_Live", true);
  CH2_Live = preferences.getBool("CH2_Live", true);

  Device_Name = Device_Name+serial_no;
  WiFi.setHostname(Device_Name.c_str());
  Serial.print("My Name Is : ");
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
  CH1_EndDelay_W *= 10000;
  CH2_EndDelay_W *= 10000;
  CH1_EndDelay_D *= 1000;
  CH2_EndDelay_D *= 1000;
  Serial.print("CH1_Curr_Wash : ");
  Serial.print(CH1_Curr_W);
  Serial.print(" CH2_Curr_Wash : ");
  Serial.println(CH2_Curr_W);

  Serial.print("CH1_Flow_Wash : ");
  Serial.print(CH1_Flow_W);
  Serial.print(" CH2_Flow_Wash : ");
  Serial.println(CH2_Flow_W);

  Serial.print("CH1_Delay_Wash : ");
  Serial.print(CH1_EndDelay_W);
  Serial.print(" CH2_Delay_Wash : ");
  Serial.println(CH2_EndDelay_W);

  Serial.print("CH1_Curr_Dry : ");
  Serial.print(CH1_Curr_D);
  Serial.print(" CH2_Curr_Dry : ");
  Serial.println(CH2_Curr_D);

  Serial.print("CH1_Delay_Dry : ");
  Serial.print(CH1_EndDelay_D);
  Serial.print(" CH2_Delay_Dry : ");
  Serial.println(CH2_EndDelay_D);

  Serial.print("CH1_Enable : ");
  Serial.print(CH1_Live);
  Serial.print(" CH2_Enable : ");
  Serial.println(CH2_Live);
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
    Serial.printf("Local IP = % s\r\n", ip.c_str());
  }
  Serial.print("MAC = ");
  Serial.println(WiFi.macAddress());
  Serial.print("SSID = ");
  Serial.println(ap_ssid);
  Serial.print("PASSWORD = ");
  Serial.print(ap_passwd);
}

// ===========================================================OTA

String reset_reason(int reason)
{
  switch (reason)
  {
    case 1 : return "POWERON_RESET";      /**<1,  Vbat power on reset*/
    case 3 : return "SW_RESET";              /**<3,  Software reset digital core*/
    case 4 : return "OWDT_RESET";            /**<4,  Legacy watch dog reset digital core*/
    case 5 : return "DEEPSLEEP_RESET";        /**<5,  Deep Sleep reset digital core*/
    case 6 : return "SDIO_RESET";             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : return "TG0WDT_SYS_RESET";       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : return "TG1WDT_SYS_RESET";       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : return "RTCWDT_SYS_RESET";       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : return "INTRUSION_RESET";       /**<10, Instrusion tested to reset CPU*/
    case 11 : return "TGWDT_CPU_RESET";       /**<11, Time Group reset CPU*/
    case 12 : return "SW_CPU_RESET";          /**<12, Software reset CPU*/
    case 13 : return "RTCWDT_CPU_RESET";      /**<13, RTC Watch dog Reset CPU*/
    case 14 : return "EXT_CPU_RESET";         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return "RTCWDT_BROWN_OUT_RESET";/**<15, Reset when the vdd voltage is not stable*/
    case 16 : return "RTCWDT_RTC_RESET";      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return "NO_MEAN";
  }
}

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, " % A, % B % d % Y % H : % M : % S");
}

String processor(const String& var)
{
  if(var == "DEVICE_NAME")
  {
    return Device_Name;
  }
  if(var == "SSID")
  {
    return ap_ssid;
  }
  if(var == "PASS")
  {
    return ap_passwd;
  }
  if(var == "RSSI")
  {
    return String(WiFi.RSSI());
  }
  if(var == "WIFI_QUALITY")
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      return "WiFi Not Connected";
    }
    int rssi = WiFi.RSSI();
    if(rssi > -40)
    {
      return "Very Good";
    }
    else if(rssi > -60)
    {
      return "Good";
    }
    else if(rssi > -70)
    {
      return "Weak";
    }
    else
    {
      return "Poor";
    }
  }
  if(var == "IP")
  {
    return WiFi.localIP().toString();
  }
  if(var == "MAC")
  {
    return WiFi.macAddress();
  }
  if(var == "TCP_STATUS")
  {
    if(client.connected())
    {
      return "Connected";
    }
    else
    {
      return "Disconnected";
    }
  }
  if(var == "FlashSize")
  {
    return String(ESP.getFlashChipSize()/1024);
  }
  if(var == "Heap")
  {
    return String(ESP.getFreeHeap()/1024);
  }
  if(var == "BUILD_VER")
  {
    return build_date;
  }
  //=======================================CH1
  if(var == "CH1_DeviceNo")
  {
    return CH1_DeviceNo;
  }
  if(var == "CH1_Live")
  {
    if(CH1_Live){
      return "Yes";
    }
    else{
      return "NO";
    }
  }
  if(var == "CH1_Mode")
  {
    if(CH1_Mode){
      return "Wash";
    }
    else{
      return "Dry";
    }
  }
  if(var == "CH1_Curr_W")
  {
    return String(CH1_Curr_W);
  }
  if(var == "CH1_Flow_W")
  {
    return String(CH1_Flow_W);
  }
  if(var == "CH1_Curr_D")
  {
    return String(CH1_Curr_D);
  }
  if(var == "CH1_EndDelay_W")
  {
    return String(CH1_EndDelay_W);
  }
  if(var == "CH1_EndDelay_D")
  {
    return String(CH1_EndDelay_D);
  }
  if(var == "Amps_TRMS1")
  {
    return String(Amps_TRMS1);
  }
  if(var == "WaterSensorData1")
  {
    return String(WaterSensorData1);
  }
  if(var == "l_hour1")
  {
    return String(l_hour1);
  }
  //=======================================CH2
  if(var == "CH2_DeviceNo")
  {
    return CH2_DeviceNo;
  }
  if(var == "CH2_Live")
  {
    if(CH2_Live){
      return "Yes";
    }
    else{
      return "NO";
    }
  }
  if(var == "CH2_Mode")
  {
    if(CH2_Mode){
      return "Wash";
    }
    else{
      return "Dry";
    }
  }
  if(var == "CH2_Curr_W")
  {
    return String(CH2_Curr_W);
  }
  if(var == "CH2_Flow_W")
  {
    return String(CH2_Flow_W);
  }
  if(var == "CH2_Curr_D")
  {
    return String(CH2_Curr_D);
  }
  if(var == "CH2_EndDelay_W")
  {
    return String(CH2_EndDelay_W);
  }
  if(var == "CH2_EndDelay_D")
  {
    return String(CH2_EndDelay_D);
  }
  if(var == "Amps_TRMS2")
  {
    return String(Amps_TRMS2);
  }
  if(var == "WaterSensorData2")
  {
    return String(WaterSensorData2);
  }
  if(var == "l_hour2")
  {
    return String(l_hour2);
  }
  return String();
}

void setupAsyncServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(auth_id.c_str(), auth_passwd.c_str()))
    {
      return request->requestAuthentication();
    }
    request->send_P(200, "text/html", manager_html, processor);
  });


  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        if (p->name() == "WiFi_SSID") {
          String ssid = p->value().c_str();
          Serial.print("SSID set to : ");
          Serial.println(ssid);
          jsonBuffer["ssid"] = ssid;
        }
        if (p->name() == "WiFi_PASS") {
          String pass = p->value().c_str();
          Serial.print("Password set to : ");
          Serial.println(pass);
          jsonBuffer["pass"] = pass;
        }
      }
    }
    request->redirect("/");
  });

  server.on("/auth", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        if (p->name() == "AUTH_ID") {
          auth_id = p->value().c_str();
          Serial.print("AUTH_ID : ");
          Serial.println(auth_id);
          if(auth_id != ""){
            putString("AUTH_ID", auth_id);
          }
        }
        if (p->name() == "AUTH_PASSWD") {
          auth_passwd = p->value().c_str();
          Serial.print("AUTH_PASSWD : ");
          Serial.println(auth_passwd);
          if(auth_passwd != ""){
            putString("AUTH_PASSWD", auth_passwd);
          }
        }
      }
    }
    request->redirect("/");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    rebooting = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", rebooting ? ok_html : failed_html);

    response->addHeader("Connection", "close");
    request->send(response);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
  {
    if(!index)
    {
      Serial.print("Updating : ");
      Serial.println(filename.c_str());

      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
      {
        Update.printError(Serial);
      }
    }
    if(!Update.hasError())
    {
      if(Update.write(data, len) != len)
      {
        Update.printError(Serial);
      }
    }
    if(final)
    {
      if(Update.end(true))
      {
        Serial.print("The update is finished : ");
        Serial.println(convertFileSize(index+len));
      }
      else
      {
        Update.printError(Serial);
      }
    }
  });
  
  server.on("/CH1", HTTP_POST, [](AsyncWebServerRequest *request) {
    String Command;
    String Value;
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        if (p->name() == "CH1") {
          Command = p->value().c_str();
          Serial.print("CH1_Commend : ");
          Serial.println(Command);
        }
        if (p->name() == "value") {
          Value = p->value().c_str();
          Serial.print("CH1_Value : ");
          Serial.println(Value);
          if(Value != ""){
            if (Command == "DeviceNo")
            {
              Serial.print("CH1_DeviceNo : ");
              Serial.println(Value);
              preferences.putString("CH1_DeviceNo", Value);
            }
            else if (Command == "Current_Wash")
            {
              Serial.print("CH1_Curr_W : ");
              Serial.println(Value);
              float var = Value.toFloat();
              preferences.putFloat("CH1_Curr_W", var);
            }
            else if (Command == "Flow_Wash")
            {
              Serial.print("CH1_Flow_W : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH1_Flow_W", var);
            }
            else if (Command == "Current_Dry")
            {
              Serial.print("CH1_Curr_D : ");
              Serial.println(Value);
              float var = Value.toFloat();
              preferences.putFloat("CH1_Curr_D", var);
            }
            else if (Command == "EndDelay_Wash")
            {
              Serial.print("CH1_EndDelay_W : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH1_EndDelay_W", var);
            }
            else if (Command == "EndDelay_Dry")
            {
              Serial.print("CH1_EndDelay_D : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH1_EndDelay_D", var);
            }
            else if (Command == "Enable")
            {
              Serial.print("CH1_Enable : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putBool("CH1_Live", var);
            }
          }
        }
      }
    }
    request->redirect("/");
  });
  
  server.on("/CH2", HTTP_POST, [](AsyncWebServerRequest *request) {
    String Command;
    String Value;
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        if (p->name() == "CH2") {
          Command = p->value().c_str();
          Serial.print("CH2_Commend : ");
          Serial.println(Command);
        }
        if (p->name() == "value") {
          Value = p->value().c_str();
          Serial.print("CH2_Value : ");
          Serial.println(Value);
          if(Value != ""){
            if (Command == "DeviceNo")
            {
              Serial.print("CH2_DeviceNo : ");
              Serial.println(Value);
              preferences.putString("CH2_DeviceNo", Value);
            }
            else if (Command == "Current_Wash")
            {
              Serial.print("CH2_Curr_W : ");
              Serial.println(Value);
              float var = Value.toFloat();
              preferences.putFloat("CH2_Curr_W", var);
            }
            else if (Command == "Flow_Wash")
            {
              Serial.print("CH2_Flow_W : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH2_Flow_W", var);
            }
            else if (Command == "Current_Dry")
            {
              Serial.print("CH2_Curr_D : ");
              Serial.println(Value);
              float var = Value.toFloat();
              preferences.putFloat("CH2_Curr_D", var);
            }
            else if (Command == "EndDelay_Wash")
            {
              Serial.print("CH2_EndDelay_W : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH2_EndDelay_W", var);
            }
            else if (Command == "EndDelay_Dry")
            {
              Serial.print("CH2_EndDelay_D : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putUInt("CH2_EndDelay_D", var);
            }
            else if (Command == "Enable")
            {
              Serial.print("CH2_Enable : ");
              Serial.println(Value);
              int var = Value.toInt();
              preferences.putBool("CH2_Live", var);
            }
          }
        }
      }
    }
    request->redirect("/");
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(auth_id.c_str(), auth_passwd.c_str()))
    {
      return request->requestAuthentication();
    }
    request->redirect("/");
    delay(200);
    ESP.restart();
  });

  server.onNotFound(notFound);

  server.begin();
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404);
}

String convertFileSize(const size_t bytes)
{
  if(bytes < 1024)
  {
    return String(bytes)+" B";
  }
  else if (bytes < 1048576)
  {
    return String(bytes/1024.0)+" kB";
  }
  else if (bytes < 1073741824)
  {
    return String(bytes/1048576.0)+" MB";
  }
}
