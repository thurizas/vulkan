#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLMFORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <iostream>
#include <vector>


#include "vkContext.h"

const std::string windowName = "Vulkan Test Window";
const uint32_t windowWidth = 1366;
const uint32_t windowHeight = 768;


void initWindow(std::string, const uint32_t, const uint32_t height, GLFWwindow**);


/************************************************************************************************************************
 * function  : main
 *
 * abstract  : entry point of the function.  This functions is responsible for setting up the GLFW window, creating the
 *             Vulkan context and subsequently initializing the data structures that Vulkan needs.  This function starts
 *             the window message pump and continues until the window is closed.
 * 
 *             update == added code to change the model matrix causing the images to rotate about the Z-axis.  We use 
 *             a standard construct to ensure the rotation speed is constant regardless of the CPU speed or processor load.
 *
 * parameters: argc -- [in] number of command line arguments
 *             argv -- [in] pointer to a C style string containing the various command line arguments.
 *
 * returns   : int, zero on successful execution, error code and failure.
 *
 * written   : Mar 2024 (GKHuber)
 * modified  : Apr 2024 (GKHuber)
************************************************************************************************************************/
int main(int argc, char** argv)
{
  GLFWwindow* window = nullptr;

  initWindow(windowName, windowWidth, windowHeight, &window);

  vkContext    ctx(window, true);      // NOTE: the false turns off validation
  if (EXIT_SUCCESS == ctx.initContext())
  {
    float  angle = 0.0f;                // angle that the image should be rotated through
    float  deltaTime = 0.0f;            // time elapse since the last image was drawn
    float  lastTime = 0.0f;             // time last render occured at.

    int helicopter = ctx.createMeshModel("./Models/uh60.obj");

    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();

      float  now = glfwGetTime();              // current time, in seconds of the GLFW timer 

      deltaTime = now - lastTime;              // calculate elapsed time
      lastTime = now;                          // update last rendered time

      angle += 10.0f * deltaTime;              // update angle, scaled to elapsed time
      if (angle > 360.0f) { angle -= 360.0; }  // clamp angle in range [0,360)

      glm::mat4 testMat = glm::scale(glm::mat4(1.0), glm::vec3(0.4f, 0.4f, 0.4f));
      testMat = glm::rotate(testMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      //testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
      testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
      ctx.updateModel(helicopter, testMat);

      ctx.draw();
    }

    ctx.cleanupContext();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  return 0;
}

void initWindow(std::string name, const uint32_t width, const uint32_t height, GLFWwindow** ppWindow)
{
  int ret = glfwInit();
  if (ret == GLFW_TRUE)
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);                  // tell GLFW not to use OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);                    // make window non-resizeable

    *ppWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (*ppWindow == nullptr)
    {
      std::cerr << "Failed to create GLFW window" << std::endl;
    }
  }
  else
  {
    *ppWindow = nullptr;
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }
}
