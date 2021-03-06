
#include "stdafx.h"
#include <NIDAQManager.h>
#include <SliderHardware.h>
#include <ForceManager.h>
#include <SpringForce.h>
#include <DragForce.h>
#include <GL/glew.h>
#include "GL/wglew.h"
#include "GLFW/glfw3.h"
#include "LoadShaders.cpp"

#include <NIDAQmx.h>
#include <chrono>
#include <ctime>
#include <d3d12shader.h>
#include <iostream>
//#include <thread>
#include <pthread.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

// DAQ error check
#define DAQmxErrChk(functionCall)                                              \
  if (DAQmxFailed(error = (functionCall)))                                     \
    goto Error;                                                                \
  else

// Queue for memory storage
// NEEDS TO BE IMPLEMENTED
std::queue<float> positionQueue;
std::queue<float> forceXQueue;
std::queue<float> forceZQueue;

// Flag used to indicate first reading of force sensor (sets the voltage bias
// array)
int calibrationFlag = 0;
// Voltage bias offset array
float64 VoltageBias[6];

// Position of the wall on a -10 to 7.4V position over a 2 meter scale (.1
// meters/volt)
float64 wallPosition =
    -6.5; //.65 meters on the negative voltage side of the rail

// Definitions for spring equation
float64 springBase = 10;
float64 springStart = 0;
float64 springConstant = -50;

