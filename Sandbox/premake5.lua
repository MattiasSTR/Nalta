project "Sandbox"
    kind "ConsoleApp"
    language "C++"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")
    
    files {
        "**.cpp"
    }

    includedirs {
        "../Engine/include"
    }

    links {
        "Engine"
    }

    copy_binaries()
    apply_common_settings()