#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TickerScheduler.h>
#include <ArduinoJson.h>
#include "SSD1306.h"
#include "BMP280.h"
#include "SHT31.h"
#include "MAX44009.h"
#include "ADS1015.h"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED       "#D3435C"
#define BLYNK_DARK_BLUE "#5F7CD8"
#include <BlynkSimpleEsp8266.h>
//Inputs
//D2
#define  SDApin 4
//D1
#define SCLpin 5
#define DISPLAY_INTERVAL_MS 3000
#define CLEAR_JSON_INTERVAL_MS 3000
#define CONTACT_SERVER_INTERVAL_MS 20000
#define UPDATE_SENSOR_MS 5000
#define HTTP_PORT 80
#define SSID "kosmos"
#define PASSWORD "funhouse"
#define BLYNK_AUTH "63fb3008df63415784b2284c087c64bd"
#define INCOMMING_SERVER "http://192.168.2.165/api/app/com.internet/weather"



//Start network
WiFiClient client;
WiFiServer server(HTTP_PORT);

#define ADDR_SHT31 0x44 // sht31 bus address
#define ADDR_SSD1306 0x3c // PCF8591 bus address
#define ADDR_MAX44009 0xCB // PCF8591 bus address
#define WIND_ALARM 20 // PCF8591 bus address
#define COUT_WIND_ALARM 50

SSD1306  display(ADDR_SSD1306, SDApin, SCLpin);
BMP280 bmp(SDApin, SCLpin);
SHT31 sht31(SDApin, SCLpin);
Max44009 myLux(ADDR_MAX44009);
ADS1115 ads;   

char readBytes(unsigned char *values, char length);
char writeBytes(unsigned char *values, char length);
char readInt(char address, int16_t &value);
char readUInt(char address, uint16_t &value);
void DisplayStatus();
void UpdateDisplay();
void UpdateSensors();
void ClearJSON();
uint8_t crc8(const uint8_t *data, uint8_t len);

bool readRequest(WiFiClient& client);
String prepareHtmlPage();
JsonObject& prepareResponse(JsonBuffer& jsonBuffer);
void writeResponse(WiFiClient& client, JsonObject& json);

int GetRequest();
int iDisplay = 1;
bool WIFIconnected = false;
bool ClientConnected = false;
double dTemperature;
double dHumidity;
double dPressure;
double dLightLux;
double dWindPower;
String sWindDirection;
boolean bRain;
int iAlarmCount;
boolean bWAlarm = false;
String sJSONsendCommand;
TickerScheduler tsTimer(3);

////////////////////////////////////////
void setup(void)
{
  Serial.begin(115200);
  Serial.println("Start");
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, "Booting.....");
  display.display();
  display.setFont(ArialMT_Plain_10);
  
  if (!bmp.begin()) 
  {
    display.drawStringMaxWidth(0, 28, 128,"NOT found BMP280 sensor");
  }
  else
  {
    display.drawStringMaxWidth(0, 28, 128,"Found BMP280 sensor");
  }
  display.display();
  if (! sht31.begin(ADDR_SHT31)) 
  {  
    display.drawStringMaxWidth(0, 40, 128,"NOT found SHT31");
  }
  else
  {   
    display.drawStringMaxWidth(0, 40, 128,"Found SHT31");
  }
  display.display();
  dLightLux = myLux.getLux(); //dummy read to generate an error
  if (myLux.getError() != 0) 
  {   
    display.drawStringMaxWidth(0, 52, 128,"NOT found MAX77009");
  }
  else
  {
   display.drawStringMaxWidth(0, 52, 128,"Found MAX77009");  
  }
  display.display();
  Blynk.begin(BLYNK_AUTH, SSID, PASSWORD);

  Serial.begin(115200);
  //WiFi.begin(SSID, PASSWORD);
  Wire.begin(SDApin, SCLpin); // the SDA and SCL
  WIFIconnected = false;
  // Initialising the UI will init the display too.
  display.clear();
  //Display update interval
  tsTimer.add(0, DISPLAY_INTERVAL_MS, UpdateDisplay, false);
  //Servercontact update interval
  tsTimer.add(1, CONTACT_SERVER_INTERVAL_MS, GetRequest, false);
  //UpdateSensors
  tsTimer.add(2, UPDATE_SENSOR_MS, UpdateSensors, false);
  UpdateSensors();
}

////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  Blynk.run();
  tsTimer.update();
  // Check if module is still connected to WiFi.
  if (WiFi.status() != WL_CONNECTED)
  {
    if (WIFIconnected == true)
    {
      Serial.println("Wifi disconnected");
      server.close();
    }
    WIFIconnected = false;
    DisplayStatus();
  }
  else
  {
    // Print the new IP to Serial.
    if (WIFIconnected == false)
    {
      server.begin();
      WIFIconnected = true;
      DisplayStatus();
    }
    WiFiClient client = server.available();
    if (client)
    {
      bool success = readRequest(client);
      if (success)
      {
        //Send JSON
        StaticJsonBuffer<200> jsonWriteBuffer;
        JsonObject& jsonWrite = prepareResponse(jsonWriteBuffer);
        writeResponse(client, jsonWrite);
        ClientConnected = true;
        delay(1);
        client.stop();
      }
    }
  }
}

