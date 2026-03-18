-- ----------------------------
-- Global directories
-- ----------------------------
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

dirs = {}
dirs.root       = os.realpath("../") -- go up one folder if premake is in a subfolder
dirs.build      = dirs.root .. "/build" -- all generated files
dirs.bin        = dirs.build .. "/bin"  -- final binaries
dirs.obj        = dirs.build .. "/obj"  -- intermediate objects
dirs.projects   = dirs.build .. "/projects" -- vcxproj locations
dirs.thirdparty = dirs.root .. "/ThirdParty"   -- Third-party libraries

ThirdPartyIncludes = {}
ThirdPartyIncludes.spdlog = dirs.thirdparty .. "/spdlog/include"
ThirdPartyIncludes.glfw = dirs.thirdparty .. "/glfw/include"

-- Ensure directories exist
os.mkdir(dirs.build)
os.mkdir(dirs.bin)
os.mkdir(dirs.obj)
os.mkdir(dirs.projects)

--==================================================
-- Helper function for project creation
--==================================================
function apply_common_settings()

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
        defines { "N_DEBUG", "N_ENABLE_ASSERTS", "GLFW_INCLUDE_NONE" }
        editandcontinue "on"
        warnings "extra"
        fatalwarnings { "All" }

    filter "configurations:Development"
        runtime "Release"
        symbols "on"
        optimize "speed"
        defines { "N_DEVELOPMENT", "N_ENABLE_ASSERTS", "GLFW_INCLUDE_NONE" }
        warnings "extra"
        fatalwarnings { "All" }

    filter "configurations:Shipping"
        runtime "Release"
        symbols "off"
        optimize "full"
        defines { "N_SHIPPING", "GLFW_INCLUDE_NONE" }
        warnings "extra"
        linktimeoptimization "Fast"

    filter {}
end