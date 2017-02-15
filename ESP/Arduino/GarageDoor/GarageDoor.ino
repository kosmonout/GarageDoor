#include <ESP8266WiFi.h>
#include <TickerScheduler.h>

const int port = 1337;
const char* ssid = "kosmos";
const char* password = "funhouse";
//RELAYS
const int Rl3GarageButPin = 13;
const int Rl4PowerGaragePin = 2;
const int Rl2LightPin = 12;
const int RlGateOpenPin = 14;
//Inputs
const int DoorSensorPin = 15;
const int GarageButtonPin = 0;
//RGB led
const int grnPin  = 5;
const int  bluPin  = 4;
const int  redPin = 16;
const int Update_ms = 1000;
const int GarageButHold_ms = 1000;
const int GarageDoorActive_ms = 21000;
const int GateDoorOpen_ms = 3000;
const int PowerGarageDoor_ms = 31000;
const int PowerBeforeButton_ms = 500;
const int LdrThreshold = 170;


// Color arrays [R,G,B]
int black[3]  = { 0, 0, 0 };
int white[3]  = { 100, 100, 100 };
int red[3]    = { 100, 0, 0 };
int purple[3]    = { 0, 100, 100 };
int green[3]  = { 0, 100, 0 };
int blue[3]   = { 0, 0, 100 };
int yellow[3] = { 100, 10, 0 };
int dimWhite[3] = { 30, 30, 30 };

// Set initial color
int redVal = black[0];
int grnVal = black[1];
int bluVal = black[2];

int FadeWaitus = 500;      // wait in us to fade

// Initialize color variables
int prevR = redVal;
int prevG = grnVal;
int prevB = bluVal;

int LDRReading;
bool LightRelayHigh;
int GarabeButton;
int DoorSensor;
bool firstrun = true;
bool WIFIconnected = false;
bool ClientConnected = false;
bool GarageDoorActivated = false;
bool GarageDoorOpen = false;

WiFiClient client;

WiFiServer server(port);
void crossFade(int color[3]);
void printWiFiStatus();
void printWiFiStatus();
void GarageHit();
void GateHit();
void SendUpdate();
void GarageDoorActive();
void GateActive();
void LightToggle();
void GarageDoorUnactive();
void Power220Off();
void GarageButtonHit();
TickerScheduler tsTimer(6);

////////////////////////////////////////
void setup(void) {
  crossFade(dimWhite);

  // Configure GPIO2 as OUTPUT.
  pinMode(grnPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluPin, OUTPUT);
  pinMode(Rl3GarageButPin, OUTPUT);
  pinMode (Rl2LightPin, OUTPUT);
  pinMode(Rl4PowerGaragePin, OUTPUT);
  pinMode (RlGateOpenPin, OUTPUT);
  pinMode (DoorSensorPin, INPUT_PULLUP);
  pinMode (GarageButtonPin, INPUT_PULLUP);
  digitalWrite(Rl3GarageButPin, HIGH);
  digitalWrite(Rl2LightPin, HIGH);
  digitalWrite(Rl4PowerGaragePin, HIGH);
  digitalWrite(RlGateOpenPin, HIGH);
  //  delay (500);
  //  digitalWrite(Rl3GarageButPin, LOW);
  //  digitalWrite(Rl2LightPin, HIGH);
  //  digitalWrite(Rl4PowerGaragePin, HIGH);
  //  digitalWrite(RlGateOpenPin, HIGH);
  //  delay (500);
  //  digitalWrite(Rl3GarageButPin, HIGH);
  //  digitalWrite(Rl2LightPin, LOW);
  //  digitalWrite(Rl4PowerGaragePin, HIGH);
  //  digitalWrite(RlGateOpenPin, HIGH);
  //  delay (500);
  //  digitalWrite(Rl3GarageButPin, HIGH);
  //  digitalWrite(Rl2LightPin, HIGH);
  //  digitalWrite(Rl4PowerGaragePin, LOW);
  //  digitalWrite(RlGateOpenPin, HIGH);
  //  delay (500);
  //  digitalWrite(Rl3GarageButPin, HIGH);
  //  digitalWrite(Rl2LightPin, HIGH);
  //  digitalWrite(Rl4PowerGaragePin, HIGH);
  //  digitalWrite(RlGateOpenPin, LOW);
  //  delay (500);
  //  digitalWrite(Rl3GarageButPin, HIGH);
  //  digitalWrite(Rl2LightPin, HIGH);
  //  digitalWrite(Rl4PowerGaragePin, HIGH);
  //  digitalWrite(RlGateOpenPin, HIGH);


  LightRelayHigh = false;

  //Start network
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  // Start TCP server.
  server.begin();
}

