/*****************************************************************************
 * 
 * ZWAY HTTP Device for controlling RF sockets and for reporting temperature. 
 * 
 * This code is designed to run on an ESP8266 based, arduino-compatible board
 * such as an AdaFruit Huzzah or a SweetPeas ESP210.
 * 
 * It relies on the following components:
 * 
 * RF transmitter (data pin connected to pin 12)
 * TMP 36GT9 (data pin connected to analog pin A0)
 * 
 * Code adapted from sample RCSwitch and sample ESP8266 Web Server code
 * 
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <RCSwitch.h>
#include "secrets.h"

// RF stuff
RCSwitch mySwitch = RCSwitch();

char * socketOn[5];
char * socketOff[5];

int transmitPin = 12;
int pulseLength = 182;
int repeat = 3;

// web server
ESP8266WebServer server(80);

// led for assiting in debug
const int led = 13;

/************************************************************
 * Web Server Handlers
 * 
 * server.on("/", handleRoot);
 * server.on("/socket", handleSocket); eg /socket?id=5&state=on
 * server.on("/temperature", handleTemperature);
 * server.on("/analogread", handleAnalogRead);
 * 
 ************************************************************/

// used to test connection from web browser
void handleRoot() {
  Serial.write("handle root\n");
  server.send(200, "text/plain", "hello from esp8266!");
}

// used to test value returned by analogRead
void handleAnalogRead (){
  int reading = analogRead(A0); 
  String webString;

  webString=""+String(reading);
  
  server.send(200, "text/plain", webString);  
}

// reads analog pin, converts to a temperature and returns in HTTP response
void handleTemperature() {
  int reading = analogRead(A0); 
  String webString;

  Serial.write("handle temperature\n");
  Serial.print("A0 reading = "); 
  Serial.println(reading);
  // when power comes via the AC/DC adapter, the analogread is low by about 32, so add on here
  reading = reading + 32;
  // max reading on pin A of Huzzah is 1.0V, which corresponds to 1024
  float scaler = 1024.0 / 1000.0;
  float voltage = reading * scaler;
  // print out the voltage
  Serial.print(voltage); 
  Serial.println(" millivolts");

  // now print out the temperature
  float temperatureC = ((voltage - 500) / 10) ;  //converting from 10 mv per degree wit 500 mV offset
                                               //to degrees ((voltage - 500mV) times 100)
  Serial.print(temperatureC); 
  Serial.println(" degrees C");

  webString=""+String(temperatureC);

  Serial.print(webString);
  Serial.write("\n");
  
  server.send(200, "text/plain", webString);
}

// extracts socket id and state
// transmits appropriate code via RF transmitter
// returns state that it thinks the socket is in (can't read the state though, so not accurate)
void handleSocket() {
  byte socketID=server.arg("id")[0];
  String state=server.arg("state");
  
  char * bitSequence = "";

  Serial.write("handle socket\n");
  Serial.write(socketID);
  Serial.write("\n");

  if (state == "on") {
    switch (socketID) {
      case '1': bitSequence = socketOn[0];
                break;      
      case '2': bitSequence = socketOn[1];
                break;
      case '3': bitSequence = socketOn[2];
                break;
      case '4': bitSequence = socketOn[3];
                break;
      case '5': bitSequence = socketOn[4];
                break;
      default: Serial.write("invalid socket num\n");
      }
  }
  else if (state == "off") {
    switch (socketID) {
      case '1': bitSequence = socketOff[0];
                break;      
      case '2': bitSequence = socketOff[1];
                break;
      case '3': bitSequence = socketOff[2];
                break;
      case '4': bitSequence = socketOff[3];
                break;
      case '5': bitSequence = socketOff[4];
                break;
      default: Serial.write("invalid socket num\n");
      }    
  }
  if (bitSequence != "")
      mySwitch.send(bitSequence);
 
  if (state == "on"){
    digitalWrite(13, LOW);
    Serial.write("socket on\n");
  }
  else if (state == "off") {
    digitalWrite(13, HIGH);
    Serial.write("socket off\n");
  }
  server.send(200, "text/plain", "Socket is now " + state);
}

// catches any requests not recognised and returns a 404 error
void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

/*********************************************************************
 * 
 * Arduino setup and loop functions
 * 
 *********************************************************************/
void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // delay of 10 seconds to allow serial monitor to be connected for display of IP address etc.
  delay (10000);
  Serial.println("ZWAY HTTP Device: RF transmitter and temperature sensor");



  // these codes were sniffed using an RF receiver and an arduino uno
  // they are specific to my EtekCity socket set
  socketOn[0] = "010100010001010100110011";
  socketOff[0] = "010100010001010100111100";
  socketOn[1] = "010100010001010111000011";
  socketOff[1] = "010100010001010111001100";
  socketOn[2] = "010100010001011100000011";
  socketOff[2] = "010100010001011100001100";
  socketOn[3] = "010100010001110100000011";
  socketOff[3] = "010100010001110100001100";
  socketOn[4] = "010100010011010100000011";
  socketOff[4] = "010100010011010100001100";
  
  // rx transmitter is connected to pin #12  
  mySwitch.enableTransmit(12);

  // Optional set pulse length.
  mySwitch.setPulseLength(182);
  
  // Optional set protocol (default is 1, will work for most outlets)
  // mySwitch.setProtocol(2);
  
  // Optional set number of transmission repetitions.
  mySwitch.setRepeatTransmit(15);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/socket", handleSocket);

  server.on("/temperature", handleTemperature);

  server.on("/analogread", handleAnalogRead);
  
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