void *visualDisplay(void *threadID) {
  // Initialise GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
  }

  // OpenGL settings
  glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
                 GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE,
                 GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

  // Open a window and create its OpenGL context
  GLFWwindow *window; //(In the accompanying source code, this variable is
                      // global for simplicity)
  window = glfwCreateWindow(1680, 1050, "Visual System", NULL, NULL);

  if (window == NULL) {
    fprintf(stderr,
            "Failed to open GLFW window. If you have an Intel GPU, they are "
            "not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
    glfwTerminate();
  }

  // Initialize GLEW
  glfwMakeContextCurrent(window);
  // Needed in core profile
  glewExperimental = true;

  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // Initialize positionReading to hold position a position read from DAQ
  float positionReading = 0;

  // Do-loop naturally runs at 60 FPS **NEEDS CONFIRMATION
  // Visual may be a bit slower because of delay from calling readPosition()
  // function
  do {
    positionReading = NIDAQManager::readPosition();

    // printf("Visual Position: %f\n", positionReading); //Check position read
    // from the visual

    // Need to convert position reading (-10V to 10V) to OpenGL (-1 to 1) and
    // update the position In the following two lines, we are able to convert
    // the current position to a value between 0 - 2 This value is used and
    // multiplied by the x-axis midpoint of the square in the block (.8) And
    // then it is subtracted from the x of the vertex Visual:
    //-.8                   0                    .8
    //______________________________________________
    // Value is divided by 8.7 because the position reading ranges from roughly
    // -10V to 7.4V (17.4V range) **NEED TO MEASURE EXACTLY HOW TO GET VALUE
    float x = (positionReading + 10) / 8.7;
    float wall = (wallPosition + 10) / 8.7;

    // List of vertices in creating the visual's shapes
    // This is done with 2-D triangles because it is faster than 3-D
    const GLfloat g_vertex_buffer_data[] = {
        // RAIL
        0.95f,
        0.1f,
        0.0f,
        0.9f,
        -0.1f,
        0.0f,
        -0.95f,
        -0.1f,
        0.0f,

        0.95f,
        0.1f,
        0.0f,
        -0.9f,
        0.1f,
        0.0f,
        -0.95f,
        -0.1f,
        0.0f,

        -0.95f,
        -0.1f,
        0.0f,
        0.9f,
        -0.1f,
        0.0f,
        -0.95f,
        -0.15f,
        0.0f,

        -0.95f,
        -0.15f,
        0.0f,
        0.9f,
        -0.1f,
        0.0f,
        0.90f,
        -0.15f,
        0.0f,

        0.9f,
        -0.1f,
        0.0f,
        0.9f,
        -0.15f,
        0.0f,
        0.95f,
        0.1f,
        0.0f,

        0.9f,
        -0.15f,
        0.0f,
        0.95f,
        0.1f,
        0.0f,
        0.95f,
        0.05f,
        0.0f,

        // REDRAWN BACKGROUND - Necessary to entirely clear the last frame
        -0.90f,
        0.2f,
        0.0f,
        -0.90f,
        0.1f,
        0.0f,
        0.95f,
        0.2f,
        0.0f,

        -0.90f,
        0.1f,
        0.0f,
        0.95f,
        0.2f,
        0.0f,
        0.95f,
        0.1f,
        0.0f,

        // BLOCK - Draw block with manipulation of the x-axis of the vertex
        0.85f - (x * .8),
        0.15f,
        0.0f,
        0.75f - (x * .8),
        0.15f,
        0.0f,
        0.85f - (x * .8),
        0.0f,
        0.0f,

        0.75f - (x * .8),
        0.0f,
        0.0f,
        0.75f - (x * .8),
        0.15f,
        0.0f,
        0.85f - (x * .8),
        0.0f,
        0.0f,

        0.75f - (x * .8),
        0.15f,
        0.0f,
        0.85f - (x * .8),
        0.15f,
        0.0f,
        0.86f - (x * .8),
        0.175f,
        0.0f,

        0.86f - (x * .8),
        0.175f,
        0.0f,
        0.75f - (x * .8),
        0.15f,
        0.0f,
        0.76f - (x * .8),
        0.175f,
        0.0f,

        0.85f - (x * .8),
        0.15f,
        0.0f,
        0.85f - (x * .8),
        0.0f,
        0.0f,
        0.86f - (x * .8),
        0.175f,
        0.0f,

        0.85f - (x * .8),
        0.0f,
        0.0f,
        0.86f - (x * .8),
        0.175f,
        0.0f,
        0.86f - (x * .8),
        0.05f,
        0.0f,

        // WALL - Draw wall with manipulation of the x-axis of the vertex
        0.84f - (wall * .8),
        -0.1f,
        0.0f,
        0.88f - (wall * .8),
        0.1f,
        0.0f,
        0.84f - (wall * .8),
        0.2f,
        0.0f,

        0.88f - (wall * .8),
        0.1f,
        0.0f,
        0.88f - (wall * .8),
        0.4f,
        0.0f,
        0.84f - (wall * .8),
        0.2f,
        0.0f,

        0.83f - (wall * .8),
        -0.1f,
        0.0f,
        0.84f - (wall * .8),
        -0.1f,
        0.0f,
        0.83f - (wall * .8),
        0.2f,
        0.0f,

        0.83f - (wall * .8),
        0.2f,
        0.0f,
        0.84f - (wall * .8),
        -0.1f,
        0.0f,
        0.84f - (wall * .8),
        0.2f,
        0.0f,

        0.83f - (wall * .8),
        0.2f,
        0.0f,
        0.87f - (wall * .8),
        0.4f,
        0.0f,
        0.84f - (wall * .8),
        0.2f,
        0.0f,

        0.88f - (wall * .8),
        0.4f,
        0.0f,
        0.87f - (wall * .8),
        0.4f,
        0.0f,
        0.84f - (wall * .8),
        0.2f,
        0.0f
    };

    // All colors for all vertices
    // Matches up with the corresponding vertices in order
    const GLfloat g_color_buffer_data[] = {
        // RAIL COLOR
        0.5,
        0.0f,
        0.0f,
        0.5,
        0.0f,
        0.0f,
        0.5,
        0.0f,
        0.0f,
        0.5,
        0.0f,
        0.0f,
        0.5,
        0.0f,
        0.0f,
        0.5,
        0.0f,
        0.0f,

        0.7,
        0.0f,
        0.0f,
        0.7,
        0.0f,
        0.0f,
        0.7,
        0.0f,
        0.0f,
        0.7,
        0.0f,
        0.0f,
        0.7,
        0.0f,
        0.0f,
        0.7,
        0.0f,
        0.0f,

        0.6,
        0.0f,
        0.0f,
        0.6,
        0.0f,
        0.0f,
        0.6,
        0.0f,
        0.0f,
        0.6,
        0.0f,
        0.0f,
        0.6,
        0.0f,
        0.0f,
        0.6,
        0.0f,
        0.0f,

        // RECOLORED BACKGROUND
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        // BLOCK COLOR
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,

        0.0f,
        0.7f,
        0.0f,
        0.0f,
        0.7f,
        0.0f,
        0.0f,
        0.7f,
        0.0f,
        0.0f,
        0.7f,
        0.0f,
        0.0f,
        0.7f,
        0.0f,
        0.0f,
        0.7f,
        0.0f,

        0.0f,
        0.9f,
        0.0f,
        0.0f,
        0.9f,
        0.0f,
        0.0f,
        0.9f,
        0.0f,
        0.0f,
        0.9f,
        0.0f,
        0.0f,
        0.9f,
        0.0f,
        0.0f,
        0.9f,
        0.0f,

        // WALL BACKGROUND
        0.0f,
        0.5f,
        0.5f,
        0.0f,
        0.5f,
        0.5f,
        0.0f,
        0.5f,
        0.5f,
        0.0f,
        0.5f,
        0.5f,
        0.0f,
        0.5f,
        0.5f,
        0.0f,
        0.5f,
        0.5f,

        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,

        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
        0.0f,
        0.4f,
        0.4f,
    };

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data),
                 g_vertex_buffer_data, GL_DYNAMIC_DRAW);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data),
                 g_color_buffer_data, GL_DYNAMIC_DRAW);

    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader",
                                   "SimpleFragmentShader.fragmentshader");

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, // attribute 0. No particular reason for 0, but
                             // must match the layout in the shader.
                          3, // size
                          GL_FLOAT, // type
                          GL_FALSE, // normalized?
                          0,        // stride
                          (void *)0 // array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, // attribute. No particular reason for 1, but must
                             // match the layout in the shader.
                          3, // size
                          GL_FLOAT, // type
                          GL_FALSE, // normalized?
                          0,        // stride
                          (void *)0 // array buffer offset
    );

    // Draw the triangle!
    // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDrawArrays(GL_TRIANGLES, 0, 3 * 22);
    glDisableVertexAttribArray(0);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
    glUseProgram(programID);
  } // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0);
  return (0);
}

