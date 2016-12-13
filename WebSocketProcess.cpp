#include <WebSocketsServer.h>
// #include <ArduinoJson.h>

#include "WebSocketProcess.h"
#include "WakeUpProcess.h"
#include "Config.h"


WebSocketProcess::WebSocketProcess(
  WakeLight &light, 
  WebSocketsServer &wss,
	Scheduler &manager, 
	ProcPriority pr, 
	unsigned int period, 
	int iterations)
    :  Process(manager, pr, period, iterations, 10), wlight(light), ws(wss) {} 

void WebSocketProcess::service(){
	ws.loop();
}
void WebSocketProcess::setup(){
  ws.begin();
  ws.onEvent(wsEvent);
  Serial.println("WebSocketServer started\n");
}

void WebSocketProcess::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
    switch(type) {
        case WStype_DISCONNECTED:
            if (DEBUG) Serial.printf("[%u] Disconnected!\n", num);
            break;
            
        case WStype_CONNECTED:
            {
              char buffer[128];
              wlight.toCharBuffer(&buffer[0], sizeof(buffer));
              ws.sendTXT(num, buffer);
            }
            break;

        case WStype_TEXT:
            if (DEBUG) Serial.printf("[%u] get Text: %s\n", num, payload);
            wlight.decodeSettings((char*) payload);
            break;

        case WStype_BIN:
            if (DEBUG) Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
            //hexdump(payload, lenght);
            break;
    }
}

void WebSocketProcess::handleWarning(ProcessWarning warning){
  if (warning == WARNING_PROC_OVERSCHEDULED)
    if (DEBUG) Serial.println("WARNING: overscheduling of WebSocketProcess");
}