////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  tsTimer.update();
  if (client)
  {
    crossFade(green);
    if (client.connected()) {
      if (client.available()) {
        ClientConnected = true;
        if (firstrun) {
          tsTimer.remove(2);
          tsTimer.add(2, Update_ms, SendUpdate, false);
          firstrun = false;
        }
        char command = client.read();
        if (command == 'G') {
          GarageDoorActive();
        }
        else if (command == 'P') {
          GateActive();
        }
        else if (command == 'Y') {
          LightToggle();
        }
      }
    } else {
      Serial.println("Client disconnected.");
      client.stop();
      ClientConnected = false;
      firstrun = true;
      tsTimer.remove(2);
    }
  } else
  {
    // Check if module is still connected to WiFi.
    firstrun = true;
    ClientConnected = false;
    tsTimer.remove(2);
    if (WiFi.status() != WL_CONNECTED) {
      crossFade(purple);
      if (WIFIconnected == true) {
        Serial.print("Wifi disconnected");
      }
      WIFIconnected = false;
      delay(500);
      //      }
    } else {
      crossFade(blue);
      // Print the new IP to Serial.
      if (WIFIconnected == false) {
        printWiFiStatus();
      }
      WIFIconnected = true;
    }
    client = server.available();
  }
  GarabeButton = digitalRead(GarageButtonPin);

  if ( GarabeButton == LOW )
  {
    Serial.print("GB ");
    GarageDoorActive();
  }

  DoorSensor = digitalRead(DoorSensorPin);
  if (GarageDoorActivated == false) {
    if ( DoorSensor == HIGH) {
      GarageDoorOpen = false;
      //Garage door is dicht
    }
    else
    {
      //Garage door is open
      GarageDoorOpen = true;
    }
  }

}
////////////////////////////////////////

void GarageDoorActive()
{
  digitalWrite(Rl4PowerGaragePin, LOW);
  tsTimer.remove(5);
  tsTimer.add(5, PowerBeforeButton_ms, GarageButtonHit, false);
  GarageDoorActivated = true;
  if ( ClientConnected == true)
  {
    if ( GarageDoorOpen == false )
    {
      //Garagedoor is aan het open gaan
      client.write('O');
    }
    else
    {
      //Garagedoor is aan het sluiten
      client.write('S');
    }
  }
  GarageDoorOpen = !GarageDoorOpen;
}

void GarageButtonHit()
{
  tsTimer.remove(5);
  digitalWrite(Rl3GarageButPin, LOW);
  tsTimer.remove(0);
  tsTimer.add(0, GarageButHold_ms, GarageHit, false);
  tsTimer.remove(3);
  tsTimer.add(3, GarageDoorActive_ms, GarageDoorUnactive, false);
  tsTimer.remove(4);;
}

void GateActive()
{
  tsTimer.remove(1);
  if (ClientConnected == true)
  {
    //ASCII "G" Poort deur is aan het openen
    client.write('G');
  }
  digitalWrite(RlGateOpenPin, LOW);
  tsTimer.add(1, GateDoorOpen_ms, GateHit, false);
}

void LightToggle() {
  if (LightRelayHigh == false) {
    digitalWrite(Rl2LightPin, LOW);
    LightRelayHigh = true;
  }
  else
  {
    digitalWrite(Rl2LightPin, HIGH);
    LightRelayHigh = false;
  }
}

void GarageHit() {
  tsTimer.remove(0);
  if (ClientConnected == true)
  {
    //ASCII "N" Poort deur geen actie
    client.write('N');
  }
  digitalWrite(Rl3GarageButPin, HIGH);
}

