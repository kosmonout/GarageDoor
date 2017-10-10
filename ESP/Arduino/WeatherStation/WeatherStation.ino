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
#define  PIN_SDA 4
//D1
#define PIN_SCL 5
//D0
#define PIN_HEATERON 16
//D5
#define PIN_POWERWIND 14


#define DISPLAY_INTERVAL_MS 3000
#define CLEAR_JSON_INTERVAL_MS 3000
#define CONTACT_SERVER_INTERVAL_MS 20000
#define UPDATE_SENSOR_MS 5000
#define CHECK_BLYNK_MS 1000
#define HTTP_PORT 80
#define HEATER_ON_MOISTURE 75
#define HEATER_OFF_MOISTURE 70
#define SSID "kosmos"
#define PASSWORD "funhouse"
#define BLYNK_AUTH "63fb3008df63415784b2284c087c64bd"
#define INCOMMING_SERVER "http://192.168.2.165/api/app/com.internet/weather"

//Start network
WiFiClient client;
WiFiServer server(HTTP_PORT);

#define ADDR_SHT31 0x44 // sht31 bus address Moisture/Temp
#define ADDR_SSD1306 0x3c // SSD1306 bus address Display
#define ADDR_MAX44009 0x4a // MAX44009 bus address Light lux
#define WIND_ALARM 20 // 
#define COUT_WIND_ALARM 50

SSD1306  display(ADDR_SSD1306, PIN_SDA, PIN_SCL);
BMP280 bmp(PIN_SDA, PIN_SCL);
SHT31 sht31(PIN_SDA, PIN_SCL);
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
void UpdateBlynk();

uint8_t crc8(const uint8_t *data, uint8_t len);

bool readRequest(WiFiClient& client);
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
bool bTemperatureAvail;
bool bHumidityAvail;
bool bPressureAvail;
bool bLightLuxAvail;
bool bWindPowerAvail;
bool bWindDirectionAvail;
bool bRainAvail;
boolean bRain;
int iAlarmCount;
int iBlynkUpdateScreen = 1;
boolean bWAlarm = false;
String sJSONsendCommand;
TickerScheduler tsTimer(4);

////////////////////////////////////////
void setup(void)
{
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, "Booting.....");
  display.display();
  pinMode(PIN_HEATERON, OUTPUT);
  pinMode(PIN_POWERWIND, OUTPUT);
  digitalWrite(PIN_HEATERON, LOW);
  digitalWrite(PIN_POWERWIND, LOW);
  Serial.begin(115200);
  WiFi.disconnect();
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Start");
  ads.begin();
  if (ads.readADC_SingleEnded(0) == 0xFFFF)
  {
    display.drawStringMaxWidth(0, 16, 128, "NOT found AD conv");
  }
  else
  {
    display.drawStringMaxWidth(0, 16, 128, "Found AD conv");
    ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  }

  if (!bmp.begin())
  {
    display.drawStringMaxWidth(0, 28, 128, "NOT found Presure sensor");
  }
  else
  {
    display.drawStringMaxWidth(0, 28, 128, "Found Presure sensor");
  }
  display.display();

  sht31.begin(ADDR_SHT31);
  uint32_t serialNo;
  if ( !sht31.readSerialNo(serialNo))
  {
    {
      display.drawStringMaxWidth(0, 40, 128, "NO I2C SHT31");
    }
  }
  else
  {
    if (!sht31.readTempHum())
    {
      display.drawStringMaxWidth(0, 40, 128, "NO VDD SHT31");
    }
    else
    {
      display.drawStringMaxWidth(0, 40, 128, "Found SHT31");
    }
  }

  display.display();
  dLightLux = myLux.getLux(); //dummy read to generate an error
  if (myLux.getError() != 0)
  {
    display.drawStringMaxWidth(0, 52, 128, "NOT found MAX77009");
  }
  else
  {
    display.drawStringMaxWidth(0, 52, 128, "Found MAX77009");
  }
  display.display();

  Blynk.config(BLYNK_AUTH);
  Blynk.connect(10);
  Wire.begin(PIN_SDA, PIN_SCL); // the SDA and SCL
  WIFIconnected = false;
  // Initialising the UI will init the display too.
  display.clear();
  //Display update interval
  tsTimer.add(0, DISPLAY_INTERVAL_MS, UpdateDisplay, false);
  //Servercontact update interval
  tsTimer.add(1, CONTACT_SERVER_INTERVAL_MS, GetRequest, false);
  //UpdateSensors
  tsTimer.add(2, UPDATE_SENSOR_MS, UpdateSensors, false);
  //Check blynk status and reconnect
  tsTimer.add(3, CHECK_BLYNK_MS, checkBlynk, false);
  UpdateSensors();
  Blynk.run();
  checkBlynk();
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
  digitalWrite(PIN_POWERWIND, HIGH);
  dTemperature = sht31.readTemperature();
  if (dTemperature < -40 || dTemperature > 125 )
  {
    bTemperatureAvail = false;
  } else
  {
    bTemperatureAvail = true;
  }
  dHumidity = sht31.readHumidity();
  if (dHumidity < 0 || dHumidity > 100)
  {
    //If we do not know the moisture level put the heater on
    digitalWrite(PIN_HEATERON, HIGH);
    bHumidityAvail = false;
  } 
  else
  {
    //Put the heater if the moisture level is too high
    if (dHumidity > HEATER_ON_MOISTURE)
    {
      digitalWrite(PIN_HEATERON, HIGH);
    }
    else if (dHumidity < HEATER_OFF_MOISTURE)
    {
      digitalWrite(PIN_HEATERON, LOW);
    }
    bHumidityAvail = true;
  }
  
  dPressure = bmp.readPressure() / 100;
  if (dPressure >= 1200 || dPressure <= 900)
  {
    bPressureAvail = false;
  } else
  {
    bPressureAvail = true;
  }
  dLightLux = myLux.getLux();
  if (myLux.getError() != 0)
  {
    bLightLuxAvail = false;
  } else
  {
    bLightLuxAvail = true;
  }
  if (ads.readADC_SingleEnded(0) == 0xFFFF)
  {
    bWindPowerAvail = false;
  }
  else
  {
    bWindPowerAvail = true;
  }
  dWindPower = 12.0;
  if (ads.readADC_SingleEnded(1) == 0xFFFF)
  {
    bWindDirectionAvail = false;
  }
  else
  {
    bWindDirectionAvail = true;
  }
  sWindDirection = "NO";
 digitalWrite(PIN_POWERWIND, LOW);
  
  if (ads.readADC_SingleEnded(2) == 0xFFFF)
  {
    bRainAvail = false;
  }
  else
  {
    bRainAvail = true;
  }
  bRain = false;
  if ((dWindPower > WIND_ALARM || bRain == true) && bWAlarm == false )
  {
    bWAlarm = true;
    iAlarmCount = COUT_WIND_ALARM;
  } else if (iAlarmCount == 0 && bRain == false)
  {
    bWAlarm = false;
  }

  if (iAlarmCount >= 0)
  {
    iAlarmCount--;
  }
}

