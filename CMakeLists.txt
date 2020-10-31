cmake_minimum_required(VERSION 3.16)
project(HelloTriangle CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED FATAL_ERROR)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(Libs/glfw)

add_subdirectory(Libs/glm)

add_executable(${PROJECT_NAME} src/HelloTriangleApp.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC src ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
                      PRIVATE
                      glfw
                      Vulkan::Vulkan
                      glm
                      )