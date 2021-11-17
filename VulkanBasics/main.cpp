#include <iostream>
#include <cstdlib>
#define NO_VALIDATION_DEBUG
#include "BasicApplication.h"

/*Description of steps for rendering a triangle:*/
/*
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
*/

//File structure:
/*
1. BasicApplication class:  used for initializing Vulkan instance, device, window, render pass and drawing the frame, which are all the same to the different scene objects(models).
2. BaseObject class: used for initializing vertex buffer, uniform buffer, descriptor sets, which are different for the objects.
3. BaseTexture class: Considering different objects/models possibly use the same texture picture, so we can just create the texture once and let different objects refer to the same texture.
*/


// ObjectType: {FixedTriangle, FixedRectangle, OBJ_Model};

// here to define the task
#define Task123

int main() {
    BasicApplication basicApp;
    basicApp.InitialApplication(800, 600, "Basic App");
    // add object to application
#ifdef Task123
    basicApp.AddObjectToApplication("Rectangle", ObjectType::FixedRectangle, nullptr, "textures/texture.jpg");
    basicApp.AddObjectToApplication("Triangle", ObjectType::FixedTriangle, nullptr, "textures/texture.jpg");
#endif
#ifdef Task4
    basicApp.AddObjectToApplication("room", ObjectType::OBJ_Model, "Mesh/viking_room.obj", "textures/viking_room.png");
#endif
    try{
        basicApp.RunApplication();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}