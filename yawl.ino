// TODOS
// - clean up
// - save important parameters in eeprom
// - get time from npt or next best http server
// - wifi manager

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ESP8266mDNS.h>
//#include <AsyncWebSocket.h> // memory leak with this one
#include <WebSocketsServer.h>
#include <ProcessScheduler.h>
#include <CapacitiveSensor.h>

Scheduler sched;
CapacitiveSensor   cs = CapacitiveSensor(12,14);   // Pin 12 = sender, 14 = reciever

#define PWMSTEPS 1024

// "witty module" LED pinouts
#define RED   (15)
#define GREEN (13)
#define BLUE  (12)

const int LED = RED;

// fill in your WiFi credentials
const char* SSID = "";
const char* PASS = "";

AsyncWebServer server(80);
WebSocketsServer ws = WebSocketsServer(81);
const char index_html[] PROGMEM = "";

boolean daily = false;
int value = 0; //current LED brightness
int timeout = 1200; // seconds
int duration = 1200; // seconds
uint8_t alarmID = 0;
time_t alarmTime = 0; // when brightness reaches max level

int inctime() {return ((float) duration / (float) PWMSTEPS) * 1000;}

class FadeOutProcess : public Process {
  public:
    FadeOutProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        : Process(manager, pr, period, iterations) {}
    static const int STEPS = 6; //PWMSTEPS / 8;
  protected:
    virtual void service(){
      if (value > 0)
        value >>= 2; //-= 8;
      if (value < 0)
        value = 0;
      analogWrite(LED, value);
    }
    virtual void onEnable(){
      setIterations(STEPS);
    }
};
FadeOutProcess fadeP(sched, HIGH_PRIORITY, 1000/FadeOutProcess::STEPS, 0);

void timeOutCallback(){
  if (value > 0){
    Serial.println("Alarm timed out and value is > 0");
    fadeP.enable();
  }
}

class WakeUpProcess : public Process {
  public:
    WakeUpProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {} 
  protected:
    const int STEPS = 1023;
    virtual void service(){
      value++;
      analogWrite(LED, value);
      int iteration = STEPS+1  - getIterations();
      Serial.printf("This is the %uth iteration\n", iteration);
      if (iteration == STEPS){
        Alarm.timerOnce(timeout, timeOutCallback);
      }
    }
    virtual void onEnable(){
      Serial.println("wakeP enabled");
      setPeriod(inctime());
      setIterations(STEPS);
    }
};
WakeUpProcess wakeP(sched, HIGH_PRIORITY, inctime(), 0);

void alarm() {
  Serial.println("ALARM");
  wakeP.enable();
  if (!daily) {
    Alarm.disable(Alarm.getTriggeredAlarmId());
    Serial.println("Alarm disabled");
  }
}

void updateAlarm(time_t t){
  alarmTime = t;
  time_t n = now();
  Serial.printf("Actual alarm updated to: %u = %u:%02u\nCurrent time: %u -> %u:%02u:%02u\n", t, hour(t), minute(t), n, hour(n), minute(n), second(n));
  time_t internalAlarm = t - duration;   
  Alarm.write(alarmID, internalAlarm);
  Serial.printf("Internal alarm updated to: %u\n", Alarm.read(alarmID));
}

void decodeSettings(char  payload[]){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  if (root.containsKey("date") && root["date"] > 0)
  {
    setTime(root["date"]);
    Serial.printf("Current date updated to: %u\n", now());
  }
  if (root.containsKey("time")){
    updateAlarm(root["time"]);    
  }
  if (root.containsKey("duration")){
    duration = root["duration"];
    updateAlarm(alarmTime);
    Serial.printf("Set duration to %u seconds\n", duration);
  }
  if (root.containsKey("timeout")){
    timeout = root["timeout"];
    Serial.printf("Set timeout to %u seconds\n", timeout);
  }
  if (root.containsKey("value")){
    wakeP.disable();
    int val = root["value"];
    value = val<<2;
    analogWrite(LED, value);
  }
  if (root.containsKey("daily")){
    daily = root["daily"];
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
            
        case WStype_CONNECTED:
            {
                DynamicJsonBuffer jsonBuffer;
                JsonObject &root = jsonBuffer.createObject();
                root["time"] = alarmTime;
                root["duration"] = duration;
                root["timeout"] = timeout;
                root["value"] = value>>2;
                root["daily"] = daily;
                char buffer[128];
                root.prettyPrintTo(buffer, sizeof(buffer));
                ws.sendTXT(num, buffer);
            }
            break;

        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            decodeSettings((char*) payload);
            break;

        case WStype_BIN:
            Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
            //hexdump(payload, lenght);
            break;
    }
}





void setup(void){
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(LED, 0);

  setTime(SECS_YR_2000);
  alarmID = Alarm.alarmRepeat((time_t) 100, alarm);
  Alarm.disable(alarmID);

  wakeP.add();
  fadeP.add();
  
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("yawl")) Serial.println("MDNS responder started");
  MDNS.addService("http", "tcp", 80);

  SPIFFS.begin();

  //ws.onEvent(onWsEvent);
  //server.addHandler(&ws);
  ws.begin();
  ws.onEvent(webSocketEvent);

//server.on("/", HTTP_GET,[](AsyncWebServerRequest *request){ request->send_P(200, "text/html", index_html); });
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age:600");
  
  server.on("/heap",HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.begin();
  Serial.println("HTTP server started");
}

long lastMaximum = 0; 

void loop(void){
  Alarm.delay(10);
  ws.loop();
  sched.run();

  while ((cs.capacitiveSensor(30) > 10) && (value < 1020)) {
    wakeP.disable();
    ws.loop();
    sched.run();

    value++;

    if (value >= 1023) {
      lastMaximum = millis();
      // signal max
      for (int z=0; z<2; z++) {
        int i = 0;
        while (i<=120) {
          i++;
          value = z ? value+5 : value-5;
          analogWrite(LED, value);
          Alarm.delay(5);
          delay(5);
        }
      }
    }
    else {
      analogWrite(LED, value);
      Alarm.delay(5);
      delay(5);
    }
  }

  delay(1);

  if ((cs.capacitiveSensor(30) > 10) 
    && (millis() - lastMaximum > 1000) // at least 1 second since reaching max value
    && (value >= 1020))
  {
    fadeP.enable();
  }



}
