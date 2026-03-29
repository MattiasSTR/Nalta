project "DirectXTex"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")

    files
    {
        dirs.thirdparty .. "/DirectXTex/DirectXTex/BC.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/BC4BC5.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/BC6HBC7.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexCompress.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexCompressGPU.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexConvert.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexDDS.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexFlipRotate.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexHDR.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexImage.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexMipmaps.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexMisc.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexNormalMaps.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexPMAlpha.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexResize.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexTGA.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexUtil.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/DirectXTexWIC.cpp",
        dirs.thirdparty .. "/DirectXTex/DirectXTex/*.h",
    }

    includedirs
    {
        dirs.thirdparty .. "/DirectXTex/DirectXTex"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/openmp" }

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