void GarageDoorUnactive() {
  tsTimer.remove(3);
  GarageDoorActivated = false;
  if (ClientConnected == true)
  {
    if ( DoorSensor == HIGH)
    {
      //Garage door is dicht
      GarageDoorOpen = false;
      client.write('D');
    } else
    {
      //Garage door is open
      GarageDoorOpen = true;
      client.write('L');
    }
  }
  digitalWrite(Rl4PowerGaragePin, LOW);
  tsTimer.remove(4);
  tsTimer.add(4, PowerGarageDoor_ms, Power220Off, false);
}

void Power220Off() {
  tsTimer.remove(4);
  digitalWrite(Rl4PowerGaragePin, HIGH);
}

void GateHit() {
  tsTimer.remove(1);
  digitalWrite(RlGateOpenPin, HIGH);
  if (ClientConnected == true)
  {
    //ASCII "N" Poort deur geen actie
    client.write('N');
  }
}

void SendUpdate() {
  LDRReading = analogRead(A0);
  Serial.println(LDRReading);
  if (LDRReading > LdrThreshold)
  {
    client.write('U');    //Light off
    Serial.println("U");
  }
  else
  {
    client.write('A');    //Light on
    Serial.println("A");
  }
  if (GarageDoorActivated == false) {
    if ( DoorSensor == HIGH) {
      //Garage door is dicht
      client.write('D');
    } else
    {
      //Garage door is open
      client.write('L');
    }
  }
}

/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS

  The program works like this:
  Imagine a crossfade that moves the red LED from 0-10,
    the green from 0-5, and the blue from 10 to 7, in
    ten steps.
    We'd want to count the 10 steps and increase or
    decrease color values in evenly stepped increments.
    Imagine a + indicates raising a value by 1, and a -
    equals lowering it. Our 10 step fade would look like:

    1 2 3 4 5 6 7 8 9 10
  R + + + + + + + + + +
  G   +   +   +   +   +
  B     -     -     -

  The red rises from 0 to 10 in ten steps, the green from
  0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.

  In the real program, the color percentages are converted to
  0-255 values, and there are 1020 steps (255*4).

  To figure out how big a step there should be between one up- or
  down-tick of one of the LED values, we call calculateStep(),
  which calculates the absolute gap between the start and end values,
  and then divides that gap by 1020 to determine the size of the step
  between adjustments in the value.
*/

int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero,
    step = 1020 / step;            //   divide by 1020
  }
  return step;
}

/* The next function is calculateVal. When the loop value, i,
   reaches the step size appropriate for one of the
   colors, it increases or decreases the value of that color by 1.
   (R, G, and B are each calculated separately.)
*/

int calculateVal(int step, int val, int i) {

  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    }
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }
  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }
  return val;
}

/* crossFade() converts the percentage colors to a
   0-255 range, then loops 1020 times, checking to see if
   the value needs to be updated each time, then writing
   the color values to the correct pins.
*/

void crossFade(int color[3]) {
  // Convert to 0-255
  int R = (color[0] * 255) / 100;
  int G = (color[1] * 255) / 100;
  int B = (color[2] * 255) / 100;

  if ((prevR != R) || (prevG != G) || (prevB != B)) {
    int stepR = calculateStep(prevR, R);
    int stepG = calculateStep(prevG, G);
    int stepB = calculateStep(prevB, B);
    for (int i = 0; i <= 1020; i++) {
      redVal = calculateVal(stepR, redVal, i);
      grnVal = calculateVal(stepG, grnVal, i);
      bluVal = calculateVal(stepB, bluVal, i);

      analogWrite(redPin, redVal);   // Write current values to LED pins
      analogWrite(grnPin, grnVal);
      analogWrite(bluPin, bluVal);

      delayMicroseconds(FadeWaitus); // Pause for 'wait' microseconds before resuming the loop
    }
    // Update current values for next loop
    prevR = redVal;
    prevG = grnVal;
    prevB = bluVal;
  }
}
void printWiFiStatus() {
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

