// TODOS
// - save important parameters in eeprom
// - (low prio.) get time from npt or next best http server
// - add capacitive sensing
// - test caching
// - (low prio.) wifi manager

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


// "witty module" LED pinouts
const int red = 15;
const int green = 13;
const int blue = 12;

const int led = red;

// fill in your WiFi credentials
const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);
WebSocketsServer ws = WebSocketsServer(81);
const char index_html[] PROGMEM = "";

boolean daily = false;
byte value = 0; //current LED brightness
int timeout = 1200; // seconds
int duration = 1200; // seconds
uint8_t alarmID = 0;
time_t alarmTime = 0; // when brightness reaches max level

int inctime() {
  return ((float) duration / 256.0) * 1000;
}

void timeOutCallback(){
  if (value > 0){
    // TODO: dim instead of abpruptly switching of
    analogWrite(led, 0);
  }
}

class WakeUpProcess : public Process {
  public:
    WakeUpProcess(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
        :  Process(manager, pr, period, iterations) {}
  
  protected:
    virtual void service(){
      value++;
      analogWrite(led, value);
      int iteration = 256 - getIterations();
      Serial.printf("This is the %uth iteration\n", iteration);
      if (iteration == 255){
        Alarm.timerOnce(timeout, timeOutCallback);
      }
    }
};


Scheduler sched;
WakeUpProcess wakeP(sched, HIGH_PRIORITY, inctime(), 255);


void alarm() {
  Serial.println("ALARM");
  
  analogWrite(led, 255);
  Alarm.delay(500);
  analogWrite(led, 0);

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
    wakeP.setPeriod(inctime());
    Serial.printf("Set duration to %u seconds\n", duration);
  }
  if (root.containsKey("timeout")){
    timeout = root["timeout"];
    Serial.printf("Set timeout to %u seconds\n", timeout);
  }
  if (root.containsKey("value")){
    int val = root["value"];
    analogWrite(led, val);
    value = val;
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
                root["value"] = value;
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
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  digitalWrite(led, 0);

  setTime(SECS_YR_2000);
  alarmID = Alarm.alarmRepeat((time_t) 100, alarm);
  Alarm.disable(alarmID);

  wakeP.add();
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to ");
  Serial.println(ssid);
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
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
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

void loop(void){
  Alarm.delay(10);
  ws.loop();
  sched.run();
}
