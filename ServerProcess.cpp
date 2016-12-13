#include <Time.h>
#include <TimeAlarms.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
//#include <AsyncWebSocket.h> // memory leak with this one

#include "ServerProcess.h"
#include "Config.h"

ServerProcess::ServerProcess(WakeLight &light, 
		WakeUpProcess &wp,
		AsyncWebServer &svr,
		OnTick_t alarm,
		Scheduler &manager, 
		ProcPriority pr, 
		unsigned int period, 
		int iterations)
        :  Process(manager, pr, period, iterations, 10), wlight(light), wakeP(wp), alarmCallback(alarm), server(svr){} 


void ServerProcess::service(){
	Alarm.delay(0);
	if (wlight.paramsChanged())
		wlight.saveParameters();
}

void ServerProcess::setup(){
	wlight.restoreParameters();
	initTimeAndAlarm();

	SPIFFS.begin();

	//server.addHandler(&ws); // memory leaking websockets, not used, see WebSocketProcess instead
	
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
	Serial.println("\nHTTP server started");

	if (MDNS.begin("yawl")) 
		Serial.println("MDNS responder started");
	MDNS.addService("http", "tcp", 80);
}

void ServerProcess::initTimeAndAlarm(){
	setTime(SECS_YR_2000);
	int id = Alarm.alarmRepeat(0, alarmCallback);
	wlight.setAlarmID(id);
	// Alarm.disable(id);
	wlight.updateAlarm();
}

void ServerProcess::alarm() {
  Serial.println("ALARM");
  wakeP.enable();
  if (!wlight.isDaily()) {
    Alarm.disable(Alarm.getTriggeredAlarmId());
    Serial.println("Alarm will not repeat.");
  }
}

void ServerProcess::handleWarning(ProcessWarning warning){
  if (warning == WARNING_PROC_OVERSCHEDULED)
    Serial.println("WARNING: overscheduling of ServerProcess");
}