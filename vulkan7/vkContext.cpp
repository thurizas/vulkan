#include <iostream>
#include <set>

#include "vkContext.h"
#include "vkValidations.h"
#include "mesh.h"


/************************************************************************************************************************
 * function  : ctor
 *
 * abstract  : Constructs an instance of the vulkan context.  We perform the following tasks,
 *               (a) get a list of the extensions required by GLFW
 *               (b) add the requested layers to a list of required layers
 *               (c) check that requested extensions are supported - if not throw runtime exception
 *               (d) check that requested layers are supported - if not disable layer support
 *
 * parameters: pWindow -- [in] pointer to the GLFW window we will be rendering to
 *
 * returns   : nothin
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
vkContext::vkContext(GLFWwindow* pWindow, bool v) : m_pWindow(pWindow), m_useValidation(v), m_device({ nullptr, nullptr })
{
  uint32_t glfwExtensionCnt = 0;
  const char** glfwExtensions;       

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);
  for (size_t ndx = 0; ndx < glfwExtensionCnt; ndx++)
  {
    m_instanceExtensions.push_back(glfwExtensions[ndx]);
    std::cout << "[?] adding " << glfwExtensions[ndx] << " to list of required extenstions" << std::endl;
  }

  // add VK_EXT_debug_utils to our list of required extensions, if debugging is enabled.
  if (m_useValidation)
  {
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (!checkInstanceExtensionSupport())
  {
    std::cerr << "[-] VkInstance does not support required extension" << std::endl;
    throw std::runtime_error("[-] VkInstance does not support required extension");
  }

  if (!checkValidationLayerSupport())
  {
    std::wcerr << "[-] VkInstance does not support a requested validation layer" << std::endl;
    //throw std::runtime_error("[-] VKInstance does not support a requested validation layer");
    m_useValidation = false;
  }
}

/************************************************************************************************************************
 * function  : dtor
 *
 * abstract  : Destroys the instance of the vulkan context
 *
 * parameters: void
 *
 * returns   : nothing
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
vkContext::~vkContext()
{

}



/************************************************************************************************************************
 * function  : initContext
 *
 * abstract  : The function constructs the actual instance of the context.  The following steps are performed:
 *               (a) create the vulkan instance
 *               (b) create a debug messenger to display messages from various layers
 *               (c) create surface that we will be rendering to
 *               (d) enumerate the physical devices and gets their properties
 *               (e) create a logical device to use as an interface to the selected physical device 
 *               (g) create swapchain and the images that the swapchain will use
 *               (h) create the render pass 
 *               (i) create the descriptor layout
 *               (j) create push-constants
 *               (k) create graphics pipeline
 *               (l) create depth buffer image
 *               (m) create the framebuffer
 *               (n) create the command pool
 *               (o) set up view and projection matricies, create mesh(s), and create index buffers that are used in the scene
 *               (p) create command buffers
 *               (q) createUniformBuffers();
 *               (r) createDescriptorPools();
 *               (s) createDescriptorSets();
 *               (t) record commands to render the scene
 *               (u) create synchronization objects to use along the graphics pipeline
 *            If any of these steps fails, it will throw a runtime exception and the program will terminate.
 *
 * parameters: void
 *
 * returns   : void.  Throws a runtime exception on error
 *
 * written   : Mar 2024 (GKHuber)
 * modified  : Apr 2024 (GKHuber) added support for descriptor sets and modifying the MVP matrix at run time.
 *                                added support for dynamic descriptor sets and push-constants
 *                                added support for depth testing
************************************************************************************************************************/
int vkContext::initContext()
{
  try
  {
    createInstance();
    createDebugMessenger();
    createSurface();
    getPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createRenderPass();
    createDescriptorSetLayout();
    createPushConstantRange();
    createGraphicsPipeline();
    createColourBufferImage();
    createDepthBufferImage();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createTextureSampler();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createInputDescriptorSets();
    createSynchronisations();

    m_uboVP.proj = glm::perspective(glm::radians(45.0f), (float)m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 100.0f);
    m_uboVP.view = glm::lookAt(glm::vec3(10.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //m_uboVP.view = glm::lookAt(glm::vec3(-2.5f, 0.0f, 3.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_uboVP.proj[1][1] *= -1;               // vulkan reverses the direction of the y-axis

    // create our default "no-texture" texture
    createTexture("plain.png");
  }
  catch (const std::runtime_error& e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}



void vkContext::updateModel(int modelId, glm::mat4 newModel)
{
  if (modelId >= m_modelList.size()) return;
  m_modelList[modelId].setModel(newModel);
}



/************************************************************************************************************************
 * function  : draw 
 *
 * abstract  : Function to actually perform the drawing of the image, we perform the following steps
 *              (a) wait for previous draw operation to complete -- wait for all fences to be cleared
 *              (b) get next image to render to
 *              (c) submit the command buffer to a command queue
 *              (d) submit finished image to the present queue to show on screen
 *             This functions throws a runtime exception if it fails
 * 
 * parameters: none
 *
 * returns   : void, throws runtime exceptions
 *
 * written   : Mar 2024 (GKHuber)
 * modified  : Apr 2024 (GKHuber) added support for uniform buffers - added code to update the uniforms
************************************************************************************************************************/
void vkContext::draw()
{
  vkWaitForFences(m_device.logical, 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
  vkResetFences(m_device.logical, 1, &m_drawFences[m_currentFrame]);
  
  uint32_t imageIndex;
  vkAcquireNextImageKHR(m_device.logical, m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

  recordcommands(imageIndex);
  updateUniformBuffers(imageIndex);

  // submit command buffer to graphics queue....
  VkSubmitInfo  submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_imageAvailable[m_currentFrame];
  
  VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandbuffers[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderFinished[m_currentFrame];

  VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_drawFences[m_currentFrame]);
  if (VK_SUCCESS != result)
  {
    throw std::runtime_error("failed to command to graphics queue");
  }

  // submit finished image to presentation queue...
  VkPresentInfoKHR  presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_renderFinished[m_currentFrame];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapchain;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);
  if (VK_SUCCESS != result)
  {
    throw std::runtime_error("failed to present image to presentation queue");
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;
}



/************************************************************************************************************************
 * function  : cleanupContext
 *
 * abstract  : This function destroys all the Vulkan objects that we created.  Things are destroyed in reverse order that 
 *             they were completed.  Prior to this function be called, it is necessary to make sure that all command queues
 *             have been emptyed (use vkDeviceWaitIdle) otherwise we will get validation errors about attempting to destroy
 *             objects that are in use, as well as not destroying objects.
 *
 * parameters: nothins
 *
 * returns   : void 
 *
 * written   : Mar 2024 (GKHuber)
 ** modified  : Apr 2024 (GKHuber) added cleanup code for descriptor sets
************************************************************************************************************************/
void vkContext::cleanupContext()
{
  vkDeviceWaitIdle(m_device.logical);
  
  //_aligned_free(m_modelTransferSpace);

  for (size_t i = 0; i < m_modelList.size(); i++)
  {
    m_modelList[i].destroyMeshModel();
  }

  vkDestroyDescriptorPool(m_device.logical, m_inputDescriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device.logical, m_inputSetLayout, nullptr);

  vkDestroyDescriptorPool(m_device.logical, m_samplerDescriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device.logical, m_samplerSetLayout, nullptr);

  vkDestroySampler(m_device.logical, m_textureSampler, nullptr);

  for (size_t i = 0; i < m_textureImages.size(); i++)
  {
    vkDestroyImageView(m_device.logical, m_textureImageViews[i], nullptr);
    vkDestroyImage(m_device.logical, m_textureImages[i], nullptr);
    vkFreeMemory(m_device.logical, m_textureImageMemory[i], nullptr);
  }

  for (size_t i = 0; i < m_depthBufferImage.size(); i++)
  {
    vkDestroyImageView(m_device.logical, m_depthBufferImageView[i], nullptr);
    vkDestroyImage(m_device.logical, m_depthBufferImage[i], nullptr);
    vkFreeMemory(m_device.logical, m_depthBufferImageMemory[i], nullptr);
  }

  for (size_t i = 0; i < m_colourBufferImage.size(); i++)
  {
    vkDestroyImageView(m_device.logical, m_colourBufferImageView[i], nullptr);
    vkDestroyImage(m_device.logical, m_colourBufferImage[i], nullptr);
    vkFreeMemory(m_device.logical, m_colourBufferImageMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(m_device.logical, m_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device.logical, m_descriptorSetLayout, nullptr);
  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    vkDestroyBuffer(m_device.logical, m_vpUniformBuffer[i], nullptr);
    vkFreeMemory(m_device.logical, m_vpUniformBufferMemory[i], nullptr);
  }

  for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
  {
    vkDestroySemaphore(m_device.logical, m_renderFinished[i], nullptr);
    vkDestroySemaphore(m_device.logical, m_imageAvailable[i], nullptr);
    vkDestroyFence(m_device.logical, m_drawFences[i], nullptr);
  }

  vkDestroyCommandPool(m_device.logical, m_graphicsCommandPool, nullptr);
  for (auto framebuffer : m_swapChainFrameBuffers)
  {
    vkDestroyFramebuffer(m_device.logical, framebuffer, nullptr);
  }

  vkDestroyPipeline(m_device.logical, m_secondPipeline, nullptr);
  vkDestroyPipelineLayout(m_device.logical, m_secondPipelineLayout, nullptr);
  vkDestroyPipeline(m_device.logical, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_device.logical, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_device.logical, m_renderPass, nullptr);
  for (auto image : m_swapChainImages)
  {
    vkDestroyImageView(m_device.logical, image.imageView, nullptr);
  }

  vkDestroySwapchainKHR(m_device.logical, m_swapchain, nullptr);
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyDevice(m_device.logical, nullptr);
  if(m_useValidation) DestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
  vkDestroyInstance(m_instance, nullptr);
}



/************************************************************************************************************************
 * function  : createInstance
 *
 * abstract  : a Vulkan instance is used for various meta-information about the application - name, version number, number
 *             of extensions required and number of layers being used (assuming validation is being performed).  While 
 *             not explicitly involved in the rendering, the instance must be the first thing created (and the last to 
 *             be destroyed)
 *
 * parameters: nothing
 *
 * returns   : void, throws runtime exception is instance creation fails.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createInstance()
{
  VkApplicationInfo appInfo = { };
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Test Vulkan App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(m_instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = m_instanceExtensions.data();

  if (m_useValidation)
  {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr;

    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result == VK_SUCCESS)
  {
    std::cout << "[+] Instance created successfully" << std::endl;
  }
  else
  {
    throw std::runtime_error("Failed to create Vulkan Instance");
  }
}



/************************************************************************************************************************
 * function  : createDebugMessenger
 *
 * abstract  : This creates a debug messenger that delievers messages from the validation layers to a specialized output
 *             function so the messages are displayed. 
 *
 * parameters: none
 *
 * returns   : void, throws runtime exception if an error occurs
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createDebugMessenger()
{
  if (!m_useValidation) return;
    
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;

  //populateDebugMessengerCreateInfo(createInfo);
  VkResult result = CreateDebugMessengerExt(m_instance, &createInfo, nullptr, &m_messenger);
  
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] sucessfully created debug messenger" << std::endl;
  }
  else
  {
    std::wcerr << "[-] failed to create debug messenger" << std::endl;
    throw std::runtime_error("failed to set up debug messenger!");
  }
}



/************************************************************************************************************************
 * function  : createLogicalDevice
 *
 * abstract  : This function creates a logical device, an abstraction of the actual physical device, and is used to pass
 *             commands down to the physical device.  The following tasks are performed
 *               (a) get the queue families that the physical device supports and stores the indicies of the required 
 *                   queue familys in the structure `queueFamilyIndices'.  The types of queue are determined by the needs
 *                   of the application.  This application requires graphics, transfer, and presentation queues.  Note 
 *                   that the Vulkan standard requires that a graphics queue supports a transfre queue
 *               (b) creates a 'vkDeviceQueueCreateInfo' for each queue needed, and builds a buffer of required queues, this
 *                   is passed on the the logical device create function.  The logical device create function will take
 *                   care of creating the necessary queues => we do not need to destroy the queues.
 *               (c) also, if the application requires any specific features from the physical device, the physical device
 *                   is queried to ensure that those features are supported.  This application does not require any
 *                   special features.
 *
 * parameters: none
 *
 * returns   : void, throws runtime exception if an error happens.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createLogicalDevice()
{
  queueFamilyIndices indices = getQueueFamilies(m_device.physical, nullptr);
  
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<int> qfi = { indices.graphicsFamily, indices.presentationFamily };

  for (int queueFamilyIndex : qfi)
  {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;

    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								
  deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());	// Number of enabled logical device extensions
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							          // List of enabled logical device extensions
  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = true;                                                  // enable anisotropy

  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;			

  VkResult result = vkCreateDevice(m_device.physical, &deviceCreateInfo, nullptr, &m_device.logical);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Logical Device");
  }
  else
  {
    std::cerr << "[+] logical device created." << std::endl;

    vkGetDeviceQueue(m_device.logical, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device.logical, indices.presentationFamily, 0, &m_presentationQueue);
  }
}



/************************************************************************************************************************
 * function  : createSurface
 *
 * abstract  : This function will create the surface will be rendered to.  This is highly dependant on the window manager
 *             that we are using (GLFW), so we just us a convience function from GLFW to create a correct surface
 *
 * parameters: none
 *
 * returns   : void, throws a runtime error on failure
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createSurface()
{
  VkResult result = glfwCreateWindowSurface(m_instance, m_pWindow, nullptr, &m_surface);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] surface created successfully" << std::endl;
  }
  else
  {
    throw std::runtime_error("Failed to create a rendering surface");
  }
}



/************************************************************************************************************************
 * function  : createSwapchain
 *
 * abstract  : THe swapchain is a circular buffer of images that are rendered to, and presented from.  At its simplese,
 *             one image is being presented while the second is be rendered to (double-buffering).  With Vulkan there 
 *             can be an arbitrary (with in reason) number of images.  The following steps are preformaed
 *                (a) get the details of the swapchain as defined by the physical device
 *                (b) choose the best surface format the window manager supplies based on what the device supports.
 *                (c) choose the best presenatation mode that the window manager supports base on that the device does.
 *                (d) get the extent (dimensions) of the images
 *                (e) determine the minimum and maximum number of images that the swap chain must have 
 *                (f) if the swap chain was created sucessfully, create an array of images that will be used with
 *                    the swap chain.
 *
 * parameters: none
 *
 * returns   : void, throws run-time exception on error.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createSwapChain()
{
  SwapChainDetails swapChainDetails = getSwapChainDetails(m_device.physical);

  VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
  VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
  VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

  // add 1 to allow for triple buffering....
  uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

  // if imageCount > max, clamp to max; if imageCount == 0 then infinite images.
  if (swapChainDetails.surfaceCapabilities.maxImageArrayLayers > 0 && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
  {
    imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = m_surface;
  swapChainCreateInfo.imageFormat = surfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapChainCreateInfo.presentMode = presentMode;
  swapChainCreateInfo.imageExtent = extent;
  swapChainCreateInfo.minImageCount = imageCount;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.clipped = VK_TRUE;

  // if graphics and presentation are on different queues, need to enable sharing.
  queueFamilyIndices indicies = getQueueFamilies(m_device.physical, nullptr);
  if (indicies.graphicsFamily != indicies.presentationFamily)
  {
    uint32_t queueFamilyIndices[] = { (uint32_t)indicies.graphicsFamily, (uint32_t)indicies.presentationFamily };
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  }

  swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result = vkCreateSwapchainKHR(m_device.logical, &swapChainCreateInfo, nullptr, &m_swapchain);
  if (result == VK_SUCCESS)
  {
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    std::cerr << "[+] successfully created swapchain" << std::endl;

    uint32_t swapChainImageCount;

    vkGetSwapchainImagesKHR(m_device.logical, m_swapchain, &swapChainImageCount, nullptr);
    std::vector<VkImage> images(swapChainImageCount);

    vkGetSwapchainImagesKHR(m_device.logical, m_swapchain, &swapChainImageCount, images.data());
    for (VkImage image : images)
    {
      swapChainImage SwapChainImage = {};
      SwapChainImage.image = image;
      SwapChainImage.imageView = createImageView(image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

      m_swapChainImages.push_back(SwapChainImage);
    }
    std::cerr << "[+]   created " << swapChainImageCount << " images for swapchain" << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to create swapchain" << std::endl;
    throw std::runtime_error("failed to create a swapchain");
  }
}



/************************************************************************************************************************
 * function  : createRenderPass
 *
 * abstract  : The render pass is used to describe how the image is rendered.  Along the render pass the image is transformed
 *             from on format to another depending on what stage the render pass is in.  We can defince actions that must 
 *             occur on load (image is being loaded) or store (image is being written too memory).  We can also define 
 *             subpasses that are used to inforce temporal constraints - i.e. action 1 must occure befor action 2 starts.
 *
 * parameters: none
 *
 * returns   : void, thows run time exception on error
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createRenderPass()
{
  // array of our subpasses
  std::array<VkSubpassDescription, 2> subpasses{};

  // attachments
  // subpass 1: attachments & references (input attachments)

  // color attachment
  VkAttachmentDescription  colorAttachment = {};
  colorAttachment.format = chooseSupportedFormat({ VK_FORMAT_R8G8B8A8_UNORM }, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  //depth attachment
  VkAttachmentDescription   depthAttachment = {};
  const std::vector<VkFormat> requestedFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
  depthAttachment.format = chooseSupportedFormat(requestedFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // REFERENCES
  // attachment reference (by index) to the attachment list used in renderPassCreateInfo
  VkAttachmentReference colorAttachmentReference = {};
  colorAttachmentReference.attachment = 1;
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentReference = {};
  depthAttachmentReference.attachment = 2;
  depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount = 1;
  subpasses[0].pColorAttachments = &colorAttachmentReference;
  subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

  // subpass 2 - attachments & references
    // Swapchain colour attachment
  VkAttachmentDescription swapchainColourAttachment = {};
  swapchainColourAttachment.format = m_swapChainImageFormat;					// Format to use for attachment
  swapchainColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
  swapchainColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
  swapchainColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// Describes what to do with attachment after rendering
  swapchainColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
  swapchainColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;// Describes what to do with stencil after rendering

  // Framebuffer data will be stored as an image, but images can be given different data layouts
  // to give optimal use for certain operations
  swapchainColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
  swapchainColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

  // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
  VkAttachmentReference swapchainColourAttachmentReference = {};
  swapchainColourAttachmentReference.attachment = 0;
  swapchainColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // References to attachments that subpass will take input from
  std::array<VkAttachmentReference, 2> inputReferences;
  inputReferences[0].attachment = 1;
  inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  inputReferences[1].attachment = 2;
  inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Set up Subpass 2
  subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[1].colorAttachmentCount = 1;
  subpasses[1].pColorAttachments = &swapchainColourAttachmentReference;
  subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
  subpasses[1].pInputAttachments = inputReferences.data();


  // subpass dependencies
  // Need to determine when layout transitions occur using subpass dependencies
  std::array<VkSubpassDependency, 3> subpassDependencies;

  // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  // Transition must happen after...
  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
  subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
  subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
  // But must happen before...
  subpassDependencies[0].dstSubpass = 0;
  subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = 0;

  // Subpass 1 layout (colour/depth) to Subpass 2 layout (shader read)
  subpassDependencies[1].srcSubpass = 0;
  subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[1].dstSubpass = 1;
  subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  subpassDependencies[1].dependencyFlags = 0;

  // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  // Transition must happen after...
  subpassDependencies[2].srcSubpass = 0;
  subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
  // But must happen before...
  subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[2].dependencyFlags = 0;

  std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColourAttachment, colorAttachment, depthAttachment };

  // Create info for Render Pass
  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
  renderPassCreateInfo.pAttachments = renderPassAttachments.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
  renderPassCreateInfo.pSubpasses = subpasses.data();
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassCreateInfo.pDependencies = subpassDependencies.data();

  VkResult result = vkCreateRenderPass(m_device.logical, &renderPassCreateInfo, nullptr, &m_renderPass);
  if (VK_SUCCESS == result)
  {
    std::cerr << "[+] render pass created" << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to create render pass" << std::endl;
    throw std::runtime_error("failed to create render pass");
  }
}



/************************************************************************************************************************
 * function  : createDescriptorSetLayout
 *
 * abstract  :
 *
 * parameters: void 
 *
 * returns   : void, throws exception on error
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createDescriptorSetLayout()
{
  // Uniform values descriptor set layout
  // UboViewProjection Binding Info
  VkDescriptorSetLayoutBinding vpLayoutBinding = {};
  vpLayoutBinding.binding = 0;											                  // Binding point in shader (designated by binding number in shader)
  vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
  vpLayoutBinding.descriptorCount = 1;									              // Number of descriptors for binding
  vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				    // Shader stage to bind to
  vpLayoutBinding.pImmutableSamplers = nullptr;							          // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

  std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

  // Create descriptor set layout with given bindings
  VkDescriptorSetLayoutCreateInfo  layoutCreateInfo = {};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutCreateInfo.pBindings = layoutBindings.data();

  VkResult result = vkCreateDescriptorSetLayout(m_device.logical, &layoutCreateInfo, nullptr, &m_descriptorSetLayout);
  if (result != VK_SUCCESS)
  {
    std::cerr << "[-] failed to create descriptor set layout" << std::endl;
    throw std::runtime_error("failed to create descriptor set layout");
  }

  // create texture sample descriptor set layout
  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  // create a descriptorset layout with given bindings for texture
  VkDescriptorSetLayoutCreateInfo  textureLayoutCreateInfo = {};
  textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  textureLayoutCreateInfo.bindingCount = 1;
  textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

  result = vkCreateDescriptorSetLayout(m_device.logical, &textureLayoutCreateInfo, nullptr, &m_samplerSetLayout);
  if (result != VK_SUCCESS)
  {
    std::cerr << "[-] failed to create sampler description set layout";
    throw std::runtime_error("failed to create smaple description set layout");
  }

  // CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
  // Colour Input Binding
  VkDescriptorSetLayoutBinding colourInputLayoutBinding = {};
  colourInputLayoutBinding.binding = 0;
  colourInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  colourInputLayoutBinding.descriptorCount = 1;
  colourInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // Depth Input Binding
  VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
  depthInputLayoutBinding.binding = 1;
  depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  depthInputLayoutBinding.descriptorCount = 1;
  depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // Array of input attachment bindings
  std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colourInputLayoutBinding, depthInputLayoutBinding };

  // Create a descriptor set layout for input attachments
  VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
  inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
  inputLayoutCreateInfo.pBindings = inputBindings.data();

  // Create Descriptor Set Layout
  result = vkCreateDescriptorSetLayout(m_device.logical, &inputLayoutCreateInfo, nullptr, &m_inputSetLayout);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Descriptor Set Layout!");
  }
}



/************************************************************************************************************************
 * function  : createPushConstantRange
 *
 * abstract  : Initialized the push constant range structure.  We need to define the stage of the pipeline that will have
 *             access to the push constant and it's size.
 *
 * parameters: void 
 *
 * returns   : void
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createPushConstantRange()
{
  m_pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  m_pushConstantRange.offset = 0;
  m_pushConstantRange.size = sizeof(Model);
}



/************************************************************************************************************************
 * function  : createGraphicsPipeline
 *
 * abstract  : This functions specifies the layout of the graphics pipeline - both the programable stages and the fixed-
 *             function stages.  The following operations are preformed
 *               (a) pre-compiled shaders (compiled by glslangValidator.exe to *.spv files) are loaded and modules are 
 *                   created for each of them
 *               (b) create create-info structures for each compone of the pipeline, these are
 *                   - vertex input (will be modified to support VAO/VBO/IBO later
 *                   - inpu assembly
 *                   - viewport and scissor
 *                   - rasterizer
 *                   - multisampling
 *                   - blending
 *               (c) create the graphics pipline layout.  Will be modified to support decriptor sets, push constants 
 *                   later
 *               (d) create the graphics pipeline.  The create info structure will use points to all of the above create 
 *                   info structures we defined -- thus destroying the pipeline will destroy all of the subordinate stages
 *
 * parameters: none
 *
 * returns   : void, throws run-time exception on error
 *
 * written   : Mar 2024 (GKHuber)
 *             Apr 2024 (GKHuber) add support for depth testing
************************************************************************************************************************/
void vkContext::createGraphicsPipeline()
{
  auto vertexShaderCode = readFile("./Shaders/vert.spv");
  auto fragmentShaderCode = readFile("./Shaders/frag.spv");

  VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
  VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

  VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
  vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderCreateInfo.module = vertexShaderModule;
  vertexShaderCreateInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
  fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderCreateInfo.module = fragmentShaderModule;
  fragmentShaderCreateInfo.pName = "main";

  // need an array of modules...
  VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

  // how the data for a single vertex is as a whole
  VkVertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;        // read a single vertex at a time

  std::array<VkVertexInputAttributeDescription, 3> attributeDescription;
  attributeDescription[0].binding = 0;
  attributeDescription[0].location = 0;
  attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[0].offset = offsetof(vertex, pos);

  attributeDescription[1].binding = 0;
  attributeDescription[1].location = 1;
  attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[1].offset = offsetof(vertex, col);

  attributeDescription[2].binding = 0;
  attributeDescription[2].location = 2;
  attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescription[2].offset = offsetof(vertex, tex);

  // build all the components for the pipeline
  // -- VERTEX INPUT
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
  vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
  vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;			          // TODO: List of Vertex Binding Descriptions (data spacing/stride information)
  vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
  vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();		          // TODO: List of Vertex Attribute Descriptions (data format and where to bind to/from)

  // -- INPUT ASSEMBLY --
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		
  inputAssembly.primitiveRestartEnable = VK_FALSE;					

  // -- VIEWPORT & SCISSOR --
  VkViewport viewport = {};
  viewport.x = 0.0f;									                // x start coordinate
  viewport.y = 0.0f;									                // y start coordinate
  viewport.width = (float)m_swapChainExtent.width;		// width of viewport
  viewport.height = (float)m_swapChainExtent.height;	// height of viewport
  viewport.minDepth = 0.0f;							              // min framebuffer depth
  viewport.maxDepth = 1.0f;							              // max framebuffer depth

  // Create a scissor info struct
  VkRect2D scissor = {};
  scissor.offset = { 0,0 };							
  scissor.extent = m_swapChainExtent;					

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
  viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateCreateInfo.viewportCount = 1;
  viewportStateCreateInfo.pViewports = &viewport;
  viewportStateCreateInfo.scissorCount = 1;
  viewportStateCreateInfo.pScissors = &scissor;

  // -- RASTERIZER --
  VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
  rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerCreateInfo.depthClampEnable = VK_FALSE;			            // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
  rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;	        // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
  rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	        // How to handle filling points between vertices
  rasterizerCreateInfo.lineWidth = 1.0f;						                // How thick lines should be when drawn
  rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		        // Which face of a tri to cull
  rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
  rasterizerCreateInfo.depthBiasEnable = VK_FALSE;			            // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

  // -- MULTISAMPLING --
  VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
  multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
  multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

  // -- BLENDING --
  VkPipelineColorBlendAttachmentState colorState = {};
  colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorState.blendEnable = VK_TRUE;													// Enable blending
  colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorState.colorBlendOp = VK_BLEND_OP_ADD;
  colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorState.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
  colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
  colorBlendingCreateInfo.attachmentCount = 1;
  colorBlendingCreateInfo.pAttachments = &colorState;

  // -- PIPELINE LAYOUT (TODO: Apply Future Descriptor Set Layouts) --
  std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { m_descriptorSetLayout, m_samplerSetLayout };

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &m_pushConstantRange;

  // Create Pipeline Layout
  VkResult result = vkCreatePipelineLayout(m_device.logical, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] successfully created graphics pipeline layout" << std::endl;
  }
  else
  {
    std::cerr << "[+] failed to create graphics pipeline layout" << std::endl;
    throw std::runtime_error("Failed to create Pipeline Layout!");
  }

  // -- DEPTH STENCIL TESTING --
  VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
  depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilCreateInfo.depthTestEnable = VK_TRUE;
  depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
  depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilCreateInfo.stencilTestEnable = VK_FALSE;


  // -- GRAPHICS PIPELINE CREATION --
  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stageCount = 2;									              // Number of shader stages
  pipelineCreateInfo.pStages = shaderStages;							          // List of shader stages
  pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// All the fixed function pipeline states
  pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
  pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
  pipelineCreateInfo.pDynamicState = nullptr;
  pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
  pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
  pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
  pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
  pipelineCreateInfo.layout = m_pipelineLayout;							        // Pipeline Layout pipeline should use
  pipelineCreateInfo.renderPass = m_renderPass;							        // Render pass description the pipeline is compatible with
  pipelineCreateInfo.subpass = 0;										                // Subpass of render pass to use with pipeline
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	          // Existing pipeline to derive from...
  pipelineCreateInfo.basePipelineIndex = -1;				                // or index of pipeline being created to derive from (in case creating multiple at once)

  // Create Graphics Pipeline
  result = vkCreateGraphicsPipelines(m_device.logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] successfully created graphics pipeline" << std::endl;
  }
  else
  {
    std::cerr << "[+] failed to create graphics pipeline" << std::endl;
    throw std::runtime_error("Failed to create a Graphics Pipeline!");
  }

  // module are no longer needed, so we can delete them here.
  vkDestroyShaderModule(m_device.logical, fragmentShaderModule, nullptr);
  vkDestroyShaderModule(m_device.logical, vertexShaderModule, nullptr);

  // CREATE SECOND PASS PIPELINE
  // Second pass shaders
  auto secondVertexShaderCode = readFile("Shaders/second_vert.spv");
  auto secondFragmentShaderCode = readFile("Shaders/second_frag.spv");

  // Build shaders
  VkShaderModule secondVertexShaderModule = createShaderModule(secondVertexShaderCode);
  VkShaderModule secondFragmentShaderModule = createShaderModule(secondFragmentShaderCode);

  // Set new shaders
  vertexShaderCreateInfo.module = secondVertexShaderModule;
  fragmentShaderCreateInfo.module = secondFragmentShaderModule;

  VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

  // No vertex data for second pass
  vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
  vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
  vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
  vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

  // Don't want to write to depth buffer
  depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

  // Create new pipeline layout
  VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
  secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  secondPipelineLayoutCreateInfo.setLayoutCount = 1;
  secondPipelineLayoutCreateInfo.pSetLayouts = &m_inputSetLayout;
  secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
  secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

  result = vkCreatePipelineLayout(m_device.logical, &secondPipelineLayoutCreateInfo, nullptr, &m_secondPipelineLayout);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Pipeline Layout!");
  }

  pipelineCreateInfo.pStages = secondShaderStages;	// Update second shader stage list
  pipelineCreateInfo.layout = m_secondPipelineLayout;	// Change pipeline layout for input attachment descriptor sets
  pipelineCreateInfo.subpass = 1;						// Use second subpass

  // Create second pipeline
  result = vkCreateGraphicsPipelines(m_device.logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_secondPipeline);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Graphics Pipeline!");
  }

  // Destroy second shader modules
  vkDestroyShaderModule(m_device.logical, secondFragmentShaderModule, nullptr);
  vkDestroyShaderModule(m_device.logical, secondVertexShaderModule, nullptr);
}



/************************************************************************************************************************
 * function : 
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : May 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createColourBufferImage()
{
  // Resize supported format for colour attachment
  m_colourBufferImage.resize(m_swapChainImages.size());
  m_colourBufferImageMemory.resize(m_swapChainImages.size());
  m_colourBufferImageView.resize(m_swapChainImages.size());

  // Get supported format for colour attachment
  VkFormat colourFormat = chooseSupportedFormat(
    { VK_FORMAT_R8G8B8A8_UNORM },
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  );

  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    // Create Colour Buffer Image
    m_colourBufferImage[i] = createImage(m_swapChainExtent.width, m_swapChainExtent.height, colourFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_colourBufferImageMemory[i]);

    // Create Colour Buffer Image View
    m_colourBufferImageView[i] = createImageView(m_colourBufferImage[i], colourFormat, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}


/************************************************************************************************************************
 * function  : createDepthBufferImage
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createDepthBufferImage()
{
  m_depthBufferImage.resize(m_swapChainImages.size());
  m_depthBufferImageMemory.resize(m_swapChainImages.size());
  m_depthBufferImageView.resize(m_swapChainImages.size());

  const std::vector<VkFormat> requestedFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
  VkFormat depthFormat = chooseSupportedFormat(requestedFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    m_depthBufferImage[i] = createImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthBufferImageMemory[i]);

    m_depthBufferImageView[i] = createImageView(m_depthBufferImage[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
  }
}


/************************************************************************************************************************
 * function  : createFramebuffers 
 *
 * abstract  : Create a number of frame buffers that correspond to the number of images that we have in the swapchain.
 *             The only correlation is that the i-th frame buffer corresponds to the i-th image.  We are responsible for 
 *             ensuring that this correlation is not broken
 *
 * parameters: none
 *
 * returns   : void, throws run-time exception on error
 *
 * written   : Mar 2024 (GKHuber)
 *             Apr 2024 (GKHuber) Added support for depth buffer image.  Due to the depth buffer being used only during 
 *                                the drawing part of the pipeline, there is not need to have a separate depth buffer
 *                                per swapchain, the drawing section is 'protected' by a mutex so no change of over 
 *                                writting the image.
************************************************************************************************************************/
void vkContext::createFramebuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImages.size());              // one frame buffer per image

  for (size_t i = 0; i < m_swapChainFrameBuffers.size(); i++)
  {
    std::array<VkImageView, 3> attachments = { m_swapChainImages[i].imageView, m_colourBufferImageView[i], m_depthBufferImageView[i] };

    VkFramebufferCreateInfo fbCreateInfo = {};
    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.renderPass = m_renderPass;
    fbCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbCreateInfo.pAttachments = attachments.data();
    fbCreateInfo.width = m_swapChainExtent.width;
    fbCreateInfo.height = m_swapChainExtent.height;
    fbCreateInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(m_device.logical, &fbCreateInfo, nullptr, &m_swapChainFrameBuffers[i]);
    if (result == VK_SUCCESS)
    {
      std::cerr << "[+] created " << i + 1 << " framebuffer" << std::endl;
    }
    else
    {
      std::cerr << "[-] failed to create " << i + 1 << "framebuffer" << std::endl;
      throw std::runtime_error("failed to create framebuffer");
    }
  }
}



