#ifndef CAPACITIVESENSING_H
#define CAPACITIVESENSING_H

#include <ProcessScheduler.h>
#include <CapacitiveSensor.h>

#include "WakeLight.h"

typedef enum SensingState{
    NORMAL,
    SIGNALMAXDOWN,
    SIGNALMAXUP,
    MAXBRIGHTNESS,
    FADEOUT
} SensingState;

class CapacitiveSensingProcess : public Process {
  public:
    CapacitiveSensingProcess(WakeLight &light, 
        CapacitiveSensor &cs, 
        Scheduler &manager, 
        ProcPriority pr, 
        unsigned int period, 
        int iterations);

    void enableFadeout();

  protected:
    WakeLight& wlight;
    CapacitiveSensor& cs;
    long lastMaximum;
    SensingState state;
    const byte SAMPLES = 20;
    
    virtual void service();
    virtual void setup();
    virtual void handleWarning(ProcessWarning warning);
};

#endif