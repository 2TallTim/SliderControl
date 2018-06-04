#pragma once
class SliderHardware {
	//TODO: Create full HAL to allow for offline testing
public:
	SliderHardware();
	~SliderHardware();
	struct slider_state_t
	{
		double position;
		double fx, fy, fz;
	};
	//void outputVoltage(float voltage);
	//Figure out what needs to go here.
};

