#pragma once
#include "stdafx.h"
#include "SliderForce.h"

class SpringForce: public SliderForce{
 public:
  SpringForce(double rl, double k, int side);
   double getForce(const SliderHardware::slider_state_t*  state);
  
 private:
  double restLength;
  double springK;
  int side;
};