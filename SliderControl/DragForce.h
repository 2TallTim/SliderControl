#pragma once
#include "SliderForce.h"
#include "stdafx.h"

class DragForce : public SliderForce {
 public:
  DragForce(double _k);
  double getForce(const SliderHardware::slider_state_t* state);

 private:
  double mu;
};