#define USE_MEGA     0

#include <SPI.h>
#if USE_MEGA
#include <Ethernet3.h>
#else
#include <Ethernet.h>
#endif

#define SS 10    // W5500 CS
#define RST 7    // W5500 RST
#define CS 4     // SD CS pin
const int LED = 13;

#include <math.h>
#include <SoftwareSerial.h>

//#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include "thermistor.h"

THERMISTOR thermistor(A3, 10000, 3950, 10000);
THERMISTOR thermistor2(A4, 10000, 3950, 10000);

#define INTERVAL_NUM 10 // 10초

#define BOARD_ID 1 // 수로개폐기 1
// #define BOARD_ID 2
// #define BOARD_ID 3
// #define BOARD_ID 4

#if (BOARD_ID == 1)
  #define USE_WATER_ONOFF 0 // 수위센서에 의한 모터 제어
#else
  #define USE_WATER_ONOFF 1
#endif

#define LED 8

#define LED_STATUS_OFF                // black
#define LED_STATUS_RS485_ON_WIFI_OFF  // 5 sec blink
#define LED_STATUS_RS485_OFF_WIFI_ON  // 2 sec blink
#define LED_STATUS_RS485_ON_WIFI_ON   // 1 sec blink

#define LED_STATUS_WIFI_OFF 0
#define LED_STATUS_WIFI_ON  1
#define LED_STATUS_RS485_OFF 0
#define LED_STATUS_RS485_ON  1
int ledStatusWIFI = LED_STATUS_WIFI_OFF; // black
int ledStatusRS485 = LED_STATUS_RS485_OFF; // black

#define MCP_LEDTOG1 5
#define MCP_LEDTOG2 6
#define MCP_LEDTOG3 7
#define MCP_LEDTOG4 8
#define MCP_LEDTOG5 9

//////////////////////////////////////////////////////

#define MAX_RELAY_NUM 1
boolean gpioState[MAX_RELAY_NUM] = 
{
  false, 
//  false,false, false,
//  false, false,false, false
};

char thingsboardServer[] = "thingsboard.cloud";
#if (BOARD_ID == 1)
  #define TOKEN "qddgg5n1tjj1dtJN5Tg1"
#elif (BOARD_ID == 2)
  #define TOKEN "VqfFC0nRNha9G5zTpq6R"
#elif (BOARD_ID == 3)
  #define TOKEN "OjqGZSEkp5gQMOdayh77"
#elif (BOARD_ID == 4)
  #define TOKEN "hYJwCVIqYCrPuJujZ6jC"
#endif

EthernetClient eTherClient;
PubSubClient client(eTherClient);

byte mac[] = {
  0x00, 0xAA, 0xBB, 0x83, 0x80 | BOARD_ID, 0xA0 
};

typedef struct AWS_INFO
{
  float pm10; 
  float pm25;
  float windSpeed;
  float windDirection;
  float soilTemp;
  float soilHumi;
  float soilConductivity;
  float rainGauge;
  float solarRadiation;
  float air_co2;
  float air_noise;
  float air_illumination;
  float air_humi;
  float air_temp;
  float air_kpa;
} AWS_INFO;

typedef enum GATE_COMMAND {
  GATE_COMMAND_UNDEFINED = -1, // 
  GATE_COMMAND_CLOSE,
  GATE_COMMAND_OPEN,
} GATE_COMMAND;

typedef enum GATE_STATUS {
  GATE_STATUS_UNDEFINED = -1, // 
  GATE_STATUS_CLOSED,
  GATE_STATUS_OPENED,
} GATE_STATUS;

int bAutoMode_backup = -1;
int water_onOff_backup = -1;

GATE_COMMAND gateCmd = GATE_COMMAND_UNDEFINED;

