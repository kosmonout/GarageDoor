#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TickerScheduler.h>
#include <ArduinoJson.h>
//#include <Wire.h>
#include "SSD1306.h"

//Inputs
//D2
const int SDApin = 4;
//D1
const int SCLpin = 5;
const int DisplayInterval_ms = 2000;
const int ClearJSONInterval_ms = 3000;
const int ContactServerInterval_ms = 30000;
const int port = 80;
const char* ssid = "kosmos";
const char* password = "funhouse";
const char* incommingserver = "http://192.168.2.165/api/app/com.internet/weather";

//Start network
//ESP8266WebServer WEBserver(port);
WiFiClient client;
WiFiServer server(port);
#define addr_PCF8591 0x48 // PCF8591 bus address
#define addr_sht31 0x44 // sht31 bus address
byte ana0, ana1, ana2, ana3;
SSD1306  display(0x3c, SDApin, SCLpin);

void DisplayStatus();
void UpdateDisplay();
void ClearJSON();

bool readRequest(WiFiClient& client);
String prepareHtmlPage();
JsonObject& prepareResponse(JsonBuffer& jsonBuffer);
void writeResponse(WiFiClient& client, JsonObject& json);

int GetRequest();
bool WIFIconnected = false;
bool ClientConnected = false;
String ScreenLocation = "?";
String sJSONsendCommand;
String sJSONreceiveCommand;
TickerScheduler tsTimer(6);
bool bJSONup = false;
bool bJSONdown = false;
int temp;
float cTemp;
float fTemp;
float humidity;

////////////////////////////////////////
void setup(void)
{
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 20, 128, "Booting.....");
  display.display();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  //0 Wire.pins(SDApin,SCLpin);// just to make sure
  Wire.begin(SDApin, SCLpin); // the SDA and SCL
  WIFIconnected = false;
  // Initialising the UI will init the display too.
  display.clear();
  //Display update interval
  tsTimer.add(4, DisplayInterval_ms, UpdateDisplay, false);
  //Servercontact update interval
  tsTimer.add(0, ContactServerInterval_ms, GetRequest, false);
  //Clear JSON Line back to IP
  tsTimer.add(5, ClearJSONInterval_ms, ClearJSON, false);
}

////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  tsTimer.update();
  Wire.beginTransmission(addr_PCF8591); // wake up PCF8591
  Wire.write(0x04); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end tranmission
  Wire.requestFrom(addr_PCF8591, 5);
  while (Wire.available())   // slave may send less than requested
  {
    ana0 = Wire.read(); // throw this one away
    ana0 = Wire.read();
    ana1 = Wire.read();
    ana2 = Wire.read();
    ana3 = Wire.read();
  }
  delay(100);
  // Start I2C Transmission
  Wire.beginTransmission(addr_sht31);
  // Send 16-bit command byte
  Wire.write(0x2C);
  Wire.write(0x06);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(100);
  // Request 6 bytes of data
  Wire.requestFrom(addr_sht31, 6);

  // Read 6 bytes of data
  // temp msb, temp lsb, temp crc, hum msb, hum lsb, hum crc
  if (Wire.available() == 6)
  {
    unsigned int data[6];
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    // Convert the data
    temp = (data[0] * 256) + data[1];
    cTemp = -45.0 + (175.0 * temp / 65535.0);
    fTemp = (cTemp * 1.8) + 32.0;
    humidity = (100.0 * ((data[3] * 256.0) + data[4])) / 65535.0;
  }

  String sParsedJSON;
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
      Serial.println("Client.");
      bool success = readRequest(client);
      if (success)
      {
        String line = client.readStringUntil('\r');
        //Read JSON
        if (line.length() > 0)
        {
          // Parse JSON
          int size = line.length() + 1;
          char jsonChar[size];
          line.toCharArray(jsonChar, size);
          StaticJsonBuffer<200> jsonReadBuffer;
          JsonObject& json_parsed = jsonReadBuffer.parseObject(jsonChar);
          if (!json_parsed.success())
          {
            Serial.println("parseObject() failed");
            return;
          }
          //json_parsed.prettyPrintTo(Serial);
          String sParsedJSON = json_parsed["screen"][0];
          if (sParsedJSON == "up")
          {
            sJSONreceiveCommand = "Rcv: " + sParsedJSON;
          }
          else if (sParsedJSON == "down")
          {
            sJSONreceiveCommand = "Rcv: " + sParsedJSON;
          }
          else
          {
            sJSONreceiveCommand = "Rcv: ?";
            DisplayStatus();
          }
        }
        //Send JSON
        //delay(1000);
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
}

void ClearJSON()
{
  sJSONsendCommand = "";
  sJSONreceiveCommand = "";
}

void DisplayStatus() {
  //Remove screen update
  tsTimer.remove(4);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  if (WIFIconnected == true)
  {
    display.drawString(0, 0, "CONNECTED P: " + String(WiFi.RSSI()) + " dBm");
    if (sJSONsendCommand == "" && sJSONreceiveCommand == "")
    {
      display.drawString(0, 16, "IP: " + WiFi.localIP().toString());
    }
    else if (sJSONsendCommand != "" && sJSONreceiveCommand != "")
    {
      display.drawString(0, 16, sJSONsendCommand);
      display.drawString(72, 16, sJSONreceiveCommand);
      display.drawString(64, 16, "|");
    } else if (sJSONsendCommand != "" && sJSONreceiveCommand == "")
    {
      display.drawString(0, 16, sJSONsendCommand);
      display.drawString(64, 16, "|");
    } else if (sJSONsendCommand == "" && sJSONreceiveCommand != "")
    {
      display.drawString(72, 16, sJSONreceiveCommand);
      display.drawString(64, 16, "|");
    }
  }
  else
  {
    display.drawString(0, 0,  "DISCONNECTED");
  }
  display.drawLine(0, 32, 128, 32);
  display.drawString(0, 35, String(ana0));
  display.drawString(25, 35, String(ana1));
  display.drawString(50, 35, String(ana2));
  display.drawString(75, 35, String(ana3));
  display.drawString(0, 46, String(cTemp));
  display.drawString(60, 46, String(humidity));
  display.display();
  //Activiate screen update
  tsTimer.add(4, DisplayInterval_ms, UpdateDisplay, false);
}
int GetRequest()
{
  sJSONsendCommand = "Snd: GET";
  DisplayStatus();
  HTTPClient http;
  http.begin(incommingserver);
  int httpCode = http.GET();
  http.end();
  return httpCode;
}

JsonObject& prepareResponse(JsonBuffer & jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& screenValues = root.createNestedArray("screen");
  screenValues.add(ScreenLocation);
  sJSONsendCommand = "Snd: " + ScreenLocation;
  DisplayStatus();
  //  JsonArray& humiValues = root.createNestedArray("humidity");
  //  humiValues.add(pfHum);
  //  JsonArray& dewpValues = root.createNestedArray("dewpoint");
  //  dewpValues.add(pfDew);
  //  JsonArray& EsPvValues = root.createNestedArray("Systemv");
  //  EsPvValues.add(pfVcc / 1000, 3);
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
////////////////////////////////////////



