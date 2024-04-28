#ifndef _vkContext_h_
#define _vkContext_h_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "stb_image.h"

#include "mesh.h"
#include "MeshModel.h"
#include "utilities.h"

class vkContext
{
public:
  vkContext(GLFWwindow*, bool v = true);
  ~vkContext();

  int initContext();

  int  createMeshModel(std::string modelFile);
  void updateModel(int modelID, glm::mat4 newModel);
  void draw();
  void cleanupContext();


private:
  GLFWwindow* m_pWindow;
  bool        m_useValidation;
  int         m_currentFrame = 0;

  // scene objects
  std::vector<MeshModel>          m_modelList;

  // scene settings
  struct UboVP {
    glm::mat4 proj;
    glm::mat4 view;
  } m_uboVP;

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

  VkImage                      m_depthBufferImage;
  VkDeviceMemory               m_depthBufferImageMemory;
  VkImageView                  m_depthBufferImageView;

  VkSampler                    m_textureSampler;

  // descriptor sets
  VkDescriptorSetLayout        m_descriptorSetLayout;
  VkDescriptorSetLayout        m_samplerSetLayout;
  VkPushConstantRange          m_pushConstantRange;

  VkDescriptorPool             m_descriptorPool;
  VkDescriptorPool             m_samplerDescriptorPool;
  std::vector<VkDescriptorSet> m_descriptorSets;
  std::vector<VkDescriptorSet> m_samplerDescriptorSets;

  std::vector<VkBuffer>        m_vpUniformBuffer;
  std::vector<VkDeviceMemory>  m_vpUniformBufferMemory;

  std::vector<VkBuffer>        m_modelDUniformBuffer;
  std::vector<VkDeviceMemory>  m_modelDUniformBufferMemory;
  std::vector<VkImageView>     m_textureImageViews;

  //VkDeviceSize                 m_minUniformBufferOffset;
  //size_t                       m_modelUniformAlignment;
  //UboModel*                    m_modelTransferSpace;

  struct {
    VkPhysicalDevice  physical;
    VkDevice          logical;
  } m_device;

  // asset handling
  std::vector<VkImage>        m_textureImages;
  std::vector<VkDeviceMemory> m_textureImageMemory;

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
  void createPushConstantRange();
  void createGraphicsPipeline();
  void createDepthBufferImage();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSynchronisations();
  void createTextureSampler();

  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();

  void updateUniformBuffers(uint32_t imageIndex);

  // Vulkan functions -- record functions
  void recordcommands(uint32_t imageIndex);

  // Vulkan functions - get functions
  void getPhysicalDevice();

  // Allocate Functions

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
  VkFormat           chooseSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);
  VkImage            createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkDeviceMemory*);


  // generic create functions
  VkImageView    createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags); 
  VkShaderModule createShaderModule(const std::vector<char>& code);
  int            createTextureImage(std::string fileName);
  int            createTexture(std::string fileName);
  int            createTextureDescriptor(VkImageView textureImage);

  // loader functions
  stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
};


#endif 
