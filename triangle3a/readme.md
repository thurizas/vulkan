This is a continuation the basic Vulkan setup, now drawing two rectangles
This is built on the code in vulkan3, but adding the following differences
  - now using dynamic uniforms to hold the MVP matrix
  - this allows us to use different model matrix for each object

- difficiencies
  (a) we need to move a buffer from the CPU to GPU on every frame, which is 
      a relatively costly operation
  (b) this will be fixed in vulkan3b - push constants.
