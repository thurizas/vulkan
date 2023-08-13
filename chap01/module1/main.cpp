#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLMFORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>
#include <string>

#include "vkCtx.h"

#ifdef __WIN32
    #include "XGetopt.h"
#else
    #include <unistd.h>
#endif

const uint32_t width = 800;
const uint32_t height = 600;

bool initWindow(GLFWwindow**);
void delWindow(GLFWwindow*);

void glfwErrorHndr(int, const char*);                       // error handler for GLFW errors

int main(int argc, char** argv)
{
  bool                       debugMode = false;
  uint32_t                   debugLvl = 3;          // only print out error or fatal notices
  int                        choice = -1;
  std::vector<std::string>   requistedLayers;
  GLFWwindow*                window = nullptr;
  uint32_t                   devProperties = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
  uint64_t                   devFeatures = 0;

  while (-1 != (choice = getopt(argc, argv, "dl:")))
  {
    switch (choice)
    {
      case 'd':                              // enabling debugging - add validation layer by default.
        if (!debugMode)                      // only setup debugging on first '-d' flage
        {
          debugMode = !debugMode;
          requistedLayers.push_back("VK_LAYER_KHRONOS_validation");
        }
        debugLvl--;
        if (debugLvl < 1) debugLvl = 1;      // clamp debugLvl between [1,6]
        if (debugLvl > 6) debugLvl = 6;
        break;

      case 'l':
      {
        std::string argList = std::string(optarg);
        std::string::size_type nLoc = -1;
        while (-1 != (nLoc = argList.find(",")))
        {
          std::string layer = argList.substr(0, nLoc);
          argList.erase(0, nLoc+1);
          // TODO : make sure layer being added is unique in list
          requistedLayers.push_back(layer);
        }
        
        if (argList.size() > 0)      // no comma found & still have a string -- push entire string
        {
          requistedLayers.push_back(argList);
        }
        break;
      }

      case '?':
        std::cout << "unrecognized command line option " << argv[optind] << std::endl;
      case 'h':            // fallthrough expected
      {
        std::cout << argv[0] << " test program for Vulkan" << std::endl;
        std::cout << "usage: " << argv[0] << "options" << std::endl;
        std::cout << "\nOptions are: " << std::endl;
        std::cout << "-l layers          layers is a comman delimited list of layers to use" << std::endl;
        std::cout << "-d                 toggles the debug flag" << std::endl;
        exit(0);
      }
    }
  }



  GLFWerrorfun oldHandler = glfwSetErrorCallback(glfwErrorHndr);  // register our custrom error handler
  if (initWindow(&window))                                        // create and instantiate the GLFW window
  {
    vkCtx theApp(&requistedLayers, debugMode);

    if (VK_SUCCESS == theApp.init())
    {
       uint32_t device = theApp.findSuitableDevice(devProperties | (devFeatures << 3));
      if (device >= 0)                                            // found a suitable device
      {
        if (theApp.createLogicalDevice(device))
        {

          while (!glfwWindowShouldClose(window))                    // rendering loop
          {
            glfwPollEvents();
          }


        }
        else
        {
          std::cout << "[-] Failed to create a graphics logical device" << std::endl;
        }
      }
      else
      {
        std::cout << "[-] Failed to find a suitable device" << std::endl;
      }
    }
    else
    {
      std::cout << "[-] Failed to initialize a vulkan context" << std::endl;
    }
  }
        
  glfwSetErrorCallback(oldHandler);                         // restore the original error handler
  if(nullptr != window)  delWindow(window);                 // GLFW window has to be destroyed after Vulkan instance is destroyed


  return 0;
}


bool initWindow(GLFWwindow** ppWindow)
{
  bool bRet = false;
  glfwInit();

  if (*ppWindow != nullptr) glfwDestroyWindow(*ppWindow);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  *ppWindow = glfwCreateWindow(width, height, "module 1 - Vulkan test", nullptr, nullptr);

  if (nullptr != *ppWindow)
  {
    bRet = true;
    std::cout << "[+] successfully created GLFW window" << std::endl;
  }
  else
  {
    std::cout << "[-] failed to create GLFW window" << std::endl;

  }

  return bRet;
}

void delWindow(GLFWwindow* pWindow)
{
  glfwDestroyWindow(pWindow);
  glfwTerminate();
}

void glfwErrorHndr(int code, const char* desc)
{
  std::cout << "[-] Error in GLFW, code: " << code << " description: " << desc << std::endl;
}