typedef struct WATER_GATE_INFO
{
  int water_encoder;
  int water_onOff;
  int st_height;
  int bWFlow;
  int bAutoMode; // 자동모드, 수동모드
  int bSwAutoMode; // 자동모드 - (자동, 수위스위치 - 수동모드)
  int rs485Timeout;
  GATE_STATUS gateStatus;
  int ntc1; // 논
  int ntc2; // 수로
  
} WATER_GATE_INFO;

#define GATE_HEIGHT_MAX 600

WATER_GATE_INFO gateInfo = {
  0,
  0,
  200,
  0,
  0,
  1,
  0,
  GATE_STATUS_UNDEFINED,
  0,
  0,
};

AWS_INFO awsInfo;

int timeout = 0;

// float adc[8] = { 0x00, }; // analog 6 + hr, hd

//#define ADC_MAX_NUM 2
//float temperature[ADC_MAX_NUM] = { 0x00, };


//Voltage initial
#define VIN 5.06
// the real value of the 'other' resistor
#define SERIESRESISTOR 10000
//#define SERIESRESISTOR 9761
//Resistence Tolerance
#define TOLERANCE 1.2

// resistance at 25 degrees C
#define THERMISTORNOOMINAL 10000
// temp. for naminal resitance (almost always 25 C)
#define TEMPERATURENOMINAL 25
//The beta coefficient of the thermistor ( usually 3000-4000 )
#define BCOEFFICIENT 3950

void readTemperature()
{
  gateInfo.ntc1 = thermistor.read();
  gateInfo.ntc2 = thermistor2.read();

  gateInfo.ntc1 -= 350;
  gateInfo.ntc2 -= 350;

//    int temperatureValue = analogRead(A3);
//  temperatureValue = 1023 - temperatureValue;
//  //Convert the analog reading (Which goes from 0 - 1023) to a valtage (0 - 5V):
//  float vout = temperatureValue * ( 5.0 / 1023 );
// // vout = 5.06 - vout;
//  Serial.print("Temp = ");
//  Serial.print(temperatureValue);
//  Serial.print(" , ");
//
//  Serial.print("vout = ");
//  Serial.print(vout);
//  Serial.print(" , ");
//  //vout = 2.5;
//  // Resistance of thermistor
//  float Rt = ( SERIESRESISTOR *(VIN-vout) )/vout;
//  Serial.print("Rt = ");
//  Serial.print(Rt);
//  Serial.print(" , ");
//  
//
//  //Ucertainty Calculation
//  float Rmin = Rt + (( Rt * TOLERANCE )/100);
//  float Rmax = Rt - (( Rt * TOLERANCE )/100);
//
//  float Tmeasure;
//  Tmeasure = Rt / THERMISTORNOOMINAL; //(R/Ro)
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
//  Tmeasure = log(Tmeasure); //  ln(R/Ro)
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
//  Tmeasure /= BCOEFFICIENT; // 1/B * ln(R/Ro)
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
//  Tmeasure += 1.0 / ( TEMPERATURENOMINAL + 273.15 ); // +(1/tO)
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
//  Tmeasure = 1.0 / Tmeasure; //Invert
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
//  Tmeasure -= 273.15; // convert to C
//
//  Serial.print("Tmeasure = ");
//  Serial.print(Tmeasure);
//  Serial.print(" , ");
  
}

void readAnalog() {
  gateInfo.water_encoder = analogRead(A0); // GATE_HEIGHT_MAX-analogRead(A0);
  gateInfo.water_onOff = (100 < analogRead(A1)) ? 1 : 0;
  
  gateInfo.bWFlow = (10 < analogRead(A2)) ? 0 : 1;
  gateInfo.bAutoMode = (100 < analogRead(A5)) ? 0 : 1;

  readTemperature();

  
//  Serial.print(gateInfo.water_encoder);
//  Serial.print(',');
//  Serial.print(gateInfo.water_onOff);
//  Serial.print(',');
//  Serial.print(gateInfo.bWFlow);
//  Serial.print(',');
//  Serial.print(gateInfo.bAutoMode);
//  Serial.print(',');
//  Serial.print(gateInfo.gateStatus);
//  Serial.print(',');
//  Serial.print(gateInfo.st_height);
//  Serial.print(',');
//  Serial.print(gateInfo.rs485Timeout);
//  Serial.print(',');
//  Serial.print(gateInfo.ntc1);
//  Serial.print(',');
//  Serial.print(gateInfo.ntc2);

//  Serial.println();
}

