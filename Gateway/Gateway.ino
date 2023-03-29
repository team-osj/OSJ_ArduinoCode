#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <SPI.h>
#include "SPIFFS.h"
#include "RF24.h"

AsyncWebServer server(80);
RF24 radio(4, 5); // CE = 4, SS = 5

uint8_t address1_uint[6] = "00001";
uint8_t address2_uint[6] = "10002";

const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";
const char *PARAM_INPUT_5 = "subnet";
const char *PARAM_INPUT_6 = "address1";
const char *PARAM_INPUT_7 = "address2";
const char *PARAM_INPUT_8 = "address3";
const char *PARAM_INPUT_9 = "address4";
const char *PARAM_INPUT_10 = "address5";
const char *PARAM_INPUT_11 = "address6";
const char *PARAM_INPUT_12 = "address7";
const char *PARAM_INPUT_13 = "address8";

String ssid;
String pass;
String ip;
String gateway;
String netmask;
String address1;
String address2;
String address3;
String address4;
String address5;
String address6;
String address7;
String address8;

const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";
const char *subnetPath = "/subnet.txt";
const char *addr1Path = "/address1.txt";
const char *addr2Path = "/address2.txt";
const char *addr3Path = "/address3.txt";
const char *addr4Path = "/address4.txt";
const char *addr5Path = "/address5.txt";
const char *addr6Path = "/address6.txt";
const char *addr7Path = "/address7.txt";
const char *addr8Path = "/address8.txt";

IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;

String WIFI_hostname = "ESPGateway_1";
char Server_domain[] = "osj.pnxelec.com";
int Server_port = 443;
SocketIOclient socketIO;
#define USE_SERIAL Serial

unsigned long previousMillis = 0;
const long interval = 10000; // 와이파이 연결 대기 시간

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      {
        char *sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
        if (id)
        {
          payload = (uint8_t *)sptr;
        }
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error)
        {
          USE_SERIAL.print(F("deserializeJson() failed: "));
          USE_SERIAL.println(error.c_str());
          return;
        }

        String eventName = doc[0];
        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        // Message Includes a ID for a ACK (callback)
        if (id)
        {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();

          // add payload (parameters) for the ack (callback function)
          JsonObject param1 = array.createNestedObject();
          param1["now"] = millis();

          // JSON to String (serializion)
          String output;
          output += id;
          serializeJson(docOut, output);

          // Send event
          socketIO.send(sIOtype_ACK, output);
        }
      }
      break;
    case sIOtype_ACK:
      USE_SERIAL.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      USE_SERIAL.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      USE_SERIAL.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}

void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    USE_SERIAL.println("An error has occurred while mounting SPIFFS");
  }
  USE_SERIAL.println("SPIFFS mounted successfully");
}

String readFile(fs::FS &fs, const char *path)
{
  USE_SERIAL.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    USE_SERIAL.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  USE_SERIAL.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    USE_SERIAL.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    USE_SERIAL.println("- file written");
  }
  else
  {
    USE_SERIAL.println("- frite failed");
  }
}

bool initWiFi()
{
  WiFi.mode(WIFI_STA);
  USE_SERIAL.println(WIFI_hostname);
  if (ssid == "")
  {
    USE_SERIAL.println("Undefined SSID.");
    return false;
  }

  if (ip == "" || gateway == "")
  {
    USE_SERIAL.println("IP and Gateway not defined.DHCP");
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  }
  else
  {
    USE_SERIAL.println("Static IP defined.");
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());
    localSubnet.fromString(netmask.c_str());
    WiFi.config(localIP, localGateway, localSubnet);
  }

  if (!WiFi.setHostname(WIFI_hostname.c_str()))
  {
    USE_SERIAL.println("Hostname failed to configure");
  }
  else
  {
    USE_SERIAL.println("Hostname configure");
  }

  WiFi.begin(ssid.c_str(), pass.c_str());
  USE_SERIAL.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      USE_SERIAL.println("Failed to connect.");
      return false;
    }
  }

  USE_SERIAL.println(WiFi.localIP());
  return true;
}

