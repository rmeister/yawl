#ifndef SERVERPROCESS_H
#define SERVERPROCESS_H

#include <ProcessScheduler.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ESPAsyncWebServer.h>

#include "WakeLight.h"
#include "WakeUpProcess.h"

const char index_html[] PROGMEM = "";

class ServerProcess : public Process {
public:
	ServerProcess(WakeLight &light, 
		WakeUpProcess &wp,
		AsyncWebServer &svr,
		OnTick_t alarm,
		Scheduler &manager, 
		ProcPriority pr, 
		unsigned int period, 
		int iterations);
	void alarm();

protected:
	virtual void service();
	virtual void setup();
    virtual void handleWarning(ProcessWarning warning);

private:
	AsyncWebServer &server;
	WakeLight &wlight;
	WakeUpProcess &wakeP;
	OnTick_t alarmCallback;

	void initTimeAndAlarm();
};

#endif