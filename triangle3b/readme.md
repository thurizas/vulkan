This is a continuation the basic Vulkan setup, now drawing two rectangles
This is built on the code in vulkan3a, but adding the following differences 
   - instead of using dynamic buffers, which incure a cost due to moving buffers 
     from the CPU to the GPU each frame, we now use push-constants.
   - there are some trade-offs in useing push-constants instead of dynamic buffers  
     a. the about of data that can be move in a push-constant is ~128 kbytes
	 b. there is only one push-constant per command queue
	 c. we need to re-record the command buffer for each frame rendered.  However, 
	    the overhead in re-recording the command buffer is less than the overhead in
		moving memory -- thus we save time overall.

- difficiencies
  (a) we still do not have a concept of depth - which will be fixed in vulkan4