void SettingPage()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/wifimanager.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          USE_SERIAL.print("SSID set to: ");
          USE_SERIAL.println(ssid);
          // Write file to save value
          writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          USE_SERIAL.print("Password set to: ");
          USE_SERIAL.println(pass);
          // Write file to save value
          writeFile(SPIFFS, passPath, pass.c_str());
        }
        // HTTP POST ip value
        if (p->name() == PARAM_INPUT_3) {
          ip = p->value().c_str();
          USE_SERIAL.print("IP Address set to: ");
          USE_SERIAL.println(ip);
          // Write file to save value
          writeFile(SPIFFS, ipPath, ip.c_str());
        }
        // HTTP POST gateway value
        if (p->name() == PARAM_INPUT_4) {
          gateway = p->value().c_str();
          USE_SERIAL.print("Gateway set to: ");
          USE_SERIAL.println(gateway);
          // Write file to save value
          writeFile(SPIFFS, gatewayPath, gateway.c_str());
        }
        // HTTP POST subnet value
        if (p->name() == PARAM_INPUT_5) {
          netmask = p->value().c_str();
          USE_SERIAL.print("Subnet set to: ");
          USE_SERIAL.println(netmask);
          // Write file to save value
          writeFile(SPIFFS, subnetPath, netmask.c_str());
        }

        // HTTP POST address1 value
        if (p->name() == PARAM_INPUT_6) {
          address1 = p->value().c_str();
          USE_SERIAL.print("address1 set to: ");
          USE_SERIAL.println(address1);
          // Write file to save value
          writeFile(SPIFFS, addr1Path, address1.c_str());
        }
        // HTTP POST address2 value
        if (p->name() == PARAM_INPUT_7) {
          address2 = p->value().c_str();
          USE_SERIAL.print("address2 set to: ");
          USE_SERIAL.println(address2);
          // Write file to save value
          writeFile(SPIFFS, addr2Path, address2.c_str());
        }
        // HTTP POST address3 value
        if (p->name() == PARAM_INPUT_8) {
          address3 = p->value().c_str();
          USE_SERIAL.print("address3 set to: ");
          USE_SERIAL.println(address3);
          // Write file to save value
          writeFile(SPIFFS, addr3Path, address3.c_str());
        }
        // HTTP POST address4 value
        if (p->name() == PARAM_INPUT_9) {
          address4 = p->value().c_str();
          USE_SERIAL.print("address4 set to: ");
          USE_SERIAL.println(address4);
          // Write file to save value
          writeFile(SPIFFS, addr4Path, address4.c_str());
        }
        // HTTP POST address5 value
        if (p->name() == PARAM_INPUT_10) {
          address5 = p->value().c_str();
          USE_SERIAL.print("address5 set to: ");
          USE_SERIAL.println(address5);
          // Write file to save value
          writeFile(SPIFFS, addr5Path, address5.c_str());
        }
        // HTTP POST address6 value
        if (p->name() == PARAM_INPUT_11) {
          address6 = p->value().c_str();
          USE_SERIAL.print("address6 set to: ");
          USE_SERIAL.println(address6);
          // Write file to save value
          writeFile(SPIFFS, addr6Path, address6.c_str());
        }
        // HTTP POST address7 value
        if (p->name() == PARAM_INPUT_12) {
          address7 = p->value().c_str();
          USE_SERIAL.print("address7 set to: ");
          USE_SERIAL.println(address7);
          // Write file to save value
          writeFile(SPIFFS, addr7Path, address7.c_str());
        }
        // HTTP POST address8 value
        if (p->name() == PARAM_INPUT_13) {
          address8 = p->value().c_str();
          USE_SERIAL.print("address8 set to: ");
          USE_SERIAL.println(address8);
          // Write file to save value
          writeFile(SPIFFS, addr8Path, address8.c_str());
        }
        //USE_SERIAL.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
    delay(3000);
    ESP.restart();
  });
  server.begin();
}