/************************************************************************************************************************
 * function  : createCommandPool
 *
 * abstract  : This creates a specialized memory pool that is used to allocate command buffers from.
 *
 * parameters: none
 *
 * returns   : void, throws runtime exception on error.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createCommandPool()
{
  queueFamilyIndices qfi = getQueueFamilies(m_device.physical, nullptr);

  VkCommandPoolCreateInfo  poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = qfi.graphicsFamily;

  VkResult result = vkCreateCommandPool(m_device.logical, &poolInfo, nullptr, &m_graphicsCommandPool);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] created command pool" << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to create command pool" << std::endl;
    throw std::runtime_error("failed to create commadn pool");
  }
}



/************************************************************************************************************************
 * function  : createCommandBuffers
 *
 * abstract  : This function creates a new command buffer associated with each frame-buffer (and thus each swapchain 
 *             image).  The correlation is based on the index into each of these arrays, and it is up to use to insure 
 *             that this correlation is maintained
 * 
 *
 * parameters: none
 *
 * returns   : void, throws run time exception
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createCommandBuffers()
{
  m_commandbuffers.resize(m_swapChainFrameBuffers.size());       // one command buffer per frame buffer

  VkCommandBufferAllocateInfo cbAllocInfo = {};
  cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbAllocInfo.commandPool = m_graphicsCommandPool;
  cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandbuffers.size());

  VkResult result = vkAllocateCommandBuffers(m_device.logical, &cbAllocInfo, m_commandbuffers.data());
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] created command buffer " << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to create command buffer" << std::endl;
    throw std::runtime_error("failed to create command buffer");
  }
}



/************************************************************************************************************************
 * function  : createSynchronisations 
 *
 * abstract  : Creates the various synchronisation objects used during the rendering.  We maintain three arrays for the 
 *             synchronisation objects to avoid run away generation of these objects (and thus a slow memory leak).  We
 *             use two semaphore (obects that we create, but are only visible to the GPU) to control access during the 
 *             graphics processing and one fense (objects that we create, but are visible by the CPU) to control access
 *             to the drawing function
 *
 * parameters: none
 *
 * returns   : void, throw run-time exception on error
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createSynchronisations()
{
  m_imageAvailable.resize(MAX_FRAME_DRAWS);
  m_renderFinished.resize(MAX_FRAME_DRAWS);
  m_drawFences.resize(MAX_FRAME_DRAWS);

  // Semaphore creation information
  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  // Fence creation information
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
  {
    if (vkCreateSemaphore(m_device.logical, &semaphoreCreateInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS ||
      vkCreateSemaphore(m_device.logical, &semaphoreCreateInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS ||
      vkCreateFence(m_device.logical, &fenceCreateInfo, nullptr, &m_drawFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
    }
  }
}



void vkContext::createTextureSampler()
{
  // Sampler Creation Info
  VkSamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U (x) direction
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V (y) direction
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in W (z) direction
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
  samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
  samplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
  samplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
  samplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
  samplerCreateInfo.anisotropyEnable = VK_TRUE;						// Enable Anisotropy
  samplerCreateInfo.maxAnisotropy = 16;								// Anisotropy sample level

  VkResult result = vkCreateSampler(m_device.logical, &samplerCreateInfo, nullptr, &m_textureSampler);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Filed to create a Texture Sampler!");
  }
}



/************************************************************************************************************************
 * function  : createUniformBuffers 
 *
 * abstract  :
 *
 * parameters: void 
 *
 * returns   : void 
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createUniformBuffers()
{
  // Buffer size will be size of all three variables (will offset to access)
  VkDeviceSize vpBufferSize = sizeof(UboVP);

  //VkDeviceSize modelBufferSize = m_modelUniformAlignment * MAX_OBJECTS;


  // One uniform buffer for each image (and by extension, command buffer)
  m_vpUniformBuffer.resize(m_swapChainImages.size());
  m_vpUniformBufferMemory.resize(m_swapChainImages.size());
  //m_modelDUniformBuffer.resize(m_swapChainImages.size());
  //m_modelDUniformBufferMemory.resize(m_swapChainImages.size());

  // Create Uniform buffers
  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    createBuffer(m_device.physical, m_device.logical, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_vpUniformBuffer[i], &m_vpUniformBufferMemory[i]);
  }
}



/************************************************************************************************************************
 * function  : createDescriptorPool
 *
 * abstract  :
 *
 * parameters: void 
 *
 * returns   : void, throws runtime exception on error
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createDescriptorPool()
{
  // create uniform descriptor pool
  // Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
  // view-projection pool
  VkDescriptorPoolSize vpPoolSize = {};
  vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  vpPoolSize.descriptorCount = static_cast<uint32_t>(m_vpUniformBuffer.size());

  std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

  // Data to create Descriptor Pool
  VkDescriptorPoolCreateInfo poolCreateInfo = {};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.maxSets = static_cast<uint32_t>(m_swapChainImages.size());	          // Maximum number of Descriptor Sets that can be created from pool
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());		// Amount of Pool Sizes being passed
  poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();									                            // Pool Sizes to create pool with

  // Create Descriptor Pool
  VkResult result = vkCreateDescriptorPool(m_device.logical, &poolCreateInfo, nullptr, &m_descriptorPool);
  if (result != VK_SUCCESS)
  {
    std::cerr << "[-] failed to create a descriptor pool" << std::endl;
    throw std::runtime_error("Failed to create a Descriptor Pool!");
  }

  // sampler pool
  VkDescriptorPoolSize samplerPoolSize = {};
  samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerPoolSize.descriptorCount = MAX_OBJECTS;

  VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
  samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
  samplerPoolCreateInfo.poolSizeCount = 1;
  samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

  result = vkCreateDescriptorPool(m_device.logical, &samplerPoolCreateInfo, nullptr, &m_samplerDescriptorPool);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Descriptor Pool!");
  }

  // attachment pool

  // CREATE INPUT ATTACHMENT DESCRIPTOR POOL
  // Colour Attachment Pool Size
  VkDescriptorPoolSize colourInputPoolSize = {};
  colourInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  colourInputPoolSize.descriptorCount = static_cast<uint32_t>(m_colourBufferImageView.size());

  // Depth Attachment Pool Size
  VkDescriptorPoolSize depthInputPoolSize = {};
  depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  depthInputPoolSize.descriptorCount = static_cast<uint32_t>(m_depthBufferImageView.size());

  std::vector<VkDescriptorPoolSize> inputPoolSizes = { colourInputPoolSize, depthInputPoolSize };

  // Create input attachment pool
  VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
  inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  inputPoolCreateInfo.maxSets = m_swapChainImages.size();
  inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
  inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

  result = vkCreateDescriptorPool(m_device.logical, &inputPoolCreateInfo, nullptr, &m_inputDescriptorPool);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Descriptor Pool!");
  }

}



/************************************************************************************************************************
 * function  : createDescriptorSets
 *
 * abstract  :
 *
 * parameters: void 
 *
 * returns   : void, throws runtime exception on error
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createDescriptorSets()
{
  // Resize Descriptor Set list so one for every buffer
  m_descriptorSets.resize(m_swapChainImages.size());

  std::vector<VkDescriptorSetLayout> setLayouts(m_swapChainImages.size(), m_descriptorSetLayout);

  // Descriptor Set Allocation Info
  VkDescriptorSetAllocateInfo setAllocInfo = {};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = m_descriptorPool;					  				                  // Pool to allocate Descriptor Set from
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());	// Number of sets to allocate
  setAllocInfo.pSetLayouts = setLayouts.data();							  		                    // Layouts to use to allocate sets (1:1 relationship)

  // Allocate descriptor sets (multiple)
  VkResult result = vkAllocateDescriptorSets(m_device.logical, &setAllocInfo, m_descriptorSets.data());
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Descriptor Sets!");
  }

  // Update all of descriptor set buffer bindings
  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    // Buffer info and data offset info
    VkDescriptorBufferInfo vpBufferInfo = {};
    vpBufferInfo.buffer = m_vpUniformBuffer[i];		// Buffer to get data from
    vpBufferInfo.offset = 0;					   	        // Position of start of data
    vpBufferInfo.range = sizeof(UboVP);				    // Size of data

    // Data about connection between binding and buffer
    VkWriteDescriptorSet vpSetWrite = {};
    vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vpSetWrite.dstSet = m_descriptorSets[i];								// Descriptor Set to update
    vpSetWrite.dstBinding = 0;											// Binding to update (matches with binding on layout/shader)
    vpSetWrite.dstArrayElement = 0;									// Index in array to update
    vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor
    vpSetWrite.descriptorCount = 1;									// Amount to update
    vpSetWrite.pBufferInfo = &vpBufferInfo;							// Information about buffer data to bind

    std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

    // Update the descriptor sets with new buffer/binding info
    vkUpdateDescriptorSets(m_device.logical, static_cast<uint32_t>(setWrites.size()),  setWrites.data(), 0, nullptr);
  }
}



/************************************************************************************************************************
 * function : 
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : May 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::createInputDescriptorSets()
{
  // Resize array to hold descriptor set for each swap chain image
  m_inputDescriptorSets.resize(m_swapChainImages.size());

  // Fill array of layouts ready for set creation
  std::vector<VkDescriptorSetLayout> setLayouts(m_swapChainImages.size(), m_inputSetLayout);

  // Input Attachment Descriptor Set Allocation Info
  VkDescriptorSetAllocateInfo setAllocInfo = {};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = m_inputDescriptorPool;
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());
  setAllocInfo.pSetLayouts = setLayouts.data();

  // Allocate Descriptor Sets
  VkResult result = vkAllocateDescriptorSets(m_device.logical, &setAllocInfo, m_inputDescriptorSets.data());
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Input Attachment Descriptor Sets!");
  }

  // Update each descriptor set with input attachment
  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    // Colour Attachment Descriptor
    VkDescriptorImageInfo colourAttachmentDescriptor = {};
    colourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    colourAttachmentDescriptor.imageView = m_colourBufferImageView[i];
    colourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

    // Colour Attachment Descriptor Write
    VkWriteDescriptorSet colourWrite = {};
    colourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    colourWrite.dstSet = m_inputDescriptorSets[i];
    colourWrite.dstBinding = 0;
    colourWrite.dstArrayElement = 0;
    colourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colourWrite.descriptorCount = 1;
    colourWrite.pImageInfo = &colourAttachmentDescriptor;

    // Depth Attachment Descriptor
    VkDescriptorImageInfo depthAttachmentDescriptor = {};
    depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    depthAttachmentDescriptor.imageView = m_depthBufferImageView[i];
    depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

    // Depth Attachment Descriptor Write
    VkWriteDescriptorSet depthWrite = {};
    depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    depthWrite.dstSet = m_inputDescriptorSets[i];
    depthWrite.dstBinding = 1;
    depthWrite.dstArrayElement = 0;
    depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthWrite.descriptorCount = 1;
    depthWrite.pImageInfo = &depthAttachmentDescriptor;

    // List of input descriptor set writes
    std::vector<VkWriteDescriptorSet> setWrites = { colourWrite, depthWrite };

    // Update descriptor sets
    vkUpdateDescriptorSets(m_device.logical, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
  }
}


/************************************************************************************************************************
 * function  : updateUniformBuffer
 *
 * abstract  :
 *
 * parameters: imageIndex -- [in] the current image we are working with.  Recall, framebuffer, image, command buffer and 
 *                           uniform buffers are all in a one-to-one correspondnce
 *
 * returns   : void
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
void vkContext::updateUniformBuffers(uint32_t imageIndex)
{
  void* data;
  vkMapMemory(m_device.logical, m_vpUniformBufferMemory[imageIndex], 0, sizeof(UboVP), 0, &data);
  memcpy(data, &m_uboVP, sizeof(UboVP));
  vkUnmapMemory(m_device.logical, m_vpUniformBufferMemory[imageIndex]);
}

/************************************************************************************************************************
 * function  : recordCommands
 *
 * abstract  : This function records the commands that will be executed by a queue when the buffer is submitted to the
 *             queue.  Seeing as how this is just recording the commands, it is very efficient and can be called multiple
 *             time without a performace penality (unlike submitting to a queue where the commands are actually executed)
 *
 * parameters: none
 *
 * returns   : void, throws runtime exception
 *
 * written   : Mar 2024 (GKHuber)
 *           : modified Apr2024 to support index buffers and resource buffering.
 *           : modified Apr2024 to support descriptor sets, depth testing 
************************************************************************************************************************/
void vkContext::recordcommands(uint32_t currentImage)
{
  // Information about how to begin each command buffer
  VkCommandBufferBeginInfo bufferBeginInfo = {};
  bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // Information about how to begin a render pass (only needed for graphical applications)
  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = m_renderPass;							// Render Pass to begin
  renderPassBeginInfo.renderArea.offset = { 0, 0 };						// Start point of render pass in pixels
  renderPassBeginInfo.renderArea.extent = m_swapChainExtent;				// Size of region to run render pass on (starting at offset)

  std::array<VkClearValue, 3> clearValues = {};
  clearValues[0].color = { 0.0f, 0.0f, 0.0, 1.0f };
  clearValues[1].color = { 0.6f, 0.65f, 0.4f, 1.0f };
  clearValues[2].depthStencil.depth = 1.0f;
 
  renderPassBeginInfo.pClearValues = clearValues.data();							// List of clear values (TODO: Depth Attachment Clear Value)
  renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

  renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[currentImage];

  // Start recording commands to command buffer!
  VkResult result = vkBeginCommandBuffer(m_commandbuffers[currentImage], &bufferBeginInfo);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to start recording a Command Buffer!");
  }

  // Begin Render Pass
  vkCmdBeginRenderPass(m_commandbuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Bind Pipeline to be used in render pass
  vkCmdBindPipeline(m_commandbuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

  for (size_t j = 0; j < m_modelList.size(); j++)
  {
    MeshModel thisModel = m_modelList[j];

    glm::mat4 model = thisModel.getModel();

    vkCmdPushConstants(m_commandbuffers[currentImage], m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(Model), &model);

    for (size_t k = 0; k < thisModel.getMeshCount(); k++)
    {
      VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer() };					// Buffers to bind
      VkDeviceSize offsets[] = { 0 };												// Offsets into buffers being bound
      vkCmdBindVertexBuffers(m_commandbuffers[currentImage], 0, 1, vertexBuffers, offsets);	// Command to bind vertex buffer before drawing with them

      // Bind mesh index buffer, with 0 offset and using the uint32 type
      vkCmdBindIndexBuffer(m_commandbuffers[currentImage], thisModel.getMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);


      std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_descriptorSets[currentImage], m_samplerDescriptorSets[thisModel.getMesh(k)->getTexId()] };

      // Bind Descriptor Sets
      vkCmdBindDescriptorSets(m_commandbuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
        0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

      // Execute pipeline
      vkCmdDrawIndexed(m_commandbuffers[currentImage], thisModel.getMesh(k)->getIndexCount(), 1, 0, 0, 0);
    }
  }

  // start second pass
    vkCmdNextSubpass(m_commandbuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandbuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_secondPipeline);
    vkCmdBindDescriptorSets(m_commandbuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_secondPipelineLayout,
      0, 1, &m_inputDescriptorSets[currentImage], 0, nullptr);
    vkCmdDraw(m_commandbuffers[currentImage], 3, 1, 0, 0);

  // End Render Pass
  vkCmdEndRenderPass(m_commandbuffers[currentImage]);

  // Stop recording to command buffer
  result = vkEndCommandBuffer(m_commandbuffers[currentImage]);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to stop recording a Command Buffer!");
  }

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vulkan functions - get functions
/************************************************************************************************************************
 * function  : getPhysicalDevice
 *
 * abstract  : This function determines which physical device will be used by this application.  It performs the following
 *             steps;
 *               (a) enumerates all physical devices this instance of Vulkan can see (depends on m_instance)
 *               (b) build a buffer of VkPhysicalDevices (an opaque pointer to the device)
 *               (c) iterates over all devices looking for an acceptable device 
 *
 * parameters: none
 *
 * returns   : void, throws run-time exception on error
 *
 * written   : Mar 2024 (GKHuber)
 * modified  : Apr 2024 (GKHuber) added code to get the minimum offset for use with dynamic uniform buffers
************************************************************************************************************************/
void vkContext::getPhysicalDevice()
{
  uint32_t deviceCnt = 0;
  vkEnumeratePhysicalDevices(m_instance, &deviceCnt, nullptr);

  if (deviceCnt == 0)
  {
    std::cerr << "[-] Could not find any physical devices" << std::endl;
    throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
  }

  std::vector<VkPhysicalDevice> deviceList(deviceCnt);
  vkEnumeratePhysicalDevices(m_instance, &deviceCnt, deviceList.data());

  for (const auto &device : deviceList)
  {
    if (checkDeviceSuitable(device))
    {
      m_device.physical = device;
      break;
    }
  }

  if (m_device.physical == nullptr)
  {
    std::cout << "[-] no suitable physical device found" << std::endl;
    throw std::runtime_error("No suitalbe physical device found");
  }

  VkPhysicalDeviceProperties        deviceProperties;
  vkGetPhysicalDeviceProperties(m_device.physical, &deviceProperties);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// support functions - checker functions
/************************************************************************************************************************
 * function  : checkInstanceExtensionsSupport 
 *
 * abstract  : This function checks to see if all requested extensions are supported by the instance of Vulkan.  The steps
 *             are;
 *                (a) get the number of extensions supported (extensionCnt)
 *                (b) allocate a buffer of VkExtensionProperties large enough to hold all extensions
 *                (c) iterate through the list of requested extensions, comparing their name to the name of each found
 *                    extention.  If names match, go on to next requested extension.  If we exhaust all the supported 
 *                    extentions and did not have a match return false.
 *                (d) if we found all requested extentions return true
 *             The list of requested extensions (m_instanceExtensions) is constructed in the class constructor.
 *
 * parameters: none
 *
 * returns   : bool, true if all requested extensions are found, false otherwise
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
bool vkContext::checkInstanceExtensionSupport()
{
  uint32_t extensionCnt = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCnt, nullptr);

  if (extensionCnt == 0) return false;

  std::vector<VkExtensionProperties> extensions(extensionCnt);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCnt, extensions.data());

  // check to see if requested extensions are in list of install extensions.
  std::cerr << "[?] found " << extensionCnt << " supported extensions" << std::endl;
  for (auto reqExt : m_instanceExtensions)
  {
    bool hasExtention = false;

    for (const auto &ext : extensions)
    {
      if (strcmp(reqExt, ext.extensionName) == 0)
      {
        hasExtention = true;
        break;
      }
    }

    if (!hasExtention)
    {
      std::cerr << "[-] extention: " << reqExt << " not found." << std::endl;
      return false;
    }
  }

  return true;
}



/************************************************************************************************************************
 * function  : checkDeviceExtensionSupport 
 *
 * abstract  : This function checks to see is a device supports a given extension.  The steps that are followed are;
 *                (a) get the number of extensions supported (extensionCount)
 *                (b) allocate a buffer of VkExtensionProperties large enough to hold all extensions
 *                (c) iterate through the list of requested extensions, comparing their name to the name of each found
 *                    extention.  If names match, go on to next requested extension.  If we exhaust all the supported 
 *                    extentions and did not have a match return false.
 *                (d) if we found all requested extentions return true
 *             The list of requested extensions (deviceExtensions) is a global variable defined in utilities.h.  It 
 *             is necessary due to swapchain being an extension.  The macro `VK_KHR_SWAPCHAIN_EXTENSION_NAME' resolves 
 *             to the Khronos name for the extension.
 * 
 *
 * parameters: dev -- [in] an opaque pointer to a VkPhysicalDevice type that represents the particular device being 
 *                    queried.
 *
 * returns   : bool - true if the device supports all of the requested extensions, false otherwise.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
bool vkContext::checkDeviceExtensionSupport(VkPhysicalDevice dev)
{
  // Get device extension count
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

  // If no extensions found, return failure
  if (extensionCount == 0)
  {
    return false;
  }

  // Populate list of extensions
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, extensions.data());

  // Check for extension
  for (const auto& deviceExtension : deviceExtensions)
  {
    bool hasExtension = false;
    for (const auto& extension : extensions)
    {
      if (strcmp(deviceExtension, extension.extensionName) == 0)
      {
        hasExtension = true;
        break;
      }
    }

    if (!hasExtension)
    {
      return false;
    }
  }

  return true;
}



/************************************************************************************************************************
 * function  : checkValidationLayerSupport
 *
 * abstract  : This function checks to see if the requested validation layers are supported by the instance of Vulkan.  
 *             The requested validation layers are stored in `validationLayers' is a file-scoped variable defined in 
 *             vkContext.h.  The steps are;
  *               (a) get the number of layers supported (validationLayerCnt)
 *                (b) allocate a buffer of VkExtensionProperties large enough to hold all layers
 *                (c) iterate through the list of requested layers, comparing their name to the name of each found
 *                    layer.  If names match, go on to next requested layer.  If we exhaust all the supported 
 *                    layer and did not have a match return false.
 *                (d) if we found all requested layers return true.
 *
 * parameters: none
 *
 * returns   : bool - true if all requested validation layers are found, false otherwise
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
bool vkContext::checkValidationLayerSupport()
{
  uint32_t validationLayerCnt;
  vkEnumerateInstanceLayerProperties(&validationLayerCnt, nullptr);

  // Check if no validation layers found AND we want at least 1 layer
  if (validationLayerCnt == 0 && validationLayers.size() > 0)
  {
    return false;
  }

  std::vector<VkLayerProperties> availableLayers(validationLayerCnt);
  vkEnumerateInstanceLayerProperties(&validationLayerCnt, availableLayers.data());

  std::cerr << "[?] found " << validationLayerCnt << " supported layers" << std::endl;
  for (const auto& validationLayer : validationLayers)
  {
    bool hasLayer = false;
    for (const auto& availableLayer : availableLayers)
    {;
      if (strcmp(validationLayer, availableLayer.layerName) == 0)
      {
        hasLayer = true;
        break;
      }
    }

    if (!hasLayer)
    {
      std::cerr << "[-] failed to find validation layer " << validationLayer << std::endl;
      return false;
    }
  }

  return true;
}



/************************************************************************************************************************
 * function  : checkDeviceSuitable
 *
 * abstract  : This checks if a device is suitable.  Suitability is determined if the device supports the required queues,
 *             and the supports the required extentions, and the device supports a swapchain.  We perform;
 *               (a) query the device properties
 *               (b) get the queue indicies that the device supports, the indices are stored in queueFamilyIndices
 *               (c) check if the device supports the required extensions
 *               (d) check if the device supports a swapchain
 *               (e) return true if steps b,c, and d all return true, false otherwise.
 *             if the application requiresspecial features form the device, they can be checked here as well.
 *
 * parameters: dev -- [in] opaque pointer to a VkPhysicalDevice structure for the device we are testing.
 *
 * returns   : bool -- true if the device is suitable, false otherwise.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
bool vkContext::checkDeviceSuitable(VkPhysicalDevice dev)
{
  VkQueueFamilyProperties queueProperties;
  
  VkPhysicalDeviceProperties devProperties;
   vkGetPhysicalDeviceProperties(dev, &devProperties);

  VkPhysicalDeviceFeatures      deviceFeatures;
  vkGetPhysicalDeviceFeatures(dev, &deviceFeatures);

  queueFamilyIndices indices = getQueueFamilies(dev, &queueProperties);

  bool extensionsSupported = checkDeviceExtensionSupport(dev);

  bool swapChainValid = false;
  if (extensionsSupported)
  {
    SwapChainDetails swapChainDetails = getSwapChainDetails(dev);
    swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
  }

  if (indices.isValid() && swapChainValid)
  {
    std::cerr << "[+] found suitable device: " << devProperties.deviceName << std::endl;
    std::cerr << "    queue families (" << queueProperties.queueCount << ")" << std::endl;
    std::cerr << "    capabilites:" << ((queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "graphics," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? "compute," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? "transfer," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "sparse," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? "protected," : "");
    std::cerr << ((queueProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) ? "video decode," : "");
    std::cerr << std::endl;
  }

  return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Support functions - getter functions.
/************************************************************************************************************************
 * function  : getQueueFamilies 
 *
 * abstract  : This function get the queue families that a device supports.  A device may have multiple queues of a given
 *             type, these queues are stored in a buffer of queues of a particular family.  This functions deals with the 
 *             family type and queries it's properties and returns the index into the buffer of queues where the family is.
 *             In the structure queueFamilyIndices, we store the index for the family that we need (graphics and presentation).
 *             Both of these are initally set ot -1 and the function `isValid::queueFamilyIndices' returns true if both of
 *             the indexs are >= 0 (i.e. we found the family located at the given index.  The steps are;
 *               (a) get the number of queue families on the device (queueFamilyCnt)
 *               (b) allocate a buffer able to hold queueFamilyCnt number of VkQueFamilyProperties.
 *               (c) get the properties of each queue family.
 *               (d) for each family see if the graphics bit or the presentation bit is set, store the queue family 
 *                   index in the appropriate member variable.
 *
 * parameters: dev -- [in] obaque pointer to a VkPhysicalDevice that we are testing
 *             pProp -- [out] optional parameter, default value of nullptr
                              pointer to a VkQueueFamilyProperties that is populateded with the results of queueFamily
                              (a properties for a specific, conforming queue family)
 *
 * returns   : queueFamilyIndices structure.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
queueFamilyIndices vkContext::getQueueFamilies(VkPhysicalDevice dev, VkQueueFamilyProperties* pProp)
{
  queueFamilyIndices indices;

  uint32_t queueFamilyCnt = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCnt, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCnt);
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCnt, queueFamilyList.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilyList)
  {
    // check to see if there are queue's with the appropriate types set.
    if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      indices.graphicsFamily = i;
    }

    // check to see if the queue family supports presentations
    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentationSupport);
    if ((queueFamily.queueCount > 0) && presentationSupport)
    {
      indices.presentationFamily = i;
    }

    if (indices.isValid())
    {
      if(nullptr != pProp) *pProp = queueFamily;
      break;
    }

    i++;
  }

  return indices;
}


/************************************************************************************************************************
 * function  :  getSwapChainDetails
 *
 * abstract  : Gets the properties that a swapchain must support.  This functions calls the following functions,
 *               (a) vkGetPhysicalDeviceSurfaceCapabilitiesKHR
 *               (b) vkGetPhysicalDeviceSurfaceFormatsKHR
 *               (c) vkGetPhysicalDeviceSurfacePresentationModesKHR
 *
 * parameters: dev -- [in] opaque pointer to the device being tested.
 *
 * returns   : a SwapChainDetails structure
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
SwapChainDetails vkContext::getSwapChainDetails(VkPhysicalDevice dev)
{
  SwapChainDetails swapChainDetails;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, m_surface, &swapChainDetails.surfaceCapabilities);

  // get list of supported formats....
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    swapChainDetails.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, swapChainDetails.formats.data());
  }

  // get list of presentation modes...
  uint32_t presentationCnt = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentationCnt, nullptr);

  if (presentationCnt != 0)
  {
    swapChainDetails.presentationModes.resize(presentationCnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentationCnt, swapChainDetails.presentationModes.data());
 };

  return swapChainDetails;
}


/************************************************************************************************************************
 * function  : chooseBestSurfaceFormat 
 *
 * abstract  : This attempts to find the best format to use.  Note: if only format is 'VK_FORMAT_UNDEFINED' means all
 *             formats are available so pick one and move on.  Otherwise check each format that is defined looking for 
 *             a format that has;
 *                 VK_FORMAT_R8G8B8A8_UNORM  _or_ VK_FORMAT_B8G8R8A8_UNORM
 *          _and_  VKVK_COLOR_SPACE_SRGB_NONLINEAR_KHR
 *             and return that format, obvisouly the exact values will be application specific.
 *
 * parameters: formats -- [in] reference to a vector of VkSurfaceFormatKHR types.
 *
 * returns   : an instacne of VkSurfaceFormatKHR containing the format of the surface to use.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
VkSurfaceFormatKHR vkContext::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
  // if only format is undefines, means all formats are available, pick one
  if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
  {
    return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  }

  for (const auto& format : formats)
  {
    if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
      && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  // can't find an optimal format, default to first one
  return formats[0];
}



/************************************************************************************************************************
 * function  : chooseBestPresentationMode
 *
 * abstract  : attempts to choose the best presentation mode to use,  if we find 'VK_PRESENT_MODE_MAILBOX_KHR' use it 
 *             otherwise use VK_PRESENT_MODE_FIFO_KHR (which compliant Vulkan implementations must support)
 *
 * parameters: presentationModes -- [in] vector of VkPresentModeKHR types with the presentation modes that the device 
 *                                  supports
 *
 * returns   : VkPresentModeKHR value representing the optimal presentation mode.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
VkPresentModeKHR vkContext::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
{
 // look for mailbox presentation mode -- prevents tearing
  for (const auto& presentationMode : presentationModes)
  {
    if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return presentationMode;
    }
  }
    // if can't find mailbox, choose FIFo mode -- Vulkan spec says it must be presnet
    return VK_PRESENT_MODE_FIFO_KHR;
}



/************************************************************************************************************************
 * function  : chooseSwapExtent
 *
 * abstract  : This function sets the size of each swapchain image.  if the current extent width is greater then the max
 *             size of a uint32_t then the size can vary so just return the current extent.  Otherwise we will need to 
 *             set it manually by doing,
 *                (a) get the frame buffer size (from GLFW) and set the new extent dimensions to match this value
 *                (b) clamp the new values between the min and max as described from the surface capabilities.
 *
 * parameters: surfaceCapabilities -- [in] reference to a const VkSurfaceCapabilitiesKHR structure contain the min and max
 *                                    height and width of an swapchain image
 *
 * returns   : VkExtent2D structure with the width and length values.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
VkExtent2D vkContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
  // if current extent is at numeric limit, then extent can vary, else need to manually set
  if(surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return surfaceCapabilities.currentExtent;
  }
  else
  {
    int width, height;              // windows current dimensions.

    glfwGetFramebufferSize(m_pWindow, &width, &height);

    VkExtent2D newExtent = {};
    newExtent.width = static_cast<uint32_t>(width);
    newExtent.height = static_cast<uint32_t>(height);

    // clamp values between the min and max described by the surface capabilities...
    newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
    newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

    return newExtent;
  }
}


/************************************************************************************************************************
 * function  : chooseSupportedFormat
 *
 * abstract  : choose a format that matches the requested format types and tiling mode.
 *
 * parameters: formats -- [in] reference to a constant std::vector of formats that are acceptable
 *             tiling -- [in] type of tiling that are acceptable for the application
 *             flags -- [in] flags descriping the features that are required.
 *
 * returns   : the format that that is choosen
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
VkFormat vkContext::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags flags)
{
  for (VkFormat format : formats)
  {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(m_device.physical, format, &properties);

    if (tiling == VK_IMAGE_TILING_LINEAR && ((properties.linearTilingFeatures & flags) == flags))
    {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && ((properties.optimalTilingFeatures & flags) == flags))
    {
      return format;
    }

    throw std::runtime_error("Failed to find a matching format");
  }
}



/************************************************************************************************************************
 * function  : createImage
 *
 * abstract  : This function creates a generic image
 *
 * parameters: width -- [in] desired width of the image
 *             height -- [in] desired height of teh image
 *             format -- [in] format that the image will be stored in
 *             tiling -- [in] how the image can be tiled on a surface
 *             useFlags -- [in] the intended use of this image
 *             propFlags -- [in] properties that the image will have (need to confirm that the physical device supports 
 *                               all features.
 *             imageMemory -- [out] where the image will be stored on the device memory
 * 
 *
 * returns   : the image object, thows a runtime exception on error.
 *
 * written   : Apr 2024 (GKHuber)
************************************************************************************************************************/
VkImage vkContext::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling , VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
  // CREATE IMAGE
  // Image Creation Info
  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;						// Type of image (1D, 2D, or 3D)
  imageCreateInfo.extent.width = width;								// Width of image extent
  imageCreateInfo.extent.height = height;								// Height of image extent
  imageCreateInfo.extent.depth = 1;									// Depth of image (just 1, no 3D aspect)
  imageCreateInfo.mipLevels = 1;										// Number of mipmap levels
  imageCreateInfo.arrayLayers = 1;									// Number of levels in image array
  imageCreateInfo.format = format;									// Format type of image
  imageCreateInfo.tiling = tiling;									// How image data should be "tiled" (arranged for optimal reading)
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout of image data on creation
  imageCreateInfo.usage = useFlags;									// Bit flags defining what image will be used for
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Whether image can be shared between queues

  // Create image
  VkImage image;
  VkResult result = vkCreateImage(m_device.logical, &imageCreateInfo, nullptr, &image);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create an Image!");
  }

  // CREATE MEMORY FOR IMAGE

  // Get memory requirements for a type of image
  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(m_device.logical, image, &memoryRequirements);

  // Allocate memory using image requirements and user defined properties
  VkMemoryAllocateInfo memoryAllocInfo = {};
  memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocInfo.allocationSize = memoryRequirements.size;
  memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(m_device.physical, memoryRequirements.memoryTypeBits, propFlags);

  result = vkAllocateMemory(m_device.logical, &memoryAllocInfo, nullptr, imageMemory);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for image!");
  }

  // Connect memory to image
  vkBindImageMemory(m_device.logical, image, *imageMemory, 0);

  return image;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// generic create functions