void doorOpen()
{
  if(GATE_STATUS_OPENED == gateInfo.gateStatus) {
    Serial.println(F("ALREADY OPEN"));
    return;
  }
  
  Serial.println(F("OPENNING"));
  digitalWrite(MCP_LEDTOG1, HIGH);
  digitalWrite(MCP_LEDTOG2, LOW);
  delay(15*1000);
  digitalWrite(MCP_LEDTOG1, LOW);
  digitalWrite(MCP_LEDTOG2, LOW);
  
  Serial.println(F("FINISHED"));

  gateInfo.gateStatus = GATE_STATUS_OPENED;
  
}

void doorClose()
{
  if(GATE_STATUS_CLOSED == gateInfo.gateStatus) {
    Serial.println(F("ALREADY CLOSE"));
    return;
  }
  
  Serial.println(F("CLOSING"));
//        Serial.println("CLOSE");
  digitalWrite(MCP_LEDTOG1, LOW);
  digitalWrite(MCP_LEDTOG2, HIGH);
  delay(15*1000);
  digitalWrite(MCP_LEDTOG1, LOW);
  digitalWrite(MCP_LEDTOG2, LOW);
  Serial.println(F("FINISHED"));

  gateInfo.gateStatus = GATE_STATUS_CLOSED;  
}

String json_parser(String s, String a) { String val; if (s.indexOf(a) != -1) { int st_index = s.indexOf(a); int val_index = s.indexOf(':', st_index); if (s.charAt(val_index + 1) == '"') { int ed_index = s.indexOf('"', val_index + 2); val = s.substring(val_index + 2, ed_index); } else { int ed_index = s.indexOf(',', val_index + 1); val = s.substring(val_index + 1, ed_index); } } else { Serial.print(a); Serial.println(F(" is not available")); } return val; }

// char sData[32];

