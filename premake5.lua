VULKAN_SDK = os.getenv("VULKAN_SDK")

workspace "VulkanHelloTriangle"
    architecture "x64"
    targetdir "build"

    configurations 
    {
        "Debug",
        "Release"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    startproject "HelloTriangle"

    outputdir = "%{cfg.system}-%{cfg.buildcfg}"

project "VulkanHelloTriangle"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.hpp",
        "src/**.cpp",
    }

    externalincludedirs
    {
        "%{VULKAN_SDK}/Include",
        "libs/glfw/include",
        "libs/glm/include"
    }

    libdirs
    {
        "%{VULKAN_SDK}/Lib",
        "libs/glfw/lib"
    }

    links
    {
        "vulkan-1",
        "glfw3"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"