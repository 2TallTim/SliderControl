#pragma once
#include <SliderHardware.h>
class SliderForce
{
public:
	// TODO: implement active ranges, efficient querying
	// TODO: handle forces that need to interact with other forces, e.g spring pressing slider up against wall
	SliderForce();
	

	virtual double getForce(const SliderHardware::slider_state_t state) = 0;

};