void on_message(const char* topic, byte* payload, unsigned int length) {

  // char json[length + 1];
  char * json = (char *)malloc(length + 1);
  char * sData = (char *)malloc(32);

//  Serial.println("On message");
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  String methodName = json_parser(json, "method");
  
  if (methodName.equals("checkStatus")) {
//    Serial.print("bAutoMode=");
//    Serial.println(bAutoMode);

    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    sprintf(sData, "{\"value\" : %d }", gateInfo.bAutoMode ? "true" : "false");
    client.publish(responseTopic.c_str(), sData);
    free(sData);
  }

  // Decode JSON request
//  StaticJsonBuffer <128> jsonBuffer;
//  JsonObject& data = jsonBuffer.parseObject((char*)json);
//
//  if (!data.success())
//  {
//    Serial.println("parseObject() failed");
//    return;
//  }
//
//  // Check request method
//  String methodName = String((const char*)data["method"]);
//
//  if (methodName.equals("checkStatus")) {
//
//    Serial.print("bAutoMode=");
//    Serial.println(bAutoMode);
//    
//    // Reply with GPIO status
//      String responseTopic = String(topic);
//      responseTopic.replace("request", "response");
//    if(bAutoMode == 1) {
//      client.publish(responseTopic.c_str(), get_auto_status(0).c_str());
//    }
//    else {
//      client.publish(responseTopic.c_str(), "");
//    }
//  } 
//
  else
  if (methodName.equals(F("getValue"))) {

//    Serial.print(F("RELAY="));
//    Serial.println(gpioState[0]);
    
    // Reply with GPIO status
      String responseTopic = String(topic);
      responseTopic.replace("request", "response");
    if(gpioState[0] == true) {
      //client.publish(responseTopic.c_str(), get_gpio_status(0).c_str());
      sprintf(sData, "{\"params\" : %s }", gpioState[0] ? "true" : "false");
      client.publish(responseTopic.c_str(), sData);      
    }
    else {
      client.publish(responseTopic.c_str(), "");
    }
  } 
  else
  if (methodName.equals(F("getValueLimit"))) {

    // Reply with GPIO status
      String responseTopic = String(topic);
      responseTopic.replace("request", "response");
      
      sprintf(sData, "{\"params\" : %d.%d }", 
        (int)gateInfo.st_height,
        (int)(gateInfo.st_height*100)%100
        );
      client.publish(responseTopic.c_str(), sData);      
  } 

  else
  if (methodName.equals(F("setValueLimit"))) {

    String params = json_parser(json, "params");
    char * buf = (char *)malloc(8);
    params.toCharArray(buf, 8); // len);
    gateInfo.st_height = atof(buf);
    free(buf);
    Serial.print(F("gateInfo.st_height="));
    Serial.println(gateInfo.st_height);
    
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
//    client.publish(responseTopic.c_str(), get_gpio_status(0).c_str());
    sprintf(sData, "{\"params\" : %d.%d }", 
      (int)gateInfo.st_height,
      (int)(gateInfo.st_height*100)%100
      );
    client.publish(responseTopic.c_str(), sData);      
    client.publish("v1/devices/me/attributes", sData);

    gateInfo.bSwAutoMode = 1;
    EEPROM.write(0, (int)gateInfo.st_height);

    
  }
  else
  if (methodName.equals(F("setValue"))) {

    String params = json_parser(json, "params");
    // set_gpio_status(1, data["params"]);

//    Serial.print("params.equals(\"true\")=");
//    Serial.println(params);
    char buf[8];
    params.toCharArray(buf, 8); // len);
//    Serial.println(buf);
    if(memcmp(buf, "true", 4) == 0) gpioState[0] = 1;
    else gpioState[0] = 0;
//    Serial.println(gpioState[0]);
    
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
//    client.publish(responseTopic.c_str(), get_gpio_status(0).c_str());
    // char sData[32];
    sprintf(sData, "{\"params\" : %s }", gpioState[0] ? "true" : "false");
    client.publish(responseTopic.c_str(), sData);      
    client.publish("v1/devices/me/attributes", sData);

    gateInfo.bSwAutoMode = 0;
    if(gpioState[0] == 1) {
//      doorOpen();
      gateCmd = GATE_COMMAND_OPEN;
    }
    else {
//      doorClose();
      gateCmd = GATE_COMMAND_CLOSE;
    }
  }

  free(json);
  free(sData);

}

SoftwareSerial rx485_SW(3, 2); // 2,3);  // 2차보드
int EN_SW = 4;

 //const PROGMEM 
 char hexmap[][8] = 
{
  // Master Query [7] C8C5,C8C5
  { 0x1,0x3,0x0,0x0,0x0,0x6,0xC5,0xC8}, 
  // Master Query [11] FBC5,FBC5
  { 0x2,0x3,0x0,0x0,0x0,0x6,0xC5,0xFB}, 
  // Master Query [8] 2AC4,2AC4
  { 0x3,0x3,0x0,0x0,0x0,0x6,0xC4,0x2A}, 
  // Master Query [9] 9DC5,9DC5
  { 0x4,0x3,0x0,0x0,0x0,0x6,0xC5,0x9D}, 
  // Master Query [1] 9F25,9F25
  { 0x4,0x3,0x0,0x2,0x0,0x1,0x25,0x9F}, 
  // Master Query [12] 4CC4,4CC4
  { 0x5,0x3,0x0,0x0,0x0,0x6,0xC4,0x4C}, 
  // Master Query [2] 7FC4,7FC4
  { 0x6,0x3,0x0,0x0,0x0,0x6,0xC4,0x7F}, 
  // Master Query [3] AEC5,AEC5
  { 0x7,0x3,0x0,0x0,0x0,0x6,0xC5,0xAE}, 
  // Master Query [6] A0E5,A0E5
  { 0x7,0x3,0x1,0xFA,0x0,0x2,0xE5,0xA0}, 
  { 0x7,0x3,0x1,0xF6,0x0,0x4, 0x00, 0x00 },
  
  // Master Query [4] 51C5,51C5
  { 0x8,0x3,0x0,0x0,0x0,0x6,0xC5,0x51}, 
  // Master Query [10] 80C4,80C4
  { 0x9,0x3,0x0,0x0,0x0,0x6,0xC4,0x80}, 
  // Master Query [5] B3C4,B3C4
  { 0xA,0x3,0x0,0x0,0x0,0x6,0xC4,0xB3}, 
};

