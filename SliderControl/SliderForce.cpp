#include "stdafx.h"
#include "SliderForce.h"


SliderForce::SliderForce(){}


double SliderForce::getForce(const SliderHardware::slider_state_t* state) {
	// Default force is nothing.
	return 0;
}