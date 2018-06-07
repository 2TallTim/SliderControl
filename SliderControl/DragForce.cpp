#include "stdafx.h"
#include "DragForce.h"

DragForce::DragForce(double _mu) { mu = _mu; }

double DragForce::getForce(const SliderHardware::slider_state_t* state) {
  return state->fx * mu;
}