void UpdateBlynk()
{
  //Upload to Blynk
  //In steps othewise the device will be loose connection with to many virtualwrites at once.

  switch (iBlynkUpdateScreen)
  {
    case 1:
      if (bTemperatureAvail)
      {
        Blynk.virtualWrite(V1, dTemperature);
        Blynk.setProperty(V1, "color", BLYNK_GREEN);
      }
      else
      {
        Blynk.virtualWrite(V1, "error");
        Blynk.setProperty(V1, "color", BLYNK_RED);
      }
      break;
    case 2:
      if (bHumidityAvail)
      {
        Blynk.setProperty(V2, "color", BLYNK_GREEN);
        Blynk.virtualWrite(V2, dHumidity);
      }
      else
      {
        Blynk.setProperty(V2, "color", BLYNK_RED);
        Blynk.virtualWrite(V2, "error");
      }
      break;
    case 3:
      if (bPressureAvail)
      {
        Blynk.setProperty(V3, "color", BLYNK_GREEN);
        Blynk.virtualWrite(V3, dPressure);
      }
      else
      {
        Blynk.setProperty(V3, "color", BLYNK_RED);
        Blynk.virtualWrite(V3, "error");
      }
      break;
    case 4:
      if (bLightLuxAvail)
      {
        Blynk.setProperty(V4, "color", BLYNK_GREEN);
        Blynk.virtualWrite(V4, dLightLux);
      }
      else
      {
        Blynk.setProperty(V4, "color", BLYNK_RED);
        Blynk.virtualWrite(V4, "error");
      }
      break;
    case 5:
      if (bWindDirectionAvail)
      {
        Blynk.setProperty(V6, "color", BLYNK_GREEN);
        Blynk.virtualWrite(V6, sWindDirection);
      }
      else
      {
        Blynk.setProperty(V6, "color", BLYNK_RED);
        Blynk.virtualWrite(V6, "error");
      }
      break;
    case 6:
      if (bWindPowerAvail)
      {
        Blynk.setProperty(V5, "color", BLYNK_GREEN);
        Blynk.virtualWrite(V5, dWindPower);
      }
      else
      {
        Blynk.setProperty(V5, "color", BLYNK_RED);
        Blynk.virtualWrite(V5, "error");
      }
      break;
    case 7:
      if (bRainAvail)
      {
        if (bRain == true)
        {
          if (bRain) {
            Blynk.setProperty(V7, "color", BLYNK_GREEN);
            Blynk.virtualWrite(V7, "Detected");
          }
          else
          {
            Blynk.setProperty(V7, "color", BLYNK_GREEN);
            Blynk.virtualWrite(V7, "None");
          }
        }
      }
      else
      {
        Blynk.setProperty(V7, "color", BLYNK_RED);
        Blynk.virtualWrite(V7, "ERROR");
      }
      break;
    case 8:
      if (bWAlarm == true)
      {
        Blynk.virtualWrite(V8, "Triggered");
      }
      else
      {
        Blynk.virtualWrite(V8, "None");
      }
      break;
  }

  iBlynkUpdateScreen++;
  if (iBlynkUpdateScreen > 8) iBlynkUpdateScreen = 1;
}

