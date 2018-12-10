#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "REPLACE";
const char* password = "ME";

const unsigned long TOTAL_ALARM_DURATION = 60000;
const unsigned long INITIAL_BLINKING_DURATION = 10000;
const unsigned long REPEAT_EVERY = 5000;
const unsigned long REPETITION_DURATION = 1000;

ESP8266WebServer server(80);

const int BLINKER_PIN = 2;
const int ALARM_DISABLED = 0;
const int ALARM_ENABLED = 1;  

const int BLINKER_DISABLED = HIGH;
const int BLINKER_ENABLED = LOW;

int alarmState = ALARM_DISABLED;
int blinkerState = BLINKER_DISABLED;

unsigned long alarmStartTime = 0;

unsigned long getActionDuration(unsigned long actionStartTime){
  return millis() - actionStartTime;
}

bool isTimePassed(unsigned long actionStartTime, unsigned long durationMillis){
  return getActionDuration(actionStartTime) >= durationMillis;
}

bool isTimeInRangeInPast(unsigned long actionStartTime, unsigned long leastNumber, unsigned long greatestNumber){
  return isTimePassed(actionStartTime, leastNumber) && !isTimePassed(actionStartTime, greatestNumber);
}


void setBlinkerState(int newState){
  if (blinkerState != newState){
    blinkerState = newState;
    digitalWrite(BLINKER_PIN, newState);
  }
}

void startAlarm(){
  alarmState = ALARM_ENABLED;
  setBlinkerState(BLINKER_ENABLED);
  alarmStartTime = millis();
}

void stopAlarm(){
  alarmState = ALARM_DISABLED;
  setBlinkerState(BLINKER_DISABLED);
}

int isAlarmEnabled(){
  return alarmState == ALARM_ENABLED;
}


void handleTimers(){
  if (!isAlarmEnabled()) return;

  if (isTimePassed(alarmStartTime, TOTAL_ALARM_DURATION)){
    stopAlarm();
    return;
  }

  int desiredBlinkerState = BLINKER_DISABLED;
  if (
    isTimeInRangeInPast(alarmStartTime, 0, INITIAL_BLINKING_DURATION)
    || isTimeInRangeInPast(alarmStartTime, 10000, 15000)
    || getActionDuration(alarmStartTime) % REPEAT_EVERY < REPETITION_DURATION
  ){
    desiredBlinkerState = BLINKER_ENABLED;
  }

  setBlinkerState(desiredBlinkerState);
}


void handleRoot() {
  String response = "hello from esp8266!\n\n";

  response += "Alarm state: ";
  if (isAlarmEnabled()){
    response += "enabled";
  } else {
    response += "disabled";
  } 
  response += "\n";

  response += "Blinker state: ";
  if (blinkerState == BLINKER_ENABLED) {
    response += "enabled";
  } else {
    response += "disabled";
  }
  response += "\n";

  response += "Time passed since last alarm: " + String( getActionDuration(alarmStartTime) / 1000 );
  
  server.send(200, "text/plain", response);
}


void printParams(String& message){
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
}


void handleAlarm() {
  String response = "Alarm state change\n\n";
  
  String alarmArgument = server.arg("alarm");

  if (alarmArgument == "true"){
    startAlarm();
  } else if (alarmArgument == "false"){
    stopAlarm();
  } else {
    response += "Unknown 'alarm' parameter value: " + alarmArgument + "\nNothing changed\n";
  }

  printParams(response);

  server.send(200, "text/plain", response);
}


void handleNotFound(){
  String message = "File Not Found\n\n";
  printParams(message);

  server.send(404, "text/plain", message);
}




void setup(void){
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);

  digitalWrite(0, LOW);
  digitalWrite(BLINKER_PIN, HIGH); // disable light by def

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

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
  server.on("/alarm", handleAlarm);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();

  handleTimers();
}
