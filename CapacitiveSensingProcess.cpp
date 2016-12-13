#include "CapacitiveSensingProcess.h"
#include "Config.h"

CapacitiveSensingProcess::CapacitiveSensingProcess(WakeLight &light, 
        CapacitiveSensor &cs, 
        Scheduler &manager, 
        ProcPriority pr, 
        unsigned int period, 
        int iterations)
      : Process(manager, pr, period, iterations, 10), wlight(light), cs(cs) {}

void CapacitiveSensingProcess::service(){
  // Serial.printf("State: %u\n ", state);
  int currentValue = wlight.getValue();
  switch(state){
    case NORMAL:
        if (cs.capacitiveSensor(SAMPLES) > 10){
          // if (DEBUG) Serial.printf("Touch detected! CurrentValue: %u\n", currentValue);
          if (wlight.setValue(currentValue+10) == PWMRANGE) {
            lastMaximum = millis();
            state = SIGNALMAXDOWN;
            if (DEBUG) Serial.println("NORMAL--->SIGNALMAXDOWN");
            yield();
          }
        } else if (currentValue == PWMRANGE){
          state = MAXBRIGHTNESS;
          if (DEBUG) Serial.println("NORMAL--->MAXBRIGHTNESS");  
        }
      break;
      
    case SIGNALMAXDOWN:
      if (currentValue > PWMRANGE/5){
        wlight.setValue(currentValue-20);
      }
      else { 
        state = SIGNALMAXUP;
        if (DEBUG) Serial.println("SIGNALMAXDOWN--->SIGNALMAXUP");
      }
      break;

    case SIGNALMAXUP:
      if (currentValue < PWMRANGE)
        wlight.setValue(currentValue+20);
      else {
        state = MAXBRIGHTNESS;
        if (DEBUG) Serial.println("SIGNALMAXUP--->MAXBRIGHTNESS");
      }
      break;

    case MAXBRIGHTNESS:
      if ((cs.capacitiveSensor(SAMPLES) > 10) && (millis() - lastMaximum > 4000)) {// before fading off, wait some seconds
        state = FADEOUT;
        if (DEBUG) Serial.println("MAXBRIGHTNESS--->FADEOUT");
      } else if (currentValue < PWMRANGE) {
        state = NORMAL;
        if (DEBUG) Serial.println("MAXBRIGHTNESS--->NORMAL");
      }
      break;

    case FADEOUT:
      // if (DEBUG) Serial.println("Fading...");
      if (currentValue > 0)
        currentValue >>=1; //-=20;
      else {
        state = NORMAL;
        yield();
        if (DEBUG) Serial.println("FADEOUT--->NORMAL");
      }
      wlight.setValue(currentValue);
      break;
  }
}

void CapacitiveSensingProcess::setup(){
  lastMaximum = 0;
  state = NORMAL;
}

void CapacitiveSensingProcess::handleWarning(ProcessWarning warning){
  if (warning == WARNING_PROC_OVERSCHEDULED)
    Serial.println("WARNING: overscheduling of CapacitiveSensingProcess");
}

void CapacitiveSensingProcess::enableFadeout(){
  state = FADEOUT;
}