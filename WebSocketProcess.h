#ifndef WEBSOCKETPROCESS_H
#define WEBSOCKETPROCESS_H

#include <ProcessScheduler.h>
#include <WebSocketsServer.h>
#include "WakeLight.h"

class WebSocketProcess : public Process {

public:
	WebSocketProcess(
		WakeLight &light, 
		WebSocketsServer &wss,
		Scheduler &manager, 
		ProcPriority pr, 
		unsigned int period, 
		int iterations);
	void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);

protected:
	WebSocketsServer &ws;
	WakeLight &wlight;

	virtual void service();
	virtual void setup();
    virtual void handleWarning(ProcessWarning warning);

};

#endif