//#include <WiFi.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncTCP.h>
//#include "FS.h"
//#include "SPIFFS.h"
//#include <ESPmDNS.h>
//#include <Update.h>
//#include <ArduinoJson.h>
//#include "config.h"
//#include "manager_html.h"
//#include "ok_html.h"
//#include "time.h"
//
//#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
//
//AsyncWebServer server(80);
//WiFiClient client;
//DynamicJsonDocument jsonBuffer(2048);
//
//struct Config{
//  String Device_Name = "WiFiScale_";
//  String WiFi_SSID;
//  String WiFi_PASS;
//  //String WiFi_IP;
//  //String WiFi_Gateway;
//  String TCP_IP;
//  String TCP_PORT;
//  String RS232_BAUD;
//  String COMM_MODE;
//  String Last_Date;
//};
//Config config;
//
//String build_date = __DATE__ " " __TIME__;
//
//unsigned long Millis = millis();
//unsigned long previousMillis = 0;
//const char ledPin = 5;
//bool led = true;
//
//bool rebooting = false;
//
//bool wifi_enable = true;
//bool wifi_retry = true;
//
//const char* ntpServer = "kr.pool.ntp.org";
//uint8_t timeZone = 9;
//
//
//
//bool wifi_fail = 1;
//
//const char *WiFiStatusCode(wl_status_t status)
//{
//  switch (status)
//  {
//  case WL_NO_SHIELD:
//    return "WL_NO_SHIELD";
//  case WL_IDLE_STATUS:
//    return "WL_IDLE_STATUS";
//  case WL_NO_SSID_AVAIL:
//    return "WL_NO_SSID_AVAIL";
//  case WL_SCAN_COMPLETED:
//    return "WL_SCAN_COMPLETED";
//  case WL_CONNECTED:
//    return "WL_CONNECTED";
//  case WL_CONNECT_FAILED:
//    return "WL_CONNECT_FAILED";
//  case WL_CONNECTION_LOST:
//    return "WL_CONNECTION_LOST";
//  case WL_DISCONNECTED:
//    return "WL_DISCONNECTED";
//  }
//}
//
//String reset_reason(int reason)
//{
//  switch (reason)
//  {
//    case 1 : return "POWERON_RESET";      /**<1,  Vbat power on reset*/
//    case 3 : return "SW_RESET";              /**<3,  Software reset digital core*/
//    case 4 : return "OWDT_RESET";            /**<4,  Legacy watch dog reset digital core*/
//    case 5 : return "DEEPSLEEP_RESET";        /**<5,  Deep Sleep reset digital core*/
//    case 6 : return "SDIO_RESET";             /**<6,  Reset by SLC module, reset digital core*/
//    case 7 : return "TG0WDT_SYS_RESET";       /**<7,  Timer Group0 Watch dog reset digital core*/
//    case 8 : return "TG1WDT_SYS_RESET";       /**<8,  Timer Group1 Watch dog reset digital core*/
//    case 9 : return "RTCWDT_SYS_RESET";       /**<9,  RTC Watch dog Reset digital core*/
//    case 10 : return "INTRUSION_RESET";       /**<10, Instrusion tested to reset CPU*/
//    case 11 : return "TGWDT_CPU_RESET";       /**<11, Time Group reset CPU*/
//    case 12 : return "SW_CPU_RESET";          /**<12, Software reset CPU*/
//    case 13 : return "RTCWDT_CPU_RESET";      /**<13, RTC Watch dog Reset CPU*/
//    case 14 : return "EXT_CPU_RESET";         /**<14, for APP CPU, reseted by PRO CPU*/
//    case 15 : return "RTCWDT_BROWN_OUT_RESET";/**<15, Reset when the vdd voltage is not stable*/
//    case 16 : return "RTCWDT_RTC_RESET";      /**<16, RTC Watch dog reset digital core and rtc module*/
//    default : return "NO_MEAN";
//  }
//}
//
//void printLocalTime() {
//  struct tm timeinfo;
//  if(!getLocalTime(&timeinfo)){
//    Serial.println("Failed to obtain time");
//    return;
//  }
//  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//}
//
//String readFile(const char * path){
//  Serial.printf("Reading file: %s\r\n", path);
//
//  File file = SPIFFS.open(path);
//  if(!file || file.isDirectory()){
//    Serial.println("- failed to open file for reading");
//    return "Fail";
//  }
//  
//  String fileContent;
//  while(file.available()){
//    fileContent = file.readString();
//    break;     
//  }
//  file.close();
//  return fileContent;
//}
//
//void writeFile(const char * path, const char * message){
//  Serial.printf("Writing file: %s\r\n", path);
//
//  File file = SPIFFS.open(path, FILE_WRITE);
//  if(!file){
//    Serial.println("- failed to open file for writing");
//    return;
//  }
//  if(file.print(message)){
//    Serial.println("- file written");
//  } else {
//    Serial.println("- frite failed");
//  }
//  file.close();
//}
//
//void appendLogFile(const char *message)
//{
//  struct tm timeinfo;
//  if (!getLocalTime(&timeinfo))
//  {
//    Serial.println("Failed to obtain time");
//    return;
//  }
//  char YMD_Char[11];
//  char HMS_Char[9];
//  strftime(YMD_Char,11, "%F", &timeinfo);
//  strftime(HMS_Char,9, "%T", &timeinfo);
//  String YMD = String(YMD_Char);
//  String HMS = String(HMS_Char);
//  String LogTime = "[" + HMS + "] ";//로그에 찍을 시간
//  Serial.print(LogTime);
//  Serial.printf(" %s\r\n",message);
//  if(config.Last_Date != YMD)
//  {
//    SPIFFS.remove("/log/1.txt");
//    SPIFFS.rename("/log/2.txt", "/log/1.txt");
//    SPIFFS.rename("/log/3.txt", "/log/2.txt");
//    SPIFFS.rename("/log/4.txt", "/log/3.txt");
//    SPIFFS.rename("/log/5.txt", "/log/4.txt");
//    SPIFFS.rename("/log/6.txt", "/log/5.txt");
//    SPIFFS.rename("/log/7.txt", "/log/6.txt");
//    SPIFFS.rename("/log/8.txt", "/log/7.txt");
//    SPIFFS.rename("/log/9.txt", "/log/8.txt");
//    SPIFFS.rename("/log/10.txt", "/log/9.txt");
//    config.Last_Date = YMD;
//    jsonBuffer["lastdate"] = config.Last_Date;
//    writeConfig();
//  }
//  File file = SPIFFS.open("/log/10.txt", FILE_APPEND);
//  if (!file)
//  {
//    Serial.println("- failed to open file for append");
//    return;
//  }
//  file.print(LogTime);
//  if (file.println(message))
//  {
//  }
//  else
//  {
//    Serial.println("- append failed");
//  }
//  file.close();
//}
//
//void writeConfig(){
//  String output;
//  serializeJsonPretty(jsonBuffer, output);
//  writeFile("/config/config.txt", output.c_str());
//  return;
//}
//
//String processor(const String& var)
//{
//  if(var == "DEVICE_NAME")
//  {
//    return config.Device_Name;
//  }
//  if(var == "SSID")
//  {
//    return config.WiFi_SSID;
//  }
//  if(var == "PASS")
//  {
//    return config.WiFi_PASS;
//  }
//  if(var == "RSSI")
//  {
//    return String(WiFi.RSSI());
//  }
//  if(var == "WIFI_QUALITY")
//  {
//    if (WiFi.status() != WL_CONNECTED)
//    {
//      return "WiFi Not Connected";
//    }
//    int rssi = WiFi.RSSI();
//    if(rssi > -40)
//    {
//      return "Very Good";
//    }
//    else if(rssi > -60)
//    {
//      return "Good";
//    }
//    else if(rssi > -70)
//    {
//      return "Weak";
//    }
//    else
//    {
//      return "Poor";
//    }
//  }
//  if(var == "IP")
//  {
//    return WiFi.localIP().toString();
//  }
//  if(var == "MAC")
//  {
//    return WiFi.macAddress();
//  }
//  if(var == "TCP_STATUS")
//  {
//    if(client.connected())
//    {
//      return "Connected";
//    }
//    else
//    {
//      return "Disconnected";
//    }
//  }
//  if(var == "TCP_IP")
//  {
//    return config.TCP_IP;
//  }
//  if(var == "TCP_PORT")
//  {
//    return config.TCP_PORT;
//  }
//  if(var == "MODE")
//  {
//    if(config.COMM_MODE == "RS232")
//    {
//      return "RS232 Only";
//    }
//    if(config.COMM_MODE == "TCP/IP")
//    {
//      return "RS232+TCP/IP";
//    }
//  }
//  if(var == "BAUD")
//  {
//    return config.RS232_BAUD;
//  }
//  if(var == "FlashSize")
//  {
//    return String(ESP.getFlashChipSize()/1024);
//  }
//  if(var == "SPIFFS_Used")
//  {
//    return String(SPIFFS.usedBytes()/1024);
//  }
//  if(var == "SPIFFS_Total")
//  {
//    return String(SPIFFS.totalBytes()/1024);
//  }
//  if(var == "Heap")
//  {
//    return String(ESP.getFreeHeap()/1024);
//  }
//  if(var == "BUILD_VER")
//  {
//    return build_date;
//  }
//  return String();
//}
//
//void setupAsyncServer()
//{
//  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
//  {
//    if(!request->authenticate(http_username, http_password))
//    {
//      return request->requestAuthentication();
//    }
//    request->send_P(200, "text/html", manager_html, processor);
//  });
//  
//
//  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
//  int params = request->params();
//  for(int i=0;i<params;i++){
//    AsyncWebParameter* p = request->getParam(i);
//    if(p->isPost()){
//      if (p->name() == "WiFi_SSID") {
//        String ssid = p->value().c_str();
//        Serial.print("SSID set to: ");
//        Serial.println(ssid);
//        jsonBuffer["ssid"] = ssid;
//      }
//      if (p->name() == "WiFi_PASS") {
//        String pass = p->value().c_str();
//        Serial.print("Password set to: ");
//        Serial.println(pass);
//        jsonBuffer["pass"] = pass;
//      }
//    }
//  }
//  writeConfig();
//  request->redirect("/");
//  });
//
//  server.on("/tcpip", HTTP_POST, [](AsyncWebServerRequest *request) {
//  int params = request->params();
//  for(int i=0;i<params;i++){
//    AsyncWebParameter* p = request->getParam(i);
//    if(p->isPost()){
//      if (p->name() == "TCP_IP") {
//        String tcp_ip = p->value().c_str();
//        Serial.print("IP set to: ");
//        Serial.println(tcp_ip);
//        jsonBuffer["tcp_ip"] = tcp_ip;
//      }
//      if (p->name() == "TCP_PORT") {
//        String tcp_port = p->value().c_str();
//        Serial.print("Port set to: ");
//        Serial.println(tcp_port);
//        jsonBuffer["tcp_port"] = tcp_port;
//      }
//    }
//  }
//  writeConfig();
//  request->redirect("/");
//  });
//
//  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
//  {
//    rebooting = !Update.hasError();
//    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", rebooting ? ok_html : failed_html);
//
//    response->addHeader("Connection", "close");
//    request->send(response);
//  },
//  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
//  {
//    if(!index)
//    {
//      Serial.print("Updating: ");
//      Serial.println(filename.c_str());
//
//      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
//      {
//        Update.printError(Serial);
//      }
//    }
//    if(!Update.hasError())
//    {
//      if(Update.write(data, len) != len)
//      {
//        Update.printError(Serial);
//      }
//    }
//    if(final)
//    {
//      if(Update.end(true))
//      {
//        Serial.print("The update is finished: ");
//        Serial.println(convertFileSize(index + len));
//      }
//      else
//      {
//        Update.printError(Serial);
//      }
//    }
//  });
//
//  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request)
//  {
//    if(!request->authenticate(http_username, http_password))
//    {
//      return request->requestAuthentication();
//    }
//    int params = request->params();
//    for (int i = 0; i < params; i++)
//    {
//      AsyncWebParameter *p = request->getParam(i);
//      Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
//      if(p->value() == "none")
//      {
//        request->redirect("/");
//      }
//      else if(p->name() == "LogNumber")
//      {
//        String logname = "/log/" + p->value() + ".txt";
//        request->send(SPIFFS, logname, String(), true);
//      }
//    }
//  });
//
//  server.on("/baud", HTTP_POST, [](AsyncWebServerRequest *request) {
//  int params = request->params();
//  for(int i=0;i<params;i++){
//    AsyncWebParameter* p = request->getParam(i);
//    if(p->isPost()){
//      if (p->name() == "BAUD") {
//        String baudrate = p->value().c_str();
//        Serial.print("BAUD set to: ");
//        Serial.println(baudrate);
//        jsonBuffer["baud"] = baudrate;
//      }
//    }
//  }
//  writeConfig();
//  request->redirect("/");
//  });
//
//  server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
//  int params = request->params();
//  for(int i=0;i<params;i++){
//    AsyncWebParameter* p = request->getParam(i);
//    if(p->isPost()){
//      if (p->name() == "MODE") {
//        String mode = p->value().c_str();
//        Serial.print("MODE set to: ");
//        Serial.println(mode);
//        jsonBuffer["mode"] = mode;
//      }
//    }
//  }
//  writeConfig();
//  request->redirect("/");
//  });
//
//  server.on("/configfile", HTTP_GET, [](AsyncWebServerRequest *request)
//  {
//    if(!request->authenticate(http_username, http_password))
//    {
//      return request->requestAuthentication();
//    }
//    request->send(SPIFFS, "/config/config.txt", String(), true);
//  });
//
//  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
//  {
//    if(!request->authenticate(http_username, http_password))
//    {
//      return request->requestAuthentication();
//    }
//    request->redirect("/");
//    delay(200);
//    ESP.restart();
//  });
//  
//  server.onNotFound(notFound);
//
//  server.begin();
//}
//
//
//void notFound(AsyncWebServerRequest *request) 
//{
//  request->send(404);
//}
//
//String convertFileSize(const size_t bytes)
//{
//  if(bytes < 1024)
//  {
//    return String(bytes) + " B";
//  }
//  else if (bytes < 1048576)
//  {
//    return String(bytes / 1024.0) + " kB";
//  }
//  else if (bytes < 1073741824)
//  {
//    return String(bytes / 1048576.0) + " MB";
//  }
//}
//
//int setup_wifi(int wifi_interval)
//{
//  if (config.WiFi_SSID == "")
//  {
//    Serial.println("No SSID");
//    return false;
//  }
//
//  WiFi.mode(WIFI_STA);
//  WiFi.setHostname(config.Device_Name.c_str());
//  WiFi.begin(config.WiFi_SSID, config.WiFi_PASS);
//  Serial.println("Connecting to WiFi...");
//
//  unsigned long currentMillis = millis();
//  previousMillis = currentMillis;
//
//  while (WiFi.status() != WL_CONNECTED)
//  {
//    currentMillis = millis();
//    if (currentMillis - previousMillis >= wifi_interval)
//    {
//      Serial.println("WiFi Failed to connect.");
//      return 1;
//    }
//  }
//  
//  configTime(3600 * timeZone, 0, ntpServer);
//  printLocalTime();
//  appendLogFile("WiFi Connected.");
//  String IP = WiFi.localIP().toString();
//  appendLogFile(IP.c_str());
//  MDNS.begin(config.Device_Name.c_str());
//  Serial.printf("Host: http://%s.local/\n", config.Device_Name.c_str());
//  setupAsyncServer();
//
//  while(client.connect(config.TCP_IP.c_str(), config.TCP_PORT.toInt()) != 1)
//  {
//    currentMillis = millis();
//    if (currentMillis - previousMillis >= 15000)
//    {
//      Serial.println("TCP/IP Failed to connect. Retry");
//      appendLogFile("TCP/IP Failed to connect. Retry");
//    }
//  }
//
//  Serial.println(WiFi.localIP());
//  
//  return 0;
//}
//
//void setup()
//{
//  pinMode(ledPin, OUTPUT);
//  Serial.begin(115200);
//  esp_reset_reason_t rst_reason = esp_reset_reason();
//  Serial.println(reset_reason(rst_reason));
//  if(!SPIFFS.begin(true))
//  {
//    Serial.println("SPIFFS mount failed!");
//    return;
//  }
//  deserializeJson(jsonBuffer,readFile("/config/config.txt"));
//  serializeJsonPretty(jsonBuffer, Serial);
//  config.Device_Name = config.Device_Name + jsonBuffer["SN"].as<String>();
//  config.WiFi_SSID = jsonBuffer["ssid"].as<String>();
//  config.WiFi_PASS = jsonBuffer["pass"].as<String>();
//  config.TCP_IP = jsonBuffer["tcp_ip"].as<String>();
//  config.TCP_PORT = jsonBuffer["tcp_port"].as<String>();
//  config.RS232_BAUD = jsonBuffer["baud"].as<String>();
//  config.COMM_MODE = jsonBuffer["mode"].as<String>();
//  config.Last_Date = jsonBuffer["lastdate"].as<String>();
//  if(config.RS232_BAUD == "" || config.RS232_BAUD == "none" || config.RS232_BAUD == "null")
//  {
//    config.RS232_BAUD = "9600";
//  }
//  if(config.COMM_MODE == "RS232")
//  {
//    wifi_enable = false;
//  }
//  else if(config.COMM_MODE == "TCP/IP")
//  {
//    wifi_enable = true;
//  }else{
//    wifi_enable = true;
//    config.COMM_MODE = "TCP/IP";
//  }
//  File file = SPIFFS.open("/log/10.txt", FILE_APPEND);
//  file.println("[SYSTEM] BOOTUP...");
//  file.print("[SYSTEM] RESET: ");
//  file.println(reset_reason(rst_reason));
//  file.print("[SYSTEM] SN: ");
//  file.println(config.Device_Name);
//  file.print("[SYSTEM] SSID: ");
//  file.println(config.WiFi_SSID);
//  file.print("[SYSTEM] PASS: ");
//  file.println(config.WiFi_PASS);
//  file.print("[SYSTEM] TCP_IP: ");
//  file.println(config.TCP_IP);
//  file.print("[SYSTEM] TCP_PORT: ");
//  file.println(config.TCP_PORT);
//  file.print("[SYSTEM] RS232_BAUD: ");
//  file.println(config.RS232_BAUD);
//  file.print("[SYSTEM] COMM_MODE: ");
//  file.println(config.COMM_MODE);
//  file.print("[SYSTEM] Last_Date: ");//Never LOL
//  file.println(config.Last_Date);
//  file.close(); 
//  if (wifi_enable == false || setup_wifi(15000) == 1)
//  {
//    WiFi.softAP(config.Device_Name.c_str(), NULL);
//    IPAddress IP = WiFi.softAPIP();
//    Serial.print("AP IP address: ");
//    Serial.println(IP);
//    MDNS.begin(config.Device_Name.c_str());
//    Serial.printf("Host: http://%s.local/\n", config.Device_Name.c_str());
//    setupAsyncServer();
//  }
//}
//
//void loop()
//{
//  if(rebooting)
//  {
//    delay(100);
//    ESP.restart();
//  }
//
//  if (WiFi.status() != WL_CONNECTED && wifi_enable && wifi_retry)
//  {
//    appendLogFile("WiFi Lost. Retrying...");
//    WiFi.disconnect(true);
//    client.stop();
//    if (setup_wifi(60000) == 1)
//    {
//      appendLogFile("WiFi Reconect Fail. Giveup");
//      WiFi.softAP(config.Device_Name.c_str(), NULL);
//      IPAddress IP = WiFi.softAPIP();
//      Serial.print("AP IP address: ");
//      Serial.println(IP);
//      WiFi.disconnect(true);
//      wifi_retry = false;
//    }
//  }
//
//  if (client.connected() != true && wifi_enable && wifi_retry)
//  {
//    appendLogFile("TCP/IP Server Lost. Retrying...");
//    unsigned long currentMillis = millis();
//    previousMillis = currentMillis;
//    while (client.connected() != true)
//    {
//      currentMillis = millis();
//      if (currentMillis - previousMillis >= 5000)
//      {
//        client.stop();
//        Serial.println("TCP/IP Failed to connect. Retry");
//        appendLogFile("TCP/IP Failed to connect. Retry");
//        if(client.connect(config.TCP_IP.c_str(), config.TCP_PORT.toInt()) == 1)
//        {
//          break;
//        }
//      }
//    }
//  }
//
//  Millis = millis();
//  if (Millis - previousMillis >= 1500) 
//  {
//    previousMillis = Millis;
//    if(wifi_enable && client.connected())
//    {
//      Serial.println(".");
//      struct tm timeinfo;
//      if(!getLocalTime(&timeinfo)){
//        Serial.println("Failed to obtain time");
//        return;
//      }
//      client.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//      appendLogFile("KaraData");
//    }
//    
//    if(led)
//    {
//      digitalWrite(ledPin, HIGH);
//      led = false;
//    }
//    else
//    {
//      digitalWrite(ledPin, LOW);
//      led = true;
//    }
//  }
//
//}
