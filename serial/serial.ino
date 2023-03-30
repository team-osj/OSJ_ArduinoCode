#include <Filters.h>
#include <SPI.h>
#include <EEPROM.h>
#include "RF24.h"

String inString;
int dex, dex1, dexc, end;

RF24 radio(8, 10); //CE, SS
uint8_t address1[6] = "00001"; //송신 주소
char text;

void setup() {
  byte HIByte = EEPROM.read(1); // read(주소)
  byte LOByte = EEPROM.read(2); // read(주소)
  int EPR = word(HIByte, LOByte);
  String EPRS = String(EPR);
  for (int i = 0; i < 5; i++)  address1[i] = EPRS[i];
  address1[5] = '\0';

  Serial.begin(9600);
  radio.begin(); //아두이노-RF모듈간 통신라인
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(address1); //송신하는 주소
  radio.stopListening();
  Serial.print("adress : ");
  Serial.println(EPR);
  byte number = EEPROM.read(3);
  int numberi = int(number);
  Serial.print("number : ");
  Serial.println(number);
    
}

void loop() {
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
      address1[i] = eprs[i];
      Serial.print(address1[i]);
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
        int text = 9;
        radio.write(&text, sizeof(text));
        Serial.println("1");      //대충 센서값 넣는곳
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
    //대충 통신하는 내용
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
    EEPROM.write(3, numberi);
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
