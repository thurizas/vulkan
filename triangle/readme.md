This is the basi Vulkan setup.
The vertices of the object are stored in the shaders, but we do render them to the screen, thus
we have examples of
- creating a vulkan instance
- enumerating and choosing a physical device to work with
- creation of a logical device
- creation of a swapchain and swapchain images
- creation of command queue pools
- creation of command queues
- creation of a graphics pipeline
- creation of vertex and fragment shaders
- copying commands to a command chain
- creation of synchronization objects to manage the pipeline
- submission of commands to the queues.