void DisplayStatus() {
  //Remove screen update
  tsTimer.remove(4);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (WIFIconnected == true)
  {
    display.drawString(0, 0, "CONNECTED");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, String(WiFi.RSSI()) + " dBm");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (sJSONsendCommand == "")
    {
      display.drawString(0, 11, "IP: " + WiFi.localIP().toString());

      if (Blynk.connected())
      {
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(128, 11, "BnkCON");
        display.setTextAlignment(TEXT_ALIGN_LEFT);
      }
      else
      {
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(128, 11, "BnkDIS");
        display.setTextAlignment(TEXT_ALIGN_LEFT);
      }
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
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  switch (iDisplay)
  {
    case 1:
      display.drawString(31, 30, "Temp [ºC]");
      display.drawString(95, 30, "Moist [%]");
      if (bTemperatureAvail)
      {
        display.drawString(31, 47, String(dTemperature));
      }
      else
      {
        display.drawString(31, 47, "ERROR");
      }
      if (bHumidityAvail)
      {
        display.drawString(95, 47, String(dHumidity));
      }
      else
      {
        display.drawString(95, 47, "ERROR");
      }
      break;
    case 2:
      display.drawString(31, 30, "Pres [mBar]");
      display.drawString(95, 30, "Light [%]");
      if (bPressureAvail)
      {
        display.drawString(31, 47, String(double(dPressure)));
      }
      else
      {
        display.drawString(31, 47, "ERROR");
      }
      if (bLightLuxAvail)
      {
        display.drawString(95, 47, String(dLightLux));
      }
      else
      {
        display.drawString(95, 47, "ERROR");
      }
      break;
    case 3:
      display.drawString(31, 30, "WndP [km/h]");
      display.drawString(95, 30, "WndDir");
      if (bWindPowerAvail)
      {
        display.drawString(31, 47, String(dWindPower));
      }
      else
      {
        display.drawString(31, 47, "ERROR");
      }
      if (bWindDirectionAvail)
      {
        display.drawString(95, 47, sWindDirection);
      }
      else
      {
        display.drawString(95, 47, "ERROR");
      }
      break;
    case 4:
      display.drawString(31, 30, "Rain");
      display.drawString(95, 30, "Alarm");
      if (bRainAvail)
      {
        if (bRain) {
          display.drawString(31, 47, String("Detected"));
        }
        else
        {
          display.drawString(31, 47, String("None"));
        }
      }
      else
      {
        display.drawString(31, 47, "ERROR");
      }
      if (bWAlarm) {
        display.drawString(95, 47, String("Triggered"));
      }
      else
      {
        display.drawString(95, 47, String("None"));
      }
      break;
  }
  display.display();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
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
  if (bTemperatureAvail)
  {
    root["dTempOut"] = String(dTemperature);
  }
  else
  {
    root["dTempOut"] = String(-999);
  }
  if (bHumidityAvail)
  {
    root["dHumidityOut"] = String(dHumidity);
  }
  else
  {
    root["dHumidityOut"] = String(-999);
  }
  if (bPressureAvail)
  {
    root["dPressureOut"] = String(dPressure);
  }
  else
  {
    root["dPressureOut"] = String(-999);
  }
  if (bLightLuxAvail)
  {
    root["dLightLuxOut"] = String(dLightLux);
  }
  else
  {
    root["dLightLuxOut"] = String(-999);
  }
  if (bWindDirectionAvail)
  {
    root["sWindDirection"] = sWindDirection;
  }
  else
  {
    root["sWindDirection"] = "ERROR";
  }
  if (bWindPowerAvail)
  {
    root["dWindPower"] = String(dWindPower);
  }
  else
  {
    root["dWindPower"] = String(-999);
  }
  if (bRainAvail)
  {
    if (bRain == true)
    {
      root["bRain"] = "Detected";
    }
    else
    {
      root["bRain"] = "None";
    }
  }
  else
  {
    root["bRain"] = "ERROR";
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

bool checkBlynk()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Blynk.connected() == false) {
      // Blynk.connect();
    }
    else
    {
      UpdateBlynk();
    }
  }
}




