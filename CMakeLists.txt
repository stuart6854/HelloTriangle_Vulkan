cmake_minimum_required(VERSION 3.18)
project(_HelloTriangle)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(Libs/glfw)
add_subdirectory(Libs/glm)

add_executable(_HelloTriangle main.cpp)

target_link_libraries(${PROJECT_NAME}
                      PRIVATE
                      glfw
                      glm
                      )