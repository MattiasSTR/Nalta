project "Engine"
    kind "StaticLib"
    language "C++"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")

    files { "src/**.cpp", "include/**.h" }
    includedirs { "include", ThirdPartyIncludes.spdlog }

    pchheader "npch.h"
    pchsource "src/npch.cpp"

    apply_common_settings()