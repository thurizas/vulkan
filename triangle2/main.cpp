#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLMFORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <stdexcept>
#include <iostream>
#include <vector>


#include "vkContext.h"

const std::string windowName = "Vulkan Test Window";
const uint32_t windowWidth = 800;
const uint32_t windowHeight = 600;


void initWindow(std::string, const uint32_t, const uint32_t height, GLFWwindow**);

int main(int argc, char** argv)
{
  GLFWwindow* window = nullptr;

  initWindow(windowName, windowWidth, windowHeight, &window);

  vkContext    ctx(window, true);
  if (EXIT_SUCCESS == ctx.initContext())
  {
    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();
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
