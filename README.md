This is a basic 'Hello World' program in in Vulkan using GLFW as the basis for window support.

The project renders a simple 2-D triangle to the screen.  It demonstrates the following,
1. creating a Vulkan context
2. enumerating the physical devices on the system
3. setting up the necesary extensions and optional debuggind layers
4. picking the optimal device to used, base on the queues supported by the device, the surfaces supported and the swap capabilites
5. creating the logical device
6. creating the surface
7. creating the swap chain
8. writing the shaders needed, and creating the modules that represent the shaders.
9. creating the render pass
10. creating and configuring the graphics pipeline
11. creating the frame buffers, and the images that they will use
12. creating the command pool
13. creating the command buffers, and recording commands to the command buffer.
14. synchronication of drawing operations

This project is based on the Udemy Vulkan course and the Vulkan tutorial.
