#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TickerScheduler.h>
#include <ArduinoJson.h>
//#include <Wire.h>
#include "SSD1306.h"


//Inputs
//D6
const int ButtonUpValuePin = 12;
//D5
const int ButtonDownValuePin = 14;
//D7
const int RelayUpPin = 13;
//D0
const int RelayDownPin = 16;
//D2
const int SDApin = 4;
//D1
const int SCLpin = 5;
//How much time does it take to go up and down
const int SunScreenActive_ms = 5000;
//After the motor has being stopped it needs some time to avoid overstessing it
const int SunStopTime_ms = 2500;
const int ButtonDelay_ms = 400;
const int DisplayInterval_ms = 2000;
const int ContactServerInterval_ms = 30000;
const int port = 80;
const char* ssid = "kosmos";
const char* password = "funhouse";
const char* incommingserver = "http://192.168.2.165/api/app/com.internet/screen";

int ButtonUpValue;
int ButtonDownValue;
bool GoingUp = false;
bool GoingDown = false;
bool IsDown = false;
bool IsUp = false;

//Start network
//ESP8266WebServer WEBserver(port);
WiFiClient client;
WiFiServer server(port);
SSD1306  display(0x3c, SDApin, SCLpin);

void DisplayStatus();
void ScreenGoingUp();
void ScreenGoingDown();
void ScreenStop();
void StopTimeReached();
void ScreenIsUp();
void ScreenIsDown();
void ButtonDelay();
void UpdateDisplay();

bool readRequest(WiFiClient& client);
String prepareHtmlPage();
JsonObject& prepareResponse(JsonBuffer& jsonBuffer);
void writeResponse(WiFiClient& client, JsonObject& json);

int GetRequest();
bool WIFIconnected = false;
bool ClientConnected = false;
bool bDebounceReached = true;
String ScreenLocation = "unknown";
String sJSONScreenCommand;
TickerScheduler tsTimer(5);
bool bJSONup = false;
bool bJSONdown = false;

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
  WIFIconnected = false;
  pinMode(RelayUpPin, OUTPUT);
  pinMode (RelayDownPin, OUTPUT);
  digitalWrite(RelayUpPin, HIGH);
  digitalWrite(RelayDownPin, HIGH);
  pinMode (ButtonUpValuePin, INPUT_PULLUP);
  pinMode (ButtonDownValuePin, INPUT_PULLUP);
  // Initialising the UI will init the display too.
  display.clear();
  //Send screen up as starting point
  ScreenGoingUp();
  //Display update interval
  tsTimer.add(4, DisplayInterval_ms, UpdateDisplay, false);
  //Servercontact update interval
  tsTimer.add(0, ContactServerInterval_ms, GetRequest, false);

}

////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  tsTimer.update();
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
          //          Serial.println("");
          //          Serial.print("*");
          //          Serial.print(jsonChar);
          //          Serial.print("*");
          //          Serial.println("");
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
            sJSONScreenCommand = "C:" + sParsedJSON;
            bJSONup = true;
            bJSONdown = false;
            //ScreenGoingUp();
          }
          else if (sParsedJSON == "down")
          {
            sJSONScreenCommand = "R:" + sParsedJSON;
            bJSONup = false;
            bJSONdown = true;
            //ScreenGoingDown();
          }
          else
          {
            sJSONScreenCommand = "C:?";
            DisplayStatus();
          }

        }
        //Send JSON
        delay(1000);
        StaticJsonBuffer<200> jsonWriteBuffer;
        JsonObject& jsonWrite = prepareResponse(jsonWriteBuffer);
        writeResponse(client, jsonWrite);
        ClientConnected = true;
        delay(1);
        client.stop();
      }
    }
  }
  ButtonUpValue = digitalRead(ButtonUpValuePin);
  ButtonDownValue = digitalRead(ButtonDownValuePin);
  if (  ((ButtonDownValue == HIGH && ButtonUpValue == LOW) || bJSONup == true) && GoingUp == false && GoingDown == false && bDebounceReached == true)
  {
    //Screen send to high
    bDebounceReached = false;
    ScreenGoingUp();
    sJSONScreenCommand = "";
    tsTimer.add(3, ButtonDelay_ms, ButtonDelay, false);
  }
  else if ( ((ButtonDownValue == LOW && ButtonUpValue == HIGH) || bJSONdown == true) && GoingUp == false && GoingDown == false && bDebounceReached == true)
  {
    //Screen send to low
    bDebounceReached = false;
    ScreenGoingDown();
    sJSONScreenCommand = "";
    tsTimer.add(3, ButtonDelay_ms, ButtonDelay, false);
  }
  else if ( GoingUp == true && GoingDown == true)
  {
    //When hold do nothing
  }
  else if ( (((bJSONdown == true || ButtonDownValue == LOW) && GoingUp == true) || ((bJSONup == true || ButtonUpValue == LOW) && GoingDown == true)) && bDebounceReached == true)
  {
    bDebounceReached = false;
    ScreenStop();
    tsTimer.add(3, ButtonDelay_ms, ButtonDelay, false);
  }
  bJSONup = false;
  bJSONdown = false;
}

