#ifndef _utilities_h_
#define _utilities_h_

#include <fstream>

const int MAX_FRAME_DRAWS = 3;

// SwapChain is an extension, need to see if it is supported.
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct queueFamilyIndices {
  int graphicsFamily = -1;
  int presentationFamily = -1;

  bool isValid()
  {
    return( (graphicsFamily >= 0) && (presentationFamily >= 0));
  }
};

struct SwapChainDetails
{
  VkSurfaceCapabilitiesKHR         surfaceCapabilities;
  std::vector<VkSurfaceFormatKHR>  formats;
  std::vector<VkPresentModeKHR>    presentationModes;
};

struct swapChainImage
{
  VkImage     image;
  VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename)
{
  std::ifstream ins(filename, std::ios::binary | std::ios::ate);

  if (!ins.is_open())
  {
    std::cerr << "[-] failed to open " << filename << " error: " << strerror(errno) << std::endl;
    throw std::runtime_error("Failed to open file");
  }

  size_t fileSize = (size_t)ins.tellg();
  std::vector<char> buf(fileSize);

  ins.seekg(0);             // move cursor to beginning of file

  ins.read(buf.data(), fileSize);

  ins.close();

  return buf;
}


#endif