/************************************************************************************************************************
 * function  : createImageView
 *
 * abstract  : This function creates an view for use with the swapchain image.  A single view is created for each image 
 *             in the swapchain.
 *
 * parameters: image -- [in] swapchain image to create a view for
 *             format -- [in] format to use to create the view.
 *             aspectFlags -- [in] properties of the view.
 * 
 * returns   : VkImageView structure
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
VkImageView vkContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
  VkImageViewCreateInfo viewCreateInfo = {};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.image = image;											              // Image to create view for
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;						    // Type of image (1D, 2D, 3D, Cube, etc)
  viewCreateInfo.format = format;											            // Format of image data
  viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;		// Allows remapping of rgba components to other rgba values
  viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  viewCreateInfo.subresourceRange.aspectMask = aspectFlags;				// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
  viewCreateInfo.subresourceRange.baseMipLevel = 0;						    // Start mipmap level to view from
  viewCreateInfo.subresourceRange.levelCount = 1;							    // Number of mipmap levels to view
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;						  // Start array level to view from
  viewCreateInfo.subresourceRange.layerCount = 1;							    // Number of array levels to view

  // Create image view and return it
  VkImageView imageView;
  VkResult result = vkCreateImageView(m_device.logical, &viewCreateInfo, nullptr, &imageView);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] created image view" << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to created image view" << std::endl;
    throw std::runtime_error("Failed to create an Image View!");
  }

  return imageView;
}


/************************************************************************************************************************
 * function  : createShaderModule
 *
 * abstract  : converts the byte-code for a shader into a module.
 *
 * parameters: code -- [in] buffer containing the byte-code for a module
 *
 * returns   : VkShaderModule reference, throws a runtime exception on error.
 *
 * written   : Mar 2024 (GKHuber)
************************************************************************************************************************/
VkShaderModule vkContext::createShaderModule(const std::vector<char>& code)
{
  VkShaderModuleCreateInfo  shaderModuleCreateInfo = {};
  shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderModuleCreateInfo.codeSize = code.size();
  shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(m_device.logical, &shaderModuleCreateInfo, nullptr, &shaderModule);
  if (result == VK_SUCCESS)
  {
    std::cerr << "[+] created shader module" << std::endl;
  }
  else
  {
    std::cerr << "[-] failed to creat shader module" << std::endl;
  }

  return shaderModule;
}