//#####################################################################################################
void ButtonDelay()
{
  bDebounceReached = true;
}

void UpdateDisplay()
{
  DisplayStatus();
}

void ScreenIsUp() {
  GoingUp = false;
  GoingUp = false;
  tsTimer.remove(1);
  ScreenLocation = "high";
  digitalWrite(RelayUpPin, HIGH);
  digitalWrite(RelayDownPin, HIGH);
  DisplayStatus();
  tsTimer.add(0, ContactServerInterval_ms, GetRequest, false);
}

void ScreenIsDown()
{
  GoingDown = false;
  GoingUp = false;
  tsTimer.remove(1);
  ScreenLocation = "low";
  digitalWrite(RelayUpPin, HIGH);
  digitalWrite(RelayDownPin, HIGH);
  DisplayStatus();
  tsTimer.add(0, ContactServerInterval_ms, GetRequest, false);
}

void StopTimeReached()
{
  tsTimer.remove(2);
  ScreenLocation = "unknown";
  GoingDown = false;
  GoingUp = false;
  DisplayStatus();
  tsTimer.add(0, ContactServerInterval_ms, GetRequest, false);
}

void ScreenGoingUp()
{
  tsTimer.remove(0);
  GoingDown = false;
  GoingUp = true;
  ScreenLocation = "goingup";
  digitalWrite(RelayUpPin, LOW);
  digitalWrite(RelayDownPin, HIGH);
  DisplayStatus();
  tsTimer.remove(1);
  tsTimer.add(1, SunScreenActive_ms, ScreenIsUp, false);
}

void ScreenStop()
{
  //Prevent Going Up or Down until timer has reached
  GoingDown = true;
  GoingUp = true;
  tsTimer.remove(0);
  ScreenLocation = "hold";
  digitalWrite(RelayUpPin, HIGH);
  digitalWrite(RelayDownPin, HIGH);
  DisplayStatus();
  tsTimer.remove(1);
  tsTimer.remove(2);
  tsTimer.add(2, SunStopTime_ms, StopTimeReached, false);
}

void ScreenGoingDown()
{
  GoingDown = true;
  GoingUp = false;
  tsTimer.remove(0);
  ScreenLocation = "goingdown";
  digitalWrite(RelayUpPin, HIGH);
  digitalWrite(RelayDownPin, LOW);
  DisplayStatus();
  tsTimer.remove(1);
  tsTimer.add(1, SunScreenActive_ms, ScreenIsDown, false);
}

void DisplayStatus() {
  //Remove screen update
  tsTimer.remove(4);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  if (WIFIconnected == true)
  {
    display.drawString(0, 0, "CONNECTED  P: " + String(WiFi.RSSI()) + " dBm");
    display.drawString(0, 16, "IP:" + WiFi.localIP().toString() + "  " + sJSONScreenCommand);
  }
  else
  {
    display.drawString(0, 0,  "DISCONNECTED");
  }
  display.drawLine(0, 32, 128, 32);
  if (ScreenLocation == "goingdown")
  {
    display.drawString(5, 38, "Screen");
    display.drawString(5, 48, "is going");
    display.setFont(ArialMT_Plain_24);
    display.drawString(58, 35, "Down");
  }
  else  if (ScreenLocation == "goingup")
  {
    display.drawString(5, 38, "Screen");
    display.drawString(5, 48, "is going");
    display.setFont(ArialMT_Plain_24);
    display.drawString(70, 35, "Up");
  }
  else if (ScreenLocation == "high")
  {
    display.drawString(5, 38, " Screen");
    display.drawString(5, 48, "     is");
    display.setFont(ArialMT_Plain_24);
    display.drawString(58, 35, "High");
  }
  else if (ScreenLocation == "low")
  {
    display.drawString(5, 38, " Screen");
    display.drawString(5, 48, "     is");
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 35, "Low");
  }
  else if (ScreenLocation == "hold")
  {
    display.drawString(5, 38, " User");
    display.drawString(5, 48, " Stop");
    display.setFont(ArialMT_Plain_24);
    display.drawString(55, 35, "HOLD");
  }
  else
  {
    display.drawString(5, 38, " Screen");
    display.drawString(5, 48, "     is");
    display.setFont(ArialMT_Plain_24);
    display.drawString(75, 35, "?");
  }
  display.display();
  //Activiate screen update
  tsTimer.add(4, DisplayInterval_ms, UpdateDisplay, false);
}
int GetRequest()
{
  sJSONScreenCommand = "S:HTTP";
  DisplayStatus();
  sJSONScreenCommand = "";
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
  sJSONScreenCommand = "R:" + ScreenLocation;
  DisplayStatus();
  sJSONScreenCommand = "";
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



