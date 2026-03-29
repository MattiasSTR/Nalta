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
dirs.binaries   = dirs.root .. "/Binaries"

local root_dir_escaped = dirs.root:gsub("\\", "/"):gsub("/$", "")

ThirdPartyIncludes = {}
ThirdPartyIncludes.spdlog = dirs.thirdparty .. "/spdlog/include"
ThirdPartyIncludes.hlslpp = dirs.thirdparty .. "/hlslpp/include"
ThirdPartyIncludes.nlohmann = dirs.thirdparty .. "/nlohmann/include"

-- Ensure directories exist
os.mkdir(dirs.build)
os.mkdir(dirs.bin)
os.mkdir(dirs.obj)
os.mkdir(dirs.projects)

function copy_binaries()
    local dlls = os.matchfiles(dirs.binaries .. "/*.dll")
    for _, dll in ipairs(dlls) do
        local src = path.getabsolute(dll)
        postbuildcommands {
            "{COPYFILE} \"" .. src .. "\" \"%{cfg.targetdir}/" .. path.getname(dll) .. "\""
        }
    end
end

--==================================================
-- Helper function for project creation
--==================================================
function apply_common_settings()

    defines {
        "N_ROOT_DIR=\"" .. root_dir_escaped .. "\""
    }

    includedirs {
        ThirdPartyIncludes.hlslpp
    }

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
        defines { "N_DEBUG", "N_ENABLE_ASSERTS" }
        editandcontinue "on"
        warnings "extra"
        fatalwarnings { "All" }

    filter "configurations:Development"
        runtime "Release"
        symbols "on"
        optimize "speed"
        defines { "N_DEVELOPMENT", "N_ENABLE_ASSERTS" }
        warnings "extra"
        fatalwarnings { "All" }

    filter "configurations:Shipping"
        runtime "Release"
        symbols "off"
        optimize "full"
        defines { "N_SHIPPING" }
        warnings "extra"
        linktimeoptimization "Fast"

    filter {}
end