
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <TickerScheduler.h>
#include <ArduinoJson.h>

const int port = 80;
const char* ssid = "kosmos";
const char* password = "funhouse";
const char* incommingserver = "http://192.168.2.165/api/app/com.internet/Screen";

//Start network
//ESP8266WebServer WEBserver(port);
WiFiClient client;
WiFiServer server(port);

void printWiFiStatus();
void turnOn();
void turnOff();
bool readRequest(WiFiClient& client);
String prepareHtmlPage();
JsonObject& prepareResponse(JsonBuffer& jsonBuffer);
void writeResponse(WiFiClient& client, JsonObject& json);
int GetRequest();
bool firstrun = true;
bool WIFIconnected = false;
bool ClientConnected = false;
String ScreenLocation = "none";

TickerScheduler tsTimer(6);

////////////////////////////////////////
void setup(void)
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
   WIFIconnected = false;
}

////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  tsTimer.update();
  // WEBserver.handleClient();
  // Check if module is still connected to WiFi.
  if (WiFi.status() != WL_CONNECTED)
  {
    if (WIFIconnected == true)
    {
      Serial.println("Wifi disconnected");
      // WEBserver.close();
      server.close();
    }
    WIFIconnected = false;
    delay(500);
  }
  else
  {
    // Print the new IP to Serial.
    if (WIFIconnected == false) 
    {
      printWiFiStatus();
      // Start TCP server.
      // WEBserver.begin();
      server.begin();
      WIFIconnected = true;
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
          int size = line.length()+1;
          char jsonChar[size];
          line.toCharArray(jsonChar, size);
          Serial.println(jsonChar);
          StaticJsonBuffer<200> jsonReadBuffer;
          JsonObject& json_parsed = jsonReadBuffer.parseObject(jsonChar);
          if (!json_parsed.success())
          {
            Serial.println("parseObject() failed");
            return;
          }
          // Make the decision to turn off or on the LED
          char screencontent[5];
          strcpy(screencontent, json_parsed["screen"]);
          Serial.print ("Screen: ");
          Serial.println(screencontent);
          if (strcmp(screencontent,"on")==0) 
          {
            turnOn();
          }
          else if (strcmp(screencontent,"off")==0)  
          {
            turnOff();
          }
          else 
          {
            Serial.println("JSON command not found.");
          }
        }
        //Send JSON
        delay(1000);
        Serial.println("Client data.");
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

void printWiFiStatus() {
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

int GetRequest()
{
  HTTPClient http;
  http.begin(incommingserver);
  int httpCode = http.GET();
  http.end();
  return httpCode;
}

void turnOn()
{
  Serial.println("Turned On");
  ScreenLocation = "Up";
}

void turnOff()
{
  Serial.println("Turned Off");
  ScreenLocation = "Down";
}

JsonObject& prepareResponse(JsonBuffer & jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& screenValues = root.createNestedArray("Screen");
  screenValues.add(ScreenLocation);
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