int hexmap_pos = 0;
#define HEXMAP_NUM (sizeof(hexmap) / sizeof(hexmap[0]))

#define POLYNORMIAL 0xA001;
unsigned short CRC16(unsigned char * puchMsg, int usDataLen)
{
  int i;
  unsigned short crc, flag;
  crc = 0xFFFF;
  while(usDataLen--)
  {
    crc ^= *puchMsg++;
    for(i = 0; i < 8; i++)
    {
      flag = crc & 0x0001;
      crc >>= 1;
      if(flag) crc ^= POLYNORMIAL;
    }
  }

  return crc;
}


void RS485_send()
{
  digitalWrite(EN_SW, 1);

//  Serial.println(F("RS485_send"));

  unsigned short crc = CRC16(hexmap[hexmap_pos], 6);

  hexmap[hexmap_pos][6] = 0xFF & crc;
  hexmap[hexmap_pos][7] = 0xFF & (crc >> 8);
  
  rx485_SW.write(hexmap[hexmap_pos], 8); // sizeof(hexmap[hexmap_pos]));
  
  hexmap_pos += 1; 
  if(HEXMAP_NUM <= hexmap_pos) hexmap_pos = 0;
  
  rx485_SW.flush();

  digitalWrite(EN_SW, 0);
}

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0


void setup() {

#if USE_MEGA

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(SS, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(SS, HIGH);
  digitalWrite(CS, HIGH);
  /* If you want to control Reset function of W5500 Ethernet controller */
  digitalWrite(RST, HIGH);

//  pinMode(LED, OUTPUT);
//  digitalWrite(LED, HIGH);

#endif  

  pinMode(A5, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("START");

  rx485_SW.begin(4800);
  pinMode(EN_SW, OUTPUT);
  digitalWrite(EN_SW, 0); // Transmitter MODE ON


  if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      // no point in carrying on, so do nothing forevermore:
      //for (;;)
      //  ;
    } 

  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  client.setServer( thingsboardServer, 1883 );
  client.setCallback(on_message);

  Serial.println();
  
  pinMode(MCP_LEDTOG1, OUTPUT);  // Toggle LED 1
  pinMode(MCP_LEDTOG2, OUTPUT);  // Toggle LED 2
  pinMode(MCP_LEDTOG3, OUTPUT);  // Toggle LED 3
  pinMode(MCP_LEDTOG4, OUTPUT);  // Toggle LED 1
  pinMode(MCP_LEDTOG5, OUTPUT);  // Toggle LED 2

  gateInfo.st_height = EEPROM.read(0);

  water_onOff_backup = EEPROM.read(1);
  
}

int reconnect_timeout = 0;
void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Connecting to ThingsBoard node ..."));
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
      Serial.println( F("[DONE]") );

      // Subscribing to receive RPC requests
      client.subscribe("v1/devices/me/rpc/request/+");

      ledStatusWIFI = LED_STATUS_WIFI_ON;
      
      reconnect_timeout = 0;
    } 
    else {

      
      Serial.print( F("[FAILED] [ rc = ") );
      Serial.print( client.state() );
      Serial.println( F(" : retrying in 5 seconds]") );
      // Wait 5 seconds before retrying

      ledStatusWIFI = LED_STATUS_WIFI_OFF;

      reconnect_timeout += 1;
      if(4 < reconnect_timeout) {

        resetFunc(); //call reset
        reconnect_timeout = 0;
        
      }
      delay( 4000 );
    }
  }
}

