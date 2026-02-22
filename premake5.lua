workspace "Nalta"
    architecture "x64"
    startproject "Sandbox"
    location "."

    configurations { "Debug", "Development", "Shipping" }
    cppdialect "C++20"
    staticruntime "off"

    if _OPTIONS["clang"] then
        toolset "clang"  -- uses clang-cl on Windows
        print("Using Clang toolset")
    else
        toolset "msc"    -- default MSVC
        print("Using MSVC toolset")
    end

-- ----------------------------
-- Global directories
-- ----------------------------
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

dirs = {}
dirs.root       = os.getcwd()           -- project root
dirs.build      = dirs.root .. "/build" -- all generated files
dirs.bin        = dirs.build .. "/bin"  -- final binaries
dirs.obj        = dirs.build .. "/obj"  -- intermediate objects
dirs.projects   = dirs.build .. "/projects" -- vcxproj locations
dirs.thirdparty = dirs.root .. "/ThirdParty"   -- Third-party libraries

-- Ensure directories exist
os.mkdir(dirs.build)
os.mkdir(dirs.bin)
os.mkdir(dirs.obj)
os.mkdir(dirs.projects)

--==================================================
-- Helper function for project creation
--==================================================
function setup_project(name, projKind, src, include_dirs)
    project(name)
        kind(projKind)  -- use parameter projKind, avoids collision
        language "C++"
        location(dirs.projects .. "/" .. name)
        targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
        objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")
        files(src)
        includedirs(include_dirs)

    -- Apply filters **per project**
    filter "system:windows"
        conformancemode "On"
        callingconvention "fastcall"
        rtti "Off"
        floatingpoint "Fast"
        multiprocessorcompile "On"
        characterset "Unicode"
        buildoptions { "/utf-8" }
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        optimize "off"
        defines { "_DEBUG" }
        editandcontinue "on"
        warnings "extra"
        fatalwarnings { "All" }
    
    filter "configurations:Development"
        runtime "Release"
        symbols "on"
        optimize "speed"
        defines { "_DEVELOPMENT" }
        warnings "extra"
        fatalwarnings { "All" }
    
    filter "configurations:Shipping"
        runtime "Release"
        symbols "off"
        optimize "full"
        defines { "_SHIPPING", "NDEBUG" }
        warnings "extra"
        linktimeoptimization "Fast"
    
    -- Reset filter
    filter {}
end

--==================================================
-- Projects
--==================================================

-- Engine Static Library
setup_project("Engine", "StaticLib",
    { "Engine/source/**.cpp", "Engine/include/**.h" },
    { "Engine/include", dirs.thirdparty }
)

-- Sandbox Executable
setup_project("Sandbox", "ConsoleApp",
    { "Sandbox/**.cpp" },
    { "Engine/include" }
)
links { "Engine" }

-- Tests Executable
setup_project("Tests", "ConsoleApp",
    { "Tests/**.cpp" },
    { "Engine/include" }
)
links { "Engine" }