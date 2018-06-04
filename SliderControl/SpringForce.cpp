#include "stdafx.h"
#include "SpringForce.h"

SpringForce::SpringForce(double rl, double k, int side=0) {
  restLength = rl;
  springK = k;
  this->side = side;
}

double SpringForce::getForce(const SliderHardware::slider_state_t* state) {
  double displaced_length = state->position - this->restLength;
  if (displaced_length * side >= 0) {
    return springK * displaced_length;
  }
  return 0;
}