//#####################################################################################################

void UpdateDisplay()
{
  DisplayStatus();
  iDisplay++;
  if (iDisplay > 4) iDisplay = 1;
}

void UpdateSensors()
{
  dTemperature = sht31.readTemperature();
  dHumidity = sht31.readHumidity();
  dPressure = bmp.readPressure() / 100;
  dLightLux = myLux.getLux();
  dWindPower = 12.0;
  sWindDirection = "NO";
  bRain = false;
  if ((dWindPower > WIND_ALARM || bRain == true) && bWAlarm == false )
  {
    bWAlarm = true;
    iAlarmCount = COUT_WIND_ALARM;
  } else if (iAlarmCount == 0 && bRain == false)
  {
    bWAlarm = false;
  }

  //Upload to Blynk
  Blynk.virtualWrite(V1, dTemperature);
  Blynk.setProperty(V1, "color", BLYNK_DARK_BLUE);
  Blynk.virtualWrite(V2, dHumidity);
  Blynk.virtualWrite(V3, dPressure);
  Blynk.virtualWrite(V4, dLightLux);
  Blynk.virtualWrite(V5, dWindPower);
  Blynk.virtualWrite(V6, sWindDirection);
  if (bRain) {
    Blynk.virtualWrite(V7, "Detected");
  }
  else
  {
    Blynk.virtualWrite(V7, "None");
  }
  if (bWAlarm) {
    Blynk.virtualWrite(V8, "Triggered");
  }
  else
  {
    Blynk.virtualWrite(V8, "None");
  }

  if (iAlarmCount >= 0)
  {
    iAlarmCount--;
  }
}

void DisplayStatus() {
  //Remove screen update
  tsTimer.remove(4);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  if (WIFIconnected == true)
  {
    display.drawString(0, 0, "CONNECTED P: " + String(WiFi.RSSI()) + " dBm");
    if (sJSONsendCommand == "")
    {
      display.drawString(0, 11, "IP: " + WiFi.localIP().toString());
    }
    else
    {
      display.drawString(0, 11, sJSONsendCommand);
      sJSONsendCommand = "";
    }
  }
  else
  {
    display.drawString(0, 0,  "DISCONNECTED");
  }
  display.drawLine(63, 23, 63, 64);
  display.drawLine(0, 23, 128, 23);
  switch (iDisplay)
  {
    case 1:
      display.drawString(0, 26, "Temp [ºC]");
      display.drawString(68, 26, "Moist [%]");
      display.drawString(0, 45, String(dTemperature));
      display.drawString(68, 45, String(dHumidity));
      break;
    case 2:
      display.drawString(0, 26, "Pres [mBar]");
      display.drawString(68, 26, "Light [%]");
      display.drawString(0, 45, String(double(dPressure)));
      display.drawString(68, 45, String(dLightLux));
      break;
    case 3:
      display.drawString(0, 26, "WndP [km/h]");
      display.drawString(68, 26, "WndDir");
      display.drawString(0, 45, String(dWindPower));
      display.drawString(68, 45, sWindDirection);
      break;
    case 4:
      display.drawString(0, 26, "Rain");
      display.drawString(68, 26, "Alarm");
      if (bRain) {
        display.drawString(0, 45, String("Detected"));
      }
      else
      {
        display.drawString(0, 45, String("None"));
      }
      if (bWAlarm) {
        display.drawString(68, 45, String("Triggered"));
      }
      else
      {
        display.drawString(68, 45, String("None"));
      }
      break;
  }
  display.display();
  //Activiate screen update
  tsTimer.add(0, DISPLAY_INTERVAL_MS, UpdateDisplay, false);
}
int GetRequest()
{
  sJSONsendCommand = "Snd: GET";
  DisplayStatus();
  HTTPClient http;
  http.begin(INCOMMING_SERVER);
  int httpCode = http.GET();
  http.end();
  return httpCode;
}

JsonObject& prepareResponse(JsonBuffer & jsonBuffer)
{
  JsonObject& root = jsonBuffer.createObject();
  root["dTempOut"] = String(dTemperature);
  root["dHumidityOut"] = String(dHumidity);
  root["dPressureOut"] = String(dPressure);
  root["dLightLuxOut"] = String(dLightLux);
  root["sWindDirection"] = sWindDirection;
  root["dWindPower"] = String(dWindPower);
  if (bRain == true)
  {
    root["bRain"] = "Detected";
  }
  else
  {
    root["bRain"] = "None";
  }
  if (bWAlarm == true)
  {
    root["bWAlarm"] = "Triggered";
  }
  else
  {
    root["bWAlarm"] = "None";
  }
  sJSONsendCommand = "Snd Sensor status";
  DisplayStatus();
  return root;
}

void writeResponse(WiFiClient & client, JsonObject & json) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  json.prettyPrintTo(client);
}

bool readRequest(WiFiClient & client) {
  bool currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n' && currentLineIsBlank) {
        Serial.println("Client Request Confirmed.");
        return true;
      } else if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }
  return false;
}



