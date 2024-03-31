// NOTE: upto udemy code_8_draw done.
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
 *               (f) create mesh(s) that are used in the scene
 *               (g) create swapchain and the images that the swapchain will use
 *               (h) create the render pass 
 *               (i) create graphics pipeline
 *               (j) create the framebuffer
 *               (k) create the command pool
 *               (l) create command buffers
 *               (m) record commands to render the scene
 *               (n) create synchronization objects to use along the graphics pipeline
 *            If any of these steps fails, it will throw a runtime exception and the program will terminate.
 *
 * parameters: void
 *
 * returns   : void.  Throws a runtime exception on error
 *
 * written   : Mar 2024 (GKHuber)
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

    std::vector<vertex> meshVertices = { {{0.4f, -0.4f, 0.0f}, {1.0f,0.0f,0.0f}},{{0.4f, 0.4f, 0.0f}, {0.0f,1.0f,0.0f}},{{-0.4f, 0.4f, 0.0f}, {0.0f,0.0f,1.0f}},
                                         {{-0.4f, 0.4f, 0.0f}, {0.0f,0.0f,1.0f}},{{-0.4f, -0.4f, 0.0f}, {1.0f,1.0f,0.0f}},{{0.4f,-0.4f, 0.0f}, {1.0f,0.0f,0.0f}} };
    firstMesh = mesh(m_device.physical, m_device.logical, &meshVertices);

    createSwapChain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    recordcommands();
    createSynchronisations();
  }
  catch (const std::runtime_error& e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
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
************************************************************************************************************************/
void vkContext::draw()
{
  vkWaitForFences(m_device.logical, 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
  vkResetFences(m_device.logical, 1, &m_drawFences[m_currentFrame]);
  
  uint32_t imageIndex;
  vkAcquireNextImageKHR(m_device.logical, m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

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
************************************************************************************************************************/
void vkContext::cleanupContext()
{
  vkDeviceWaitIdle(m_device.logical);

  firstMesh.destroyVertexBuffer();

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
  VkAttachmentDescription  colorAttachment = {};
  colorAttachment.format = m_swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // attachment reference (by index) to the attachment list used in renderPassCreateInfo
  VkAttachmentReference colorAttachmentReference = {};
  colorAttachmentReference.attachment = 0;
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  //Infomation about a particular subpass the render is using
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentReference;

  // determine when layout xlations occure using subpass dependencies
  std::array<VkSubpassDependency, 2> subpassDependencies;
  // defines VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL must happen after
  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  // ...but before
  subpassDependencies[0].dstSubpass = 0;
  subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = 0;

  // defines VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR must happen after
  subpassDependencies[1].srcSubpass = 0;
  subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  // ...but before    
  subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[1].dependencyFlags = 0;

  // finally...create the render pass
  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = 1;
  renderPassCreateInfo.pAttachments = &colorAttachment;
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
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

  std::array<VkVertexInputAttributeDescription, 2> attributeDescription;
  attributeDescription[0].binding = 0;
  attributeDescription[0].location = 0;
  attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[0].offset = offsetof(vertex, pos);

  attributeDescription[1].binding = 0;
  attributeDescription[1].location = 1;
  attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[1].offset = offsetof(vertex, col);


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
  rasterizerCreateInfo.depthClampEnable = VK_FALSE;			    // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
  rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
  rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices
  rasterizerCreateInfo.lineWidth = 1.0f;						        // How thick lines should be when drawn
  rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
  rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// Winding to determine which side is front
  rasterizerCreateInfo.depthBiasEnable = VK_FALSE;			    // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

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
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 0;
  pipelineLayoutCreateInfo.pSetLayouts = nullptr;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
  pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

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

  // TODO:-- DEPTH STENCIL TESTING --

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
  pipelineCreateInfo.pDepthStencilState = nullptr;
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
************************************************************************************************************************/
void vkContext::createFramebuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImages.size());              // one frame buffer per image

  for (size_t i = 0; i < m_swapChainFrameBuffers.size(); i++)
  {
    std::array<VkImageView, 1> attachments = { m_swapChainImages[i].imageView };

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
************************************************************************************************************************/
void vkContext::recordcommands()
{
  // infomation about how to begin each command buffer
  VkCommandBufferBeginInfo bufferBeginInfo = {};
  bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // infomation about how to begin render pass -- only for graphical apps
  VkRenderPassBeginInfo renderBeginInfo = {};
  renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderBeginInfo.renderPass = m_renderPass;
  renderBeginInfo.renderArea.offset = { 0, 0 };
  renderBeginInfo.renderArea.extent = m_swapChainExtent;

  VkClearValue clearValues[] = { 0.6f, 0.65f, 0.4f, 1.0f };
  renderBeginInfo.pClearValues = clearValues;
  renderBeginInfo.clearValueCount = 1;

  for (size_t i = 0; i < m_commandbuffers.size(); i++)
  {
    renderBeginInfo.framebuffer = m_swapChainFrameBuffers[i];

    VkResult  result = vkBeginCommandBuffer(m_commandbuffers[i], &bufferBeginInfo);
    if (result != VK_SUCCESS)
    {
      throw std::runtime_error("failed to start recording a command buffer");
    }

    // begin render pass....
    vkCmdBeginRenderPass(m_commandbuffers[i], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkBuffer vertexBuffers[] = { firstMesh.getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_commandbuffers[i], 0, 1, vertexBuffers, offsets);

    vkCmdDraw(m_commandbuffers[i], static_cast<uint32_t>(firstMesh.getVertexCount()), 1, 0, 0);

    //...end render pass.
    vkCmdEndRenderPass(m_commandbuffers[i]);

    result = vkEndCommandBuffer(m_commandbuffers[i]);
    if (result != VK_SUCCESS)
    {
      throw std::runtime_error("failed to stop recording a command buffer");
    }
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

  return indices.isValid() && extensionsSupported && swapChainValid;
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





