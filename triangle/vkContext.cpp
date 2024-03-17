// NOTE: upto udemy code_6_pipeline_fixedfunction done.
#include <iostream>
#include <set>

#include "vkContext.h"
#include "vkValidations.h"



/************************************************************************************************************************
 * function  : ctor
 *
 * abstract  : Constructs an instance of the vulkan context.  We perform the following tasks,
 *               (a) get a list of the extensions required by GLFW, and check that they exist
 *
 * parameters: pWindow -- [in] pointer to the GLFW window we will be rendering to
 *
 * returns   : 
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
 * returns   :
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
 *               (b) enumerate the physical devices and gets their properties
 *               (c) select the physical device that matches the application requirements
 *               (d) create a logical device
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
    createSwapChain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    recordcommands();
  }
  catch (const std::runtime_error& e)
  {
    std::cout << "[ERROR] " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void vkContext::cleanupContext()
{
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
    populateDebugMessengerCreateInfo(debugCreateInfo);

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

void vkContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

void vkContext::createDebugMessenger()
{
  if (!m_useValidation) return;
    
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

  populateDebugMessengerCreateInfo(createInfo);
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

  // build all the components for the pipeline
  // -- VERTEX INPUT
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
  vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
  vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;			          // TODO: List of Vertex Binding Descriptions (data spacing/stride information)
  vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
  vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;		          // TODO: List of Vertex Attribute Descriptions (data format and where to bind to/from)

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
    vkCmdDraw(m_commandbuffers[i], 3, 1, 0, 0);

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


bool vkContext::checkDeviceSuitable(VkPhysicalDevice dev)
{
  VkQueueFamilyProperties queueProperties;
  
  VkPhysicalDeviceProperties devProperties;
  vkGetPhysicalDeviceProperties(dev, &devProperties);

  //VkPhysicalDeviceFeatures devFeatures;
  //vkGetPhysicalDeviceFeatures(dev, &devFeatures);
  

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





