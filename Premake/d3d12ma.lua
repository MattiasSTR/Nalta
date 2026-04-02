project "D3D12MA"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")

    files
    {
        dirs.thirdparty .. "/D3D12MA/src/D3D12MemAlloc.cpp",
        dirs.thirdparty .. "/D3D12MA/include/D3D12MemAlloc.h",
    }

    includedirs
    {
        dirs.thirdparty .. "/D3D12MA/include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        optimize "off"

    filter "configurations:Development"
        runtime "Release"
        symbols "on"
        optimize "speed"

    filter "configurations:Shipping"
        runtime "Release"
        symbols "off"
        optimize "speed"