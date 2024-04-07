This is a continuation the basic Vulkan setup, now drawing two rectangle versus a triangle
This is built on the code in vulkan2, but adding the following differences
- shows how to render multiple objects in a scene
- creation of the equivalent of openGL's IBO (index buffer object).

- difficiencies
  (a) we can not programmatically move verticies or programmatically adjust model-view-projection matricies - to fix
      this we need to implement one of 
	      - descriptor sets             (when we have a few object)
		  - dynamic descriptor sets     (when we have a large number of objects)
		  - push constants              (when we have small data (<= 128 k) - will require re-recording commands each frame
