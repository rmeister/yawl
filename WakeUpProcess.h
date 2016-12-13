#ifndef WAKEUPPROCESS_H
#define WAKEUPPROCESS_H

#include <ProcessScheduler.h>
#include <Time.h>
#include <TimeAlarms.h>

#include "WakeLight.h"
#include "CapacitiveSensingProcess.h"

class WakeUpProcess : public Process {
  public:
    WakeUpProcess(WakeLight &light, CapacitiveSensingProcess &cs, Scheduler &manager, ProcPriority pr, OnTick_t toc);
    void timeout();

  protected:
    WakeLight& wlight;
    CapacitiveSensingProcess& senseProc;
    OnTick_t timeOutCallback;
    
    virtual void service();
    virtual void onEnable();

    int inctime();
};

#endif