int vkContext::createTextureImage(std::string fileName)
{
  int width, height;
  VkDeviceSize imageSize;

  stbi_uc* imageData = loadTextureFile(fileName, &width, &height, &imageSize);

  // create staging buffer to hold loaded data
  VkBuffer imageStagingBuffer;
  VkDeviceMemory imageStagingBufferMemory;

  createBuffer(m_device.physical, m_device.logical, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageStagingBuffer, &imageStagingBufferMemory);

  // copy image data to buffer
  void* data;
  vkMapMemory(m_device.logical, imageStagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, imageData, static_cast<uint32_t>(imageSize));
  vkUnmapMemory(m_device.logical, imageStagingBufferMemory);

  // free original image data
  stbi_image_free(imageData);

  // create image to hold final texture
  VkImage texImage;
  VkDeviceMemory texImageMemory;
  texImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

  // copy data to image
  transitionImageLayout(m_device.logical, m_graphicsQueue, m_graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyImageBuffer(m_device.logical, m_graphicsQueue, m_graphicsCommandPool, imageStagingBuffer, texImage, width, height);

  // Transition image to be shader readable for shader usage
  transitionImageLayout(m_device.logical, m_graphicsQueue, m_graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  m_textureImages.push_back(texImage);
  m_textureImageMemory.push_back(texImageMemory);

  vkDestroyBuffer(m_device.logical, imageStagingBuffer, nullptr);
  vkFreeMemory(m_device.logical, imageStagingBufferMemory, nullptr);

  return m_textureImages.size() - 1;
}



int vkContext::createTexture(std::string fileName)
{
  // Create Texture Image and get its location in array
  int textureImageLoc = createTextureImage(fileName);

  // Create Image View and add to list
  VkImageView imageView = createImageView(m_textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
  m_textureImageViews.push_back(imageView);

  // Create Texture Descriptor
  int descriptorLoc = createTextureDescriptor(imageView);

  // Return location of set with texture
  return descriptorLoc;
}



int vkContext::createTextureDescriptor(VkImageView textureImage)
{
  VkDescriptorSet descriptorSet;

  // Descriptor Set Allocation Info
  VkDescriptorSetAllocateInfo setAllocInfo = {};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = m_samplerDescriptorPool;
  setAllocInfo.descriptorSetCount = 1;
  setAllocInfo.pSetLayouts = &m_samplerSetLayout;

  // Allocate Descriptor Sets
  VkResult result = vkAllocateDescriptorSets(m_device.logical, &setAllocInfo, &descriptorSet);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Texture Descriptor Sets!");
  }

  // Texture Image Info
  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// Image layout when in use
  imageInfo.imageView = textureImage;									// Image to bind to set
  imageInfo.sampler = m_textureSampler;									// Sampler to use for set

  // Descriptor Write Info
  VkWriteDescriptorSet descriptorWrite = {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  // Update new descriptor set
  vkUpdateDescriptorSets(m_device.logical, 1, &descriptorWrite, 0, nullptr);

  // Add descriptor set to list
  m_samplerDescriptorSets.push_back(descriptorSet);

  // Return descriptor set location
  return m_samplerDescriptorSets.size() - 1;
}


int vkContext::createMeshModel(std::string modelFile)
{
  // Import model "scene"
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
  if (!scene)
  {
    throw std::runtime_error("Failed to load model! (" + modelFile + ")");
  }

  // Get vector of all materials with 1:1 ID placement
  std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

  // Conversion from the materials list IDs to our Descriptor Array IDs
  std::vector<int> matToTex(textureNames.size());

  // Loop over textureNames and create textures for them
  for (size_t i = 0; i < textureNames.size(); i++)
  {
    // If material had no texture, set '0' to indicate no texture, texture 0 will be reserved for a default texture
    if (textureNames[i].empty())
    {
      matToTex[i] = 0;
    }
    else
    {
      // Otherwise, create texture and set value to index of new texture
      matToTex[i] = createTexture(textureNames[i]);
    }
  }

  // Load in all our meshes
  std::vector<mesh> modelMeshes = MeshModel::loadNode(m_device.physical, m_device.logical, m_graphicsQueue, m_graphicsCommandPool,
    scene->mRootNode, scene, matToTex);

  // Create mesh model and add to list
  MeshModel meshModel = MeshModel(modelMeshes);
  m_modelList.push_back(meshModel);

  return m_modelList.size() - 1;
}



stbi_uc* vkContext::loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
  // number of channels image used
  int channels;

  // load pixel data for images
  std::string fileLoc = "./Textures/" + fileName;
  stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

  if (!image)
  {
    throw std::runtime_error("failed to load a texture file (" + fileName + ")");
  }

  //calculate the size of the image
  *imageSize = *width * *height * 4;

  return image;
}





