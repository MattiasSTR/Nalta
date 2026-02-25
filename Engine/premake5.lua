project "Engine"
    kind "StaticLib"
    language "C++"
    staticruntime "off"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")

    pchheader "npch.h"
    pchsource "src/npch.cpp"

    files { 
        "src/**.cpp", 
        "include/**.h" 
    }

    defines{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

    includedirs { 
        "include", 
        ThirdPartyIncludes.spdlog, 
        ThirdPartyIncludes.glfw 
    }

    links { 
        "GLFW"
    }

    

    apply_common_settings()