void setup()
{
  radio.begin();
  Serial.begin(115200);
  radio.begin(); // 아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(0, address1_uint); // 파이프 주소 넘버 ,저장할 파이프 주소
  radio.openReadingPipe(1, address2_uint);
  radio.startListening();
  USE_SERIAL.setDebugOutput(true);
  initSPIFFS();
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile(SPIFFS, gatewayPath);
  netmask = readFile(SPIFFS, subnetPath);

  address1 = readFile(SPIFFS, addr1Path);
  address2 = readFile(SPIFFS, addr2Path);
  address3 = readFile(SPIFFS, addr3Path);
  address4 = readFile(SPIFFS, addr4Path);
  address5 = readFile(SPIFFS, addr5Path);
  address6 = readFile(SPIFFS, addr6Path);
  address7 = readFile(SPIFFS, addr7Path);
  address8 = readFile(SPIFFS, addr8Path);
  if (address1 == "" || address2 == "" || address3 == "" || address4 == "" || address5 == "" || address6 == "" || address7 == "" || address8 == "")
  {
    USE_SERIAL.println("Pipe Address not defined.Default value.");
  }
  USE_SERIAL.println(ssid);
  USE_SERIAL.println(pass);
  USE_SERIAL.println(ip);
  USE_SERIAL.println(gateway);
  USE_SERIAL.println(netmask);

  USE_SERIAL.println(address1);
  USE_SERIAL.println(address2);
  USE_SERIAL.println(address3);
  USE_SERIAL.println(address4);
  USE_SERIAL.println(address5);
  USE_SERIAL.println(address6);
  USE_SERIAL.println(address7);
  USE_SERIAL.println(address8);

  if (initWiFi())
  {
    SettingPage();
  }
  else
  {
    // Connect to Wi-Fi network with SSID and password
    USE_SERIAL.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESPGateway_1", NULL);

    IPAddress IP = WiFi.softAPIP();
    USE_SERIAL.print("AP IP address: ");
    USE_SERIAL.println(IP);

    SettingPage();
  }
  socketIO.beginSSL(Server_domain, Server_port, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void update_state(int device_id, int updated_state, int alive)
{
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  array.add("update_state"); // event name
  JsonObject jsondata = array.createNestedObject();
  jsondata["id"] = device_id;
  jsondata["state"] = updated_state;
  jsondata["alive"] = alive;
  String JSONdata;
  serializeJson(doc, JSONdata);
  socketIO.sendEVENT(JSONdata);
  USE_SERIAL.println(JSONdata);
  return;
}

void loop()
{
  socketIO.loop();
  byte pipe;
  if (radio.available(&pipe))
  {
    char text[30];
    radio.read(text, sizeof(text));
    while (radio.available(&pipe))
    {
      char trash_value[30];
      radio.read(trash_value, sizeof(trash_value));
    }
    int onOff = text[0] - 49;
    String Data(&text[1]);
    int data;
    data = Data.toInt();
    USE_SERIAL.println(data);
    update_state(data, onOff, 1);
    USE_SERIAL.print("onOff : ");
    USE_SERIAL.println(onOff);
    USE_SERIAL.print("data : ");
    USE_SERIAL.println(data);
    /*switch (data)
      {
      case 1:
      update_state(1, 1, 1); // 1번 디바이스 사용가능
      break;
      case 2:
      update_state(1, 0, 1); // 1번 디바이스 사용불가
      break;
      case 3:
      update_state(2, 1, 1); // 2번 디바이스 사용가능
      break;
      case 4:
      update_state(2, 0, 1); // 2번 디바이스 사용불가
      break;
      }
      USE_SERIAL.println("haha");
      radio.stopListening();
      radio.startListening();
      }*/
    /*if(USE_SERIAL.available()){
      update_state(1,USE_SERIAL.parseInt(),1);
      }*/
  }
}
