

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

int main(int argc, char** argv)
{
  bool                       debugMode = false;
  uint32_t                   debugLvl = 3;                  // only print out error or fatal notices
  int                        choice = -1;
  std::vector<std::string>   requestedLayers;
  GLFWwindow*                window = nullptr;
  vkProperties               properties{ VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU | VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,0 };
 


  while (-1 != (choice = getopt(argc, argv, "dl:e:")))
  {
    switch (choice)
    {
      case 'd':                              // enabling debugging - add validation layer by default.
        if (!debugMode)                      // only setup debugging on first '-d' flage
        {
          debugMode = !debugMode;
          requestedLayers.push_back("VK_LAYER_KHRONOS_validation");
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
          requestedLayers.push_back(layer);
        }
        
        if (argList.size() > 0)      // no comma found & still have a string -- push entire string
        {
          requestedLayers.push_back(argList);
        }
      }
      break;
      
      case 'e':                              // handling extensions from command line.
      {
        std::string argList = std::string(optarg);
      }
      break;

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



  GLFWerrorfun oldHandler = glfwSetErrorCallback(glfwErrorHndr);     // register our custrom error handler
  if (initWindow(&window))                                           // create and instantiate the GLFW window
  {  
    // we are using GLFW, which has a list of required extension...this needs to be called after the GLFW window is created
    uint32_t glfwExtensionCnt = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);
    std::vector<const char*> requestedExts(glfwExtensions, glfwExtensions + glfwExtensionCnt);


    uint32_t device = 0;
    vkCtx theApp(&requestedLayers, &requestedExts, window, debugMode);

    if (VK_SUCCESS == theApp.init(properties,&device))
    {
      if (debugMode)                                                  // enable custom logging of layer outpue
      {
        VkResult res = theApp.createDebugMessenger(debugCallback);
        
        if (res == VK_SUCCESS)
        {
          std::cout << "[+] installed custom layer messenger handler" << std::endl;
        }
        else
        {
          std::cout << "[-] failed to install custom layer messenger handler, error is :" << res << " did you add extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME?" << std::endl;
        }
      }

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

      if (debugMode)
      {
        theApp.destroyDebugMessenger();
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


