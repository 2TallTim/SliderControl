
#include "stdafx.h"
#include "NIDAQManager.h"

const float64 NIDAQManager::fx_mult_vals[6] = {-0.47358,  0.39123,  3.41870,
                                               -46.59179, -2.22599, 44.93318};

const float64 NIDAQManager::Fz_mult_vals[6] = {59.09970, 0.62739,  58.70165,
                                               -1.01252, 60.48335, -0.85125};
const float64 NIDAQManager::Fz_gain = 0.314991696374748;
const float64 NIDAQManager::Fx_gain = 0.784274894660663;

pthread_mutex_t NIDAQManager::daq_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t NIDAQManager::daq_write_mutex = PTHREAD_MUTEX_INITIALIZER;

bool NIDAQManager::running = false;

double NIDAQManager::last_write;
pthread_t NIDAQManager::daq_thread_id;
NIDAQManager::reading_t NIDAQManager::last_reading;

double NIDAQManager::outputVoltage(double voltage) {
  if (!running) {
    std::cerr << "Thread not running.\n";
    return NULL;
  }

  pthread_mutex_lock(&daq_write_mutex);
  last_write = voltage;
  pthread_mutex_unlock(&daq_write_mutex);
}

double NIDAQManager::readForce() {
  if (!running) {
    std::cerr << "Thread not running.\n";
    return NULL;
  }

  double ret = 0;

  pthread_mutex_lock(&daq_read_mutex);
  ret = last_reading.fx;
  // TODO: refactor, separate readings for position and force. Separate by axis.
  pthread_mutex_unlock(&daq_read_mutex);

  return ret;
}

double NIDAQManager::readPosition() {
  if (!running) {
    std::cerr << "Thread not running.\n";
    return NULL;
  }

  double ret = 0;

  pthread_mutex_lock(&daq_read_mutex);
  ret = last_reading.position;
  pthread_mutex_unlock(&daq_read_mutex);

  return ret;
}

void NIDAQManager::initThreads() {
  // Create a thread to handle incoming requests for data

  pthread_mutex_init(&daq_read_mutex, NULL);
  pthread_mutex_init(&daq_write_mutex, NULL);

  running = true;

  pthread_create(&daq_thread_id, NULL, DAQThread, NULL);
}

void *NIDAQManager::DAQThread(void *_) {
  TaskHandle taskHandle = (TaskHandle)1;
  DAQmxCreateTask("", &taskHandle);

  /*
  DAQ WIRING
  ==========
  Force sensor: "Dev1/ai52:55,Dev1/ai64:65"
  Motor output: "Dev1/ao0"
  Position read: "Dev1/ai0"
  */

  // Configure channels
  DAQmxCreateAIVoltageChan(taskHandle, "Dev1/ai0", "PosRead",
                           DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts,
                           NULL);
  DAQmxCreateAIVoltageChan(taskHandle, "Dev/ai52:55,Dev1/ai64:65", "ForceRead",
                           DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts,
                           NULL);
  DAQmxCreateAOVoltageChan(taskHandle, "Dev/ao0", "ForceWrite", -10.0, 10.0,
                           DAQmx_Val_Volts, "");

  // Configure sample clocks
  DAQmxCfgSampClkTiming(taskHandle, "PosRead", 1000, DAQmx_Val_Rising,
                        DAQmx_Val_FiniteSamps, 2);
  DAQmxCfgSampClkTiming(taskHandle, "ForceRead", 1000, DAQmx_Val_Rising,
                        DAQmx_Val_FiniteSamps, 2);

  DAQmxStartTask(taskHandle);

  while (running) {
    float64 data[7];
    float64 Fz_Value = 0.0;
    float64 Fx_Value = 0.0;
    DAQmxReadAnalogF64(taskHandle, 1, 1.0, DAQmx_Val_GroupByScanNumber, data, 1,
                       nullptr, NULL);
    for (int i = 1; i < 7; i++) {
      Fz_Value += (Fz_mult_vals[i - 1] / Fz_gain) * data[i];
      Fx_Value += (fx_mult_vals[i - 1] / Fx_gain) * data[i];
    }

    pthread_mutex_lock(&daq_read_mutex);
    last_reading.fx = (double)Fx_Value;
    last_reading.fz = (double)Fz_Value;
    last_reading.position = data[0];
    pthread_mutex_unlock(&daq_read_mutex);

    float64 voltage_to_write;

    pthread_mutex_lock(&daq_write_mutex);
    voltage_to_write = last_write;
    pthread_mutex_unlock(&daq_write_mutex);

    DAQmxWriteAnalogF64(taskHandle, 1, 1, 1.0, DAQmx_Val_GroupByChannel,
                        &voltage_to_write, nullptr, NULL);
  }

  DAQmxStopTask(taskHandle);
  DAQmxClearTask(taskHandle);

  return nullptr;
}
