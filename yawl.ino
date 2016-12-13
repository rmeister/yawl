#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ProcessScheduler.h>
#include <CapacitiveSensor.h>

#include "Config.h"
#include "WakeLight.h"
#include "FadeOutProcess.h"
#include "WakeUpProcess.h"
#include "WebSocketProcess.h"
#include "ServerProcess.h"
#include "CapacitiveSensingProcess.h"

// fill in your WiFi credentials
const char* SSID = "";
const char* PASS = "";

// "witty module" LED pinout
#define RED    15
#define GREEN  13
#define BLUE   12

// #define LEDPORT RED    // the pin your LED is connected to, defaults to 15
// #define PWMRANGE 1023  // uncomment and change for other controllers than the ESP

// capacitive sensing pins: (sender, receiver)
CapacitiveSensor cs(12,14);

// HTTP and WebSocket servers
AsyncWebServer server(80);
WebSocketsServer ws(81);

WakeLight light;
Scheduler sched;

CapacitiveSensingProcess senseP(light, cs, sched, HIGH_PRIORITY, 30, RUNTIME_FOREVER);
WakeUpProcess wakeP(light, senseP, sched, LOW_PRIORITY, timeout);
ServerProcess serveP(light, wakeP, server, alarm, sched, MEDIUM_PRIORITY, 50, RUNTIME_FOREVER);
WebSocketProcess websP(light, ws, sched, MEDIUM_PRIORITY, 20, RUNTIME_FOREVER);

#ifdef _PROCESS_STATISTICS
long counter;
#endif

void setup(void){
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  analogWrite(RED, 0);
  analogWrite(GREEN, 0);
  analogWrite(BLUE, 0);

  Serial.begin(115200);
  Serial.println("\n––––––––––––––––––––––––––––––––––––––––––");
  Serial.println("_/      _/    _/_/    _/          _/  _/  ");
  Serial.println(" _/  _/    _/    _/  _/          _/  _/   ");  
  Serial.println("  _/      _/_/_/_/  _/    _/    _/  _/    ");
  Serial.println(" _/      _/    _/    _/  _/  _/    _/     ");   
  Serial.println("_/      _/    _/      _/  _/      _/_/_/_/");
  Serial.println("__________________________________________");
  Serial.printf("PWMRANGE is >%u<\n", PWMRANGE);
  
  WiFi.begin(SSID, PASS);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.printf("\nConnected to '%s'\n", SSID);
  Serial.print("IP address: [");
  Serial.print(WiFi.localIP());
  Serial.printf("]\n\n");

  wakeP.add();
  serveP.add();
  websP.add();
  senseP.add();

  serveP.enable();
  websP.enable();
  senseP.enable();


#ifdef _PROCESS_STATISTICS
  counter = 0;
#endif
}

void loop(void){
  sched.run();

#ifdef _PROCESS_STATISTICS
  counter++;
  if (counter == 1e5){
    if (sched.updateStats()) 
      Serial.println("Updated stats:");
    Serial.printf("++STATS++\nwakeP\t%u\nserveP\t%u\nwebsP\t%u\nsenseP\t%u\n",
      wakeP.getLoadPercent(),
      serveP.getLoadPercent(),
      websP.getLoadPercent(),
      senseP.getLoadPercent());
    counter = 0;
  }
#endif
}


// callback wrappers
void alarm() {
  serveP.alarm();
}
void timeout(){
  wakeP.timeout();
}
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght){
  websP.webSocketEvent(num, type, payload, lenght);
}
