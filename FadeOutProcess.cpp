#include <math.h>

#include "FadeOutProcess.h"

FadeOutProcess::FadeOutProcess(WakeLight &light, Scheduler &manager, ProcPriority pr, int fadePeriod, int pwmrange)
      : Process(manager, pr, 0, 0), wlight(light), fPeriod(fadePeriod), range(pwmrange) {}

void FadeOutProcess::service(){
  int val = wlight.getValue();
  if (val > 0)
    val >>= 2; //-= 8;
  if (val < 0)
    val = 0;
  wlight.setValue(val);
}

void FadeOutProcess::onEnable(){
  int iters = calcInterations(range);
  setIterations(iters);
  setPeriod(calcPeriod(iters));
}

int FadeOutProcess::calcInterations(int pwmrange){
  return log10(pwmrange)/log10(2);
}
int FadeOutProcess::calcPeriod(int iters){
  return fPeriod / iters;
}
