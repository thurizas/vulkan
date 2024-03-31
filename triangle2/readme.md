This is a continuation the basic Vulkan setup, now drawing a rectangle versus a triangle
This is built on the code in vulkan1, but adding the following differences
- the vertex data (position and color) has been moved into the main application
- creation of a mesh class to hold our vertex data
- full implementation of the vertex load stage of the graphics pipeline
- creation of the equivalent of openGL's VAO, and VBO.

- difficiencies
  (a) we repeat verticies if using multiple triangles - to fix we need to use index buffer arrays (fixed in vulkan2a)
  (b) by using host visible buffers, we are storing the vertex data in less than an optimal section of GPU memory - 
      to fix we need to consider staging buffers (fixed in vulkan3)
  (c) we can not programmatically move verticies or programmatically adjust model-view-projection matricies - to fix
      this we need to implement uniforms. (fixed in vulkan3)

Note: vulkan2a also shows how to have multiple objects in the scene.