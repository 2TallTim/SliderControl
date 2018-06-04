#pragma once
#include "stdafx.h"
#include <pthread.h>
#include <NIDAQmx.h>
#include <iostream>


class NIDAQManager {
public:
	//TODO: refactor to hardware abstraction, create registration methods instead
  static float outputVoltage(float voltage);
  static float readForce();
  static float readPosition();
  static void initThreads();

private:
  static pthread_mutex_t daq_read_mutex;
  // TODO: merge with slider state?
  static struct reading_t { float position, fx, fz; } last_reading;

  static pthread_mutex_t daq_write_mutex;
  static float last_write;

  static void *DAQThread(void *_);
  static pthread_t daq_thread_id;
  static bool running;

  static const float64 fx_mult_vals[6];

  static const float64 Fz_mult_vals[6];
  static const float64 Fz_gain;
  static const float64 Fx_gain;
};
