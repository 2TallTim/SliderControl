
#include "stdafx.h"
#include "NIDAQManager.h"

#define DAQmxErrChk(functionCall)          \
  if (DAQmxFailed(error = (functionCall))) \
    goto Error;                            \
  else

const float64 NIDAQManager::fx_mult_vals[6] = {-0.47358,  0.39123,  3.41870,
                                               -46.59179, -2.22599, 44.93318};

const float64 NIDAQManager::fz_mult_vals[6] = {59.09970, 0.62739,  58.70165,
                                               -1.01252, 60.48335, -0.85125};
const float64 NIDAQManager::fz_gain = 0.314991696374748;
const float64 NIDAQManager::fx_gain = 0.784274894660663;

pthread_mutex_t NIDAQManager::daq_read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t NIDAQManager::daq_write_mutex = PTHREAD_MUTEX_INITIALIZER;

float64 NIDAQManager::voltage_bias[6];

bool NIDAQManager::running = false;
bool NIDAQManager::bias_measured = false;
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
  TaskHandle input_task_handle = (TaskHandle)1;
  TaskHandle output_task_handle = (TaskHandle)2;
  DAQmxCreateTask("i", &input_task_handle);
  DAQmxCreateTask("o", &output_task_handle);

  int32 error = 0;
  /*
  DAQ WIRING
  ==========
  Force sensor: "Dev1/ai52:55,Dev1/ai64:65"
  Motor output: "Dev1/ao0"
  Position read: "Dev1/ai0"
  */

  // Configure channels
  DAQmxCreateAIVoltageChan(input_task_handle, "Dev1/ai0", "PosRead",
                           DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts,
                           NULL);
  DAQmxCreateAIVoltageChan(input_task_handle, "Dev1/ai52:55,Dev1/ai64:65",
                                       "",
                           DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts,
                           NULL);
  DAQmxCreateAOVoltageChan(output_task_handle, "Dev1/ao0", "ForceWrite", -10.0, 10.0,
                           DAQmx_Val_Volts, "");


  DAQmxStartTask(input_task_handle);
  DAQmxStartTask(output_task_handle);
  char errBuff[2048] = {'\0'};
  while (running) {
    float64 data[7];
    float64 Fz_Value = 0.0;
    float64 Fx_Value = 0.0;
    int32 read;


    DAQmxReadAnalogF64(input_task_handle, -1, float64(1.0),
                                   DAQmx_Val_GroupByChannel, data, 7,
                       &read, NULL);
    if (!bias_measured) {
      memcpy(voltage_bias, data + 1, 6 * sizeof(float64));
      bias_measured = true;
    }
    for (int i = 1; i < 7; i++) {
      Fz_Value +=
          (fz_mult_vals[i - 1] / fz_gain) * (data[i] - voltage_bias[i - 1]);
      Fx_Value +=
          (fx_mult_vals[i - 1] / fx_gain) * (data[i] - voltage_bias[i - 1]);
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
    DAQmxWriteAnalogF64(output_task_handle, 1, 1, 1.0, DAQmx_Val_GroupByChannel,
                        &voltage_to_write, nullptr, NULL);
  }

  DAQmxStopTask(input_task_handle);
  DAQmxStopTask(output_task_handle);
  DAQmxClearTask(input_task_handle);
  DAQmxClearTask(output_task_handle);

  return nullptr;
Error:
  if (DAQmxFailed(error)) DAQmxGetExtendedErrorInfo(errBuff, 2048);
  if (input_task_handle != 0) {
    /*********************************************/
    // DAQmx Stop Code
    /*********************************************/
    DAQmxStopTask(input_task_handle);
    DAQmxClearTask(input_task_handle);
  }
  if (DAQmxFailed(error)) printf("DAQmx Error: %s\n", errBuff);
  printf("End of program, press Enter key to quit\n");
  getchar();
  return 0;
}