#define DATA_NUM 64
//unsigned char data[DATA_NUM] = { 0x00, };

#define PACKET_NUM 64
//unsigned char packet[PACKET_NUM] = { 0x00, };

char sTelemetry[] = "v1/devices/me/telemetry";

void loop() {
  // put your main code here, to run repeatedly:
  if ( !client.connected() ) {
    reconnect();
  }

  client.loop();

  if(timeout % 10 == 0) {
    readAnalog();
  }

  unsigned char * data = (unsigned char *)malloc(DATA_NUM);
  unsigned char * packet = (unsigned char *)malloc(DATA_NUM);

  if(timeout % 10 == 0) {
    RS485_send();
  }

  memset(data, 0x00, DATA_NUM);
  
  int pos = 0;
  while(0 < rx485_SW.available()) {
    
    unsigned char val = rx485_SW.read(); 
    data[pos] = val; pos += 1;
//    Serial.print(val, HEX); Serial.print(','); // TEST
    delay(1);

    if(DATA_NUM-20 < pos) break;
  
  }  

  int pi = 0;
  int di = 0;

  while(di < pos)
  {

    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    packet[pi++] = data[di++]; if(di > pos) break;
    
    int len = packet[2];
    
    if(len <= 2 /* || bFirst == true*/) { // query
      unsigned short crc = ((unsigned short)packet[7] << 8 | (unsigned short)packet[6]);
      pi = 0;
    }
    else {
      for(int j = 5; j < len + 2; j++)
      {
        packet[pi++] = data[di++];  if(di > pos) break;   
      }
      
      if(di > pos) break;

      
      unsigned short crc = ((unsigned short)packet[3 + len + 1] << 8 | (unsigned short)packet[3 + len]);
      if(CRC16(packet, 3 + len) == crc) {

        digitalWrite(LED, true);

        gateInfo.rs485Timeout = 0;
        
  #if 1      
        #if (DEBUG_PRINT_BINARY == 1)
        
          packet[pi++] = 0x0d;
          packet[pi++] = 0x0a;
          Serial.write(packet, pi);
        
        #elif 1

          if (packet[0] == 0x01) // air quality
          {
              awsInfo.pm10 = (float)(packet[3] * 0x100 + packet[4]);
              awsInfo.pm25 = (float)(packet[5] * 0x100 + packet[6]);
//                  Serial.print("awsInfo.pm10=");
//                  Serial.println(awsInfo.pm10);
//                  Serial.print("awsInfo.pm25=");
//                  Serial.println(awsInfo.pm25);
              
          }
          if (packet[0] == 0x02) // wind speed
          {
              awsInfo.windSpeed = ((float)(packet[3] * 0x100 + packet[4]) / 10);
          }
          if (packet[0] == 0x03) // wind direction
          {
              awsInfo.windDirection = (float)(packet[5] * 0x100 + packet[6]);
          }
          if (packet[0] == 0x04) // soil
          {
              awsInfo.soilTemp = ((float)(packet[5] * 0x100 + packet[6])/10);
              
              awsInfo.soilHumi = ((float)(packet[3] * 0x100 + packet[4])/10);
              if(100 < awsInfo.soilHumi) awsInfo.soilHumi = 100;
//              Serial.print("awsInfo.soilHumi=");
//              Serial.println(awsInfo.soilHumi);
              awsInfo.soilConductivity = ((float)(packet[7] * 0x100 + packet[8]) );
          }
          if (packet[0] == 0x05) // rain
          {
              awsInfo.rainGauge = (packet[5] * 0x100 + packet[6]);
          }
          if (packet[0] == 0x06) // solar
          {
              awsInfo.solarRadiation = (packet[3] * 0x100 + packet[4]);
          }
          if (packet[0] == 0x07) // CO2
          {
              // byte[] temp = { 0x7, 0x3, 0x1, 0xFA, 0x0, 0x2};
              // 07 03 04 00 00 01 42 1D 92 0D 0A Illumination

              // byte[] temp = { 0x7, 0x3, 0x0, 0x0, 0x0, 0x6 }; //, 0xC5, 0xAE };
              // 07 03 0C 01 9F 00 B2 04 01 00 00 00 00 00 00 18 33 0D 0A humi, temp, kpa

              // byte[] temp = { 0x7, 0x3, 0x1, 0xF6, 0x0, 0x4 }; //, 0xC5, 0xAE };
              // 07 03 08 01 A3 05 48 00 00 04 01 FA C2 0D 0A noise, co2  
              if (packet[2] == 4)
              {
                  awsInfo.air_illumination = ((float)(packet[5] * 0x100 + packet[6]) / 1);
//                  Serial.print("awsInfo.air_illumination=");
//                  Serial.println(awsInfo.air_illumination);
              }
              else
              if (packet[2] == 8)
              {
                  awsInfo.air_noise = ((float)(packet[3] * 0x100 + packet[4]) / 10);
                  awsInfo.air_co2 = ((float)(packet[5] * 0x100 + packet[6]) / 1);
//                  Serial.print("awsInfo.air_co2=");
//                  Serial.println(awsInfo.air_co2);                        
              }
              else // 0x0c
              { 
                  awsInfo.air_humi = ((float)(packet[3] * 0x100 + packet[4]) / 10);
                  awsInfo.air_temp = ((float)(packet[5] * 0x100 + packet[6]) / 10);
                  awsInfo.air_kpa = ((float)(packet[7] * 0x100 + packet[8]) / 10);
//                  Serial.print("awsInfo.air_humi=");
//                  Serial.println(awsInfo.air_humi);
//                  Serial.print("awsInfo.air_temp=");
//                  Serial.println(awsInfo.air_temp);
                  
              }
          }
          if (packet[0] == 0x08) // 
          {
              //textBox8.Text = ((float)(packet[3] * 0x100 + packet[4]) / 10);
          }
          if (packet[0] == 0x09) // 
          {
              //textBox9.Text = (packet[5] * 0x100 + packet[6]);
          }
          if (packet[0] == 0x0A) // 
          {
              //textBoxA.Text = (packet[5] * 0x100 + packet[6]);
          }     

//          Serial.print("Slave Data "); Serial.print(CRC16(packet, 3 + len), HEX); Serial.print(","); Serial.print(crc, HEX);      
//          Serial.println();     
//
//          for(int i = 0; i < pi; i++) {
//             Serial.print(packet[i], HEX); Serial.print(',');
//          }
//          Serial.println();
        #endif

        // delay(100);

        
  #endif      

      }
      pi = 0;
    }
  }

  free(data);
  free(packet);

  delay(100);

        digitalWrite(LED, false);

//  Serial.print("timeout=");
//  Serial.println(timeout); 
  timeout += 1;
  gateInfo.rs485Timeout += 1;

  if(timeout % (INTERVAL_NUM * 10) == 0) { // 1min

//    Serial.println(F("SendToServer"));

    // char data[64] = { 0x00, };
    memset(data, 0x00, DATA_NUM);
    sprintf(data, "{\"wHeight\":%d, \"wOnOff\":%d, \"bAutoMode\":%d, \"bWFlow\":%d}", 
       gateInfo.water_encoder, 
       gateInfo.water_onOff, 
       gateInfo.bAutoMode,
       gateInfo.bWFlow);
//    Serial.println(   data);
    client.publish(sTelemetry, data);

    memset(data, 0x00, DATA_NUM);
    sprintf(data, "{\"pm10\":%d.%d, \"pm25\":%d.%d }", 
       (int)awsInfo.pm10, (int)(awsInfo.pm10*100)%100,
       (int)awsInfo.pm25, (int)(awsInfo.pm25*100)%100 );
//    Serial.println(   data);
    client.publish(sTelemetry, data);    

    memset(data, 0x00, DATA_NUM);
    sprintf(data, "{\"ntc1\":%d.%d, \"ntc2\":%d.%d }", 
       gateInfo.ntc1/10, (gateInfo.ntc1%10),
       gateInfo.ntc2/10, (gateInfo.ntc2%10) );
//    Serial.println(   data);
    client.publish(sTelemetry, data);      


    //    tb.sendTelemetryFloat("air_humi", awsInfo.air_humi);
    //    tb.sendTelemetryFloat("air_temp", awsInfo.air_temp);
    //    tb.sendTelemetryFloat("air_kpa", awsInfo.air_kpa );
    //    tb.sendTelemetryFloat("pm10", awsInfo.pm10 );
    //    tb.sendTelemetryFloat("pm25", awsInfo.pm25 );
    //    tb.sendTelemetryFloat("windSpeed", awsInfo.windSpeed );
    //    tb.sendTelemetryFloat("windDirection", awsInfo.windDirection );
    //    tb.sendTelemetryFloat("soilTemp", awsInfo.soilTemp );
    //    tb.sendTelemetryFloat("soilHumi", awsInfo.soilHumi );
    //    tb.sendTelemetryFloat("soilConductivity", awsInfo.soilConductivity );
    //    tb.sendTelemetryFloat("rainGauge", awsInfo.rainGauge );
    //    tb.sendTelemetryFloat("solarRadiation", awsInfo.solarRadiation );
    //    tb.sendTelemetryFloat("air_co2", awsInfo.air_co2 );
    //    tb.sendTelemetryFloat("air_noise", awsInfo.air_noise );
    //    tb.sendTelemetryFloat("air_illumination", awsInfo.air_illumination );
  }


//typedef struct WATER_GATE_INFO
//{
//  int water_encoder;
//  int water_onOff;
//  int st_height;
//  int ad2;
//  int bAutoMode;
//  GATE_STATUS gateStatus;
//  
//} WATER_GATE_INFO;

  // 수문 높이 조절
  if(timeout % 10 == 0) {

    if(bAutoMode_backup != gateInfo.bAutoMode) {
      gateInfo.gateStatus = GATE_STATUS_UNDEFINED;
      bAutoMode_backup = gateInfo.bAutoMode;
    }
    
    if(gateInfo.bAutoMode == 1)
    {
       // Open
  
//       if(gateInfo.water_encoder < gateInfo.st_height)
//       {
//          doorOpen();
//       }
//  
//       // close
//       if(gateInfo.st_height < gateInfo.water_encoder)
//       {
//          doorClose();
//       }    

#if USE_WATER_ONOFF 
       if(water_onOff_backup != gateInfo.water_onOff) {

         EEPROM.write(1, (int)gateInfo.water_onOff);
         
         if(gateInfo.water_onOff == 0)
         {
            doorOpen();
         }
           // close
         else
         if(gateInfo.water_onOff == 1)
         {
            doorClose();
         }
         water_onOff_backup = gateInfo.water_onOff;

         
       }
#endif       

       if(gateCmd != GATE_COMMAND_UNDEFINED)
       {
         if(gateCmd == GATE_COMMAND_OPEN)
         {
            doorOpen();

            
         }
           // close
         else
         if(gateCmd == GATE_COMMAND_CLOSE)
         {
            doorClose();
         }

         gateCmd = GATE_COMMAND_UNDEFINED;
       }
    }
    else {
      digitalWrite(MCP_LEDTOG1, LOW);
      digitalWrite(MCP_LEDTOG2, LOW);
    }
  }

}
