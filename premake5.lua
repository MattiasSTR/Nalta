newoption {
    trigger     = "clang",
    description = "Use Clang toolset instead of MSVC"
}

workspace "Nalta"
    architecture "x64"
    startproject "Sandbox"
    location "."

    configurations { "Debug", "Development", "Shipping" }
    cppdialect "C++20"
    staticruntime "off"

    if _OPTIONS["clang"] then
        toolset "clang"
        print("Using Clang toolset")
    else
        toolset "msc"
        print("Using MSVC toolset")
    end

dofile("premake/common.lua")

-- Include projects
group "Engine"
    include "Engine"

group "Applications"
    include "Sandbox"