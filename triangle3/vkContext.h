#ifndef _vkContext_h_
#define _vkContext_h_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "mesh.h"
#include "utilities.h"

class vkContext
{
public:
  vkContext(GLFWwindow*, bool v = true);
  ~vkContext();

  int initContext();
  void updateModel(glm::mat4 newModel);
  void draw();
  void cleanupContext();


private:
  GLFWwindow* m_pWindow;
  bool        m_useValidation;
  int         m_currentFrame = 0;

  // scene objects
  std::vector<mesh>          m_meshList;

  // scene settings
  struct MVP {
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 model;
  } mvp;

  // Vulkan components
  VkDebugUtilsMessengerEXT    m_messenger;
  VkInstance                  m_instance;
  VkQueue                     m_graphicsQueue;
  VkQueue                     m_presentationQueue;
  VkSurfaceKHR                m_surface;
  VkSwapchainKHR              m_swapchain;
  std::vector<const char*>    m_instanceExtensions = std::vector<const char*>();
  
  std::vector<swapChainImage> m_swapChainImages;
  std::vector<VkFramebuffer>  m_swapChainFrameBuffers;
  std::vector<VkCommandBuffer> m_commandbuffers;

  // descriptor sets
  VkDescriptorSetLayout        m_descriptorSetLayout;
  VkDescriptorPool             m_descriptorPool;
  std::vector<VkDescriptorSet> m_descriptorSets;

  std::vector<VkBuffer>        m_uniformBuffer;
  std::vector<VkDeviceMemory>  m_uniformBufferMemory;

  struct {
    VkPhysicalDevice  physical;
    VkDevice          logical;
  } m_device;

  // pipeline components
  VkPipeline                  m_graphicsPipeline;
  VkPipelineLayout            m_pipelineLayout;
  VkRenderPass                m_renderPass;

  //pools
  VkCommandPool       m_graphicsCommandPool;

  // Utility components
  VkFormat   m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;

  // synchronisation
 std::vector<VkSemaphore>  m_imageAvailable;
 std::vector<VkSemaphore>  m_renderFinished;
 std::vector<VkFence>      m_drawFences;

  // Vulkan functions - create functions
  void createInstance();
  void createDebugMessenger();
  void createLogicalDevice();
  void createSurface();
  void createSwapChain(); 
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSynchronisations();

  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();

  void updateUniformBuffer(uint32_t imageIndex);

  // Vulkan functions -- record functions
  void recordcommands();

  // Vulkan functions - get functions
  void getPhysicalDevice();

  // support functions -- checker functions
  bool checkInstanceExtensionSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice);
  bool checkValidationLayerSupport();
  bool checkDeviceSuitable(VkPhysicalDevice);
  
  // support functions -- getter functions
  queueFamilyIndices getQueueFamilies(VkPhysicalDevice, VkQueueFamilyProperties*);
  SwapChainDetails   getSwapChainDetails(VkPhysicalDevice);

  // choose functions
  VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats); 
  VkPresentModeKHR   chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes); 
  VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities); 

  // generic create functions
  VkImageView    createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags); 
  VkShaderModule createShaderModule(const std::vector<char>& code);
};


#endif 
