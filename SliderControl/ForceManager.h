#pragma once

#include <SliderForce.h>
#include <stdlib.h>
#include <set>

class ForceManager
{
public:
	static bool addForce(SliderForce *f);
	static void removeForce(SliderForce *f);
	static double computeForce(SliderForce::slider_state_t state);
private:
	static std::set<SliderForce*> active_forces;
};

