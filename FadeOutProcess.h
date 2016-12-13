#ifndef FADEOUTPROCESS_H
#define FADEOUTPROCESS_H

#include <ProcessScheduler.h>
#include "WakeLight.h"

class FadeOutProcess : public Process {
	public:
		FadeOutProcess(WakeLight &light, Scheduler &manager, ProcPriority pr, int fadePeriod, int pwmrange);
	
	protected:
	    WakeLight &wlight;
		int range;
	    int fPeriod;

	    virtual void service();
	    virtual void onEnable();

	private:
	    int calcInterations(int pwmrange);
	    int calcPeriod(int iters);
};
#endif