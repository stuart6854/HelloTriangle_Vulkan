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
        
    }

    links
    {
        
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"