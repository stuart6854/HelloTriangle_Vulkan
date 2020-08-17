//
// Created by stuart on 17/08/2020.
//

#ifndef _HELLOTRIANGLE_HELLTRIANGLEAPP_H
#define _HELLOTRIANGLE_HELLTRIANGLEAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class HelloTriangleApp
{
public:
    void run();
    
private:
    GLFWwindow* m_window;
    
    void init_window();
    void init_vulkan();
    void main_loop();
    void cleanup();
    
};

#endif //_HELLOTRIANGLE_HELLTRIANGLEAPP_H
