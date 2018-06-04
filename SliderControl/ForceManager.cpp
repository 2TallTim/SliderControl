#include "stdafx.h"
#include "ForceManager.h"
/*
  ForceManager handles the storage and computation of slider forces. 
*/

std::set<SliderForce *> ForceManager::active_forces;

bool ForceManager::addForce(SliderForce *f) {
	auto r = active_forces.insert(f);
	//Return true if insertion took place.
	return r.second;
}

void ForceManager::removeForce(SliderForce *f) {
	auto r = active_forces.erase(f);
}

double ForceManager::computeForce(SliderHardware::slider_state_t state) {
	double force_sum = 0.0;

	for (auto i : active_forces){
		force_sum += i->getForce(state);
	}
	return force_sum;
}