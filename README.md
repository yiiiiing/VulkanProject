# VulkanProject
This project is for learning Vulkan, the current goal is from rendering a simple triangle, making movement by passing MVP matrix in uniform buffuer, rendering an object, add texture to the object, adding keyboard input and UI and finnally making a simple interactive game.

### The progress
- [x] Basic steps to render a triangle
1. Create Vulkan instance and initialize a GLFW window to present the triangle.
2. Create window surface, used for selecting a physical device which supports the triangle presenting.
3. Select a physical device based on our required features, like sampler anisotropy. But some requirements must be satisfied, like device extensions must be supported by Vulkan and the queue family supported by the device must support presentation and VK_QUEUE_GRAPHICS_BIT.
4. Create a logical device based on the physical device to describe the device features and the queue family we want to use, then retrieve the 2 queues for drawing operations and presentation operations.
5. Create the swap chain, essentially a queue of images that waiting to be presented to the screen.
6. Create graphics pipeline to specify some details for collecting vertices data from CPU to GPU, handling these vertices data based on the vertex shader, rasterization, coloring each image pixel based on the outputs from vertex shader, such as vertex color, texture coordinates and these values have been interpolated in the rasterization stage, finally through the color blending stage, the program finishes the drawing step.
7. Create the vertex buffer and index buffer, used for transferring the vertices data
8. Create command buffer, binding all the operations with the command buffer.
9. In the Loop function, the program draws the frame, including getting the available image from the swap chain, updating the uniform buffer, submitting the command to graphics queue and at last, presenting the finished image. The execution order is ensured by the semaphores.
10. When the user closes the window, destroying all the objects we created.

- [x] Create Base Application class and BaseObject class 
BaseApplication class is used for initializing Vulkan instance, device, window, render pass and drawing the frame, which are all the same to the different scene objects(models). While BaseObject class is Used for initializing vertex buffer, uniform buffer, descriptor sets, which are different for the objects.
- [x] Create BaseTexture class 
 Because differect objects can have same texture, so we don't need to add textures for each objects, so we can create new texture in BaseApplication class, and make the pointer `BaseTexture* texturePointer`  in BaseObject class refers to this texture.
 
- [ ] Implement Phong lighting 
- [ ] Add keyboard input
- [ ] Add some UI, like texts, image, bar on the window