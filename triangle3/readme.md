This is a continuation the basic Vulkan setup, now drawing two rectangles
This is built on the code in vulkan2a, but adding the following differences
- able to apply a model matrix to the scene
- how to modify the shaders to use a uniform value
- vulkan uniform : applied to all uniform to all objects in the scene
- introduce descriptor sets to make uniform objects

- difficiencies
  (a) all the model matrix is applie to all objects in the scene -> result is that the scene rotates around the z axis.
  (b) we can not move an object singally (will be resoved in 3a - dynamic descriptor sets)
  (c) dynamic descriptor sets are limited in number, so to move a large number of vertices they will fail (resolved 
      in 3b - push objects).