// Wall Thread
void *wallBlock(void *threadID) {
  float64 radius = .014235; // Diameter of the pulley 28.47mm, Radius 14.235mm
  float64 newtonMetersPerAmp = .71; // Constant in motor DO NOT TOUCH
  float64 ampsPerVolt = .096;       // Constant in motor DO NOT TOUCH

  int direction;   // A variable that is equal to -1 or 1 based on where cart is
                   // in relation to the wall
  float64 voltage; // Calculate voltage and write to DAQ
  float64 forceX;  // Holds x force reading from FT sensor
  float64 positionRead; // Holds position reading from motor
  float64 readPos;      // Another variable to hold position reading from motor

  // Initially read and store a single-current position
  positionRead = NIDAQManager::readPosition();

  // Conditional to determine where the cart is in respect to the wall
  if (positionRead >
      wallPosition) { // Current position is greater than wall position
    direction = -1;   // Has to go from -1 to 1 to get closer to wall
  } else if (positionRead <
             wallPosition) { // Current position is less than the wall position
    direction = 1;           // Has to go from 1 to -1 to get closer to wall
  }

  // Start the loop that checks for the wall & apply necessary force to create
  // the wall
  while (1) {
    // Read and store current position
    readPos = NIDAQManager::readPosition();

    // printf("Pos: %f\n", readPos); //Check the position

    // Conditional to compare current position to the wall direction and check
    // if it tries to pass the wall
    if (readPos >= wallPosition &&
        direction == 1) { // Direction says it has to go from negative side
                          // towards positive side but the current position
                          // indicates that it is already on the other side of
                          // the wall Read and store force from FT sensor
      forceX = NIDAQManager::readForce();

      // printf("Direction: %d\n", direction);
      printf("1Force %f\n: ", forceX);

      if (abs(forceX) <=
          50) { // if the force is between 0 and 50 **FIX
                // forceXQueue.push(forceX); //Store read force into queue
        voltage = 1 * (((forceX * radius) / newtonMetersPerAmp) /
                       ampsPerVolt); // Calculate the voltage equal but
                                     // opposite of the read force
        if (voltage > 0 &&
            abs(voltage) <= 10) { // If voltage is negative && absolute value
                                  // of voltage is less than 10
          printf("Voltage %f\n", voltage);
          // write to DAQ as output
          NIDAQManager::outputVoltage(voltage);
        } else { // Otherwise, write 0V to DAQ as output. This is to filter out
                 // potential random voltages
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos <= wallPosition &&
               direction ==
                   -1) { // Direction says it has to go from positive side
                         // towards negative side but the current position
                         // indicates that it is already on the other side of
                         // the wall Read and store force from FT sensor
      forceX = NIDAQManager::readForce();

      // printf("Direction: %d\n", direction);
      printf("Force %f\n: ", forceX);

      if (abs(forceX) <=
          50) { // If the force is between -50 and 0 **FIX
                // forceXQueue.push(forceX); //Store read force into queue
        voltage = 1 * (((forceX * radius) / newtonMetersPerAmp) /
                       ampsPerVolt); // Calculate the voltage equal but
                                     // opposite of the read force
        if (voltage < 0 &&
            abs(voltage) <= 10) { // If voltage is positive && absolute value
                                  // of voltage is less than 10
          printf("Voltage %f\n", voltage);
          // write to DAQ as output
          NIDAQManager::outputVoltage(voltage);
        } else { // Otherwise, write 0V to DAQ as output. This is to filter out
                 // potential random voltages
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos < springStart && readPos > wallPosition) {
      NIDAQManager::outputVoltage(0);
    }
  }
}

void *springForce(void *threadID) {
  float64 radius = .014235;         // radius of the pulley 28.47mm 14.235
  float64 newtonMetersPerAmp = .71; // NM/amps Constant in motor
  float64 ampsPerVolt = .096;       // amps/volts Constant in motor

  float64 springForce;      // Variable for force of the spring
  float64 voltage;          // Calculate voltage and write to DAQ
  float64 readPos;          // Holds position read
  float64 positionToMeters; // Spring displacement

  while (1) {
    // Read and store position from DAQ
    readPos = NIDAQManager::readPosition();

    if (readPos >= springStart && readPos <= springBase) {
      // Calculate spring force
      //.1 meters per volt
      // Calculates displacement. (Take absolute current position and adds 10 to
      // obtain a value from 0 - 17.4 and then multiplies by .1 to get where it
      // is, 0m to 1.74m) Subtract 1 to get a range of .74 meters to -1 meters
      positionToMeters = ((abs(readPos) + 10) * .1) - 1;
      // printf("Displacement: %f\n", positionToMeters);
      // Calculate springForce = *spring constant * displacement
      // The displacement would have to be positionToMeters - springStart, but
      // because springStart is set at 0, it does not matter.
      springForce = springConstant * positionToMeters;
      // printf("Spring Force: %f\n", springForce);
      if (springForce <= 0 ||
          springForce >=
              -50) { // If springForce is between 0 and 50
                     // forceXQueue.push(forceX); //Store force in queue
        voltage = -1 * (((springForce * radius) / newtonMetersPerAmp) /
                        ampsPerVolt); // Calculate voltage
        if (voltage > 0 &&
            abs(voltage) <= 10) { // if voltage is greater than 0 and absolute
                                  // value of voltage is less than 10
          printf("Voltage %f\n", voltage);
          // send to DAQ as output
          NIDAQManager::outputVoltage(voltage);
        } else { // Otherwise, write 0V to DAQ as output. This is to filter out
                 // potential random voltages
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos < springStart &&
               readPos > wallPosition) { // Do nothing when spring isn't active
      printf("Voltage 0\n");
      NIDAQManager::outputVoltage(0);
    }
  }
  return 0;
}

void *springAndWall(void *threadID) {
  // float64 springBase = 10;
  // float64 springStart = 0;
  // float64 springConstant = -50;
  float64 springForce;

  float64 voltage;
  float64 radius = .014235;         // radius of the pulley 28.47mm 14.235
  float64 newtonMetersPerAmp = .71; // Constant in motor
  float64 ampsPerVolt = .096;       // Constant in motor

  float64 readPos;
  float64 positionToMeters;

  int direction;
  float64 forceX;
  float64 position;
  float64 positionRead;

  positionRead = NIDAQManager::readPosition();
  if (positionRead > wallPosition) {
    direction = -1; // Has to go from -1 to 1 to get closer to wall
  } else if (positionRead < wallPosition) {
    direction = 1; // Has to go from 1 to -1 to get closer to wall
  }

  while (1) {
    readPos = NIDAQManager::readPosition();
    // printf("Pos: %f\n", readPos);
    if (readPos <= wallPosition && direction == -1) {
      // read FT sensor, convert to torque, conversion to voltage, and flip sign

      forceX = NIDAQManager::readForce();
      // printf("1Direction: %d\n", direction);
      // printf("1Force %f\n: ", forceX);
      // forceX = 0;
      if (abs(forceX) <= 50) {
        // forceXQueue.push(forceX);
        voltage = .9 * (((forceX * radius) / newtonMetersPerAmp) / ampsPerVolt);
        if (voltage < 0 && abs(voltage) <= 10) {
          printf("Voltage %f\n", voltage);
          // send to DAQ as output
          NIDAQManager::outputVoltage(voltage);
        } else {
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos >= wallPosition && direction == 1) {
      forceX = NIDAQManager::readForce();
      // printf("Direction: %d\n", direction);
      // printf("Force %f\n: ", forceX);
      // forceX = 0;
      if (abs(forceX) <= 50) {
        // forceXQueue.push(forceX);
        voltage = .9 * (((forceX * radius) / newtonMetersPerAmp) / ampsPerVolt);
        if (voltage > 0 && abs(voltage) <= 10) {
          printf("Voltage %f\n", voltage);
          NIDAQManager::outputVoltage(voltage);
        } else {
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos < springStart && readPos > wallPosition) {
      // printf("Voltage 0\n");
      NIDAQManager::outputVoltage(0);
    }
    if (readPos >= springStart && readPos <= springBase) {
      // Calculate spring force
      //.1 meters per volt
      positionToMeters = ((abs(readPos) + 10) * .1) - 1;
      // printf("Displacement: %f\n", positionToMeters);
      springForce = springConstant * positionToMeters;
      // printf("Spring Force: %f\n", springForce);
      if (springForce <= 0 || springForce >= -50) {
        // forceXQueue.push(forceX);
        voltage =
            -1 * (((springForce * radius) / newtonMetersPerAmp) / ampsPerVolt);
        if (voltage > 0 && abs(voltage) <= 10) {
          printf("Voltage %f\n", voltage);
          // send to DAQ as output
          NIDAQManager::outputVoltage(voltage);
        } else {
          printf("Voltage 0.0\n");
          NIDAQManager::outputVoltage(0);
        }
      }
    } else if (readPos < springStart && readPos > wallPosition) {
      printf("Voltage 0\n");
      NIDAQManager::outputVoltage(0);
    }
  }
}

int main() {
  NIDAQManager::readForce();  // DO NOT TOUCH FT SENSOR INITIALLY, THIS
                              // CONFIGURES THE BIAS.

  //Hardware note: motor set to 0.205Arms/V
  //This is equivalent to 10 newtons/V.

  pthread_t visualThread;
  pthread_t wallThread;
  pthread_t springThread;
  pthread_t demoThread;
  pthread_attr_t attr;
  void *status;

  NIDAQManager::initThreads();

  // Main Loop 

  SliderHardware::slider_state_t state1;

  SpringForce f1 = SpringForce(0,2,0); // Weak spring force centered at 0
  DragForce f2 = DragForce(-0.08);
  
  ForceManager::addForce(&f1);
  ForceManager::addForce(&f2);

  while (1) {
    state1.fx = NIDAQManager::readForce();
    state1.position = NIDAQManager::readPosition();

	  double force = ForceManager::computeForce(state1);

    if (force > 10 || force < -10) {
      force = 0;
    }
    NIDAQManager::outputVoltage(force);
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  //pthread_create(&visualThread, &attr, visualDisplay, (void *)1);

  //pthread_join(visualThread, &status);

  pthread_exit(NULL);
}
