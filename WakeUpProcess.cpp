#include <Time.h>
#include <TimeAlarms.h>

#include "WakeUpProcess.h"
#include "Config.h"


WakeUpProcess::WakeUpProcess(
  WakeLight &light, 
  CapacitiveSensingProcess &cs, 
  Scheduler &manager, 
  ProcPriority pr, 
  OnTick_t toc)
  :  Process(manager, pr, 0, PWMRANGE), wlight(light), senseProc(cs), timeOutCallback(toc) {}

int WakeUpProcess::inctime() {
  return ((float) wlight.getDuration() / (float) PWMRANGE) * 1000;
}

void WakeUpProcess::service(){
  if (!wlight.incValue()){
    Alarm.timerOnce(wlight.getTimeout(), timeOutCallback);
    disable();
  }
  int iteration = PWMRANGE+1  - getIterations();
  // Serial.printf("This is the %uth iteration\n", iteration);
  if (iteration == PWMRANGE){
    Alarm.timerOnce(wlight.getTimeout(), timeOutCallback);
    if (DEBUG) Serial.println("Last WakeUpProcess iteration finished");
  }
}
void WakeUpProcess::onEnable(){
  Serial.println("wakeP enabled");
  setPeriod(inctime());
  setIterations(PWMRANGE);
}

void WakeUpProcess::timeout(){
  if (wlight.getValue() > 0){
    if (DEBUG) Serial.println("Alarm timed out and value is > 0");
    senseProc.enableFadeout();
  }
}