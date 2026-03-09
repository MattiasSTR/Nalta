project "GLFW"
    kind "StaticLib"
    language "C"

    location(dirs.projects .. "/%{prj.name}")
    targetdir(dirs.bin .. "/%{outputdir}/%{prj.name}")
    objdir(dirs.obj .. "/%{outputdir}/%{prj.name}")

    local glfw_dir = dirs.thirdparty .. "/glfw"
    
    files {
        glfw_dir .. "/include/GLFW/glfw3.h",
        glfw_dir .. "/include/GLFW/glfw3native.h",
        glfw_dir .. "/src/internal.h",
        glfw_dir .. "/src/platform.h",
        glfw_dir .. "/src/mappings.h",
        glfw_dir .. "/src/context.c",
        glfw_dir .. "/src/init.c",
        glfw_dir .. "/src/input.c",
        glfw_dir .. "/src/monitor.c",
        glfw_dir .. "/src/platform.c",
        glfw_dir .. "/src/vulkan.c",
        glfw_dir .. "/src/window.c",
        glfw_dir .. "/src/egl_context.c",
        glfw_dir .. "/src/osmesa_context.c",
        glfw_dir .. "/src/null_platform.h",
        glfw_dir .. "/src/null_joystick.h",
        glfw_dir .. "/src/null_init.c",
        glfw_dir .. "/src/null_monitor.c",
        glfw_dir .. "/src/null_window.c",
        glfw_dir .. "/src/null_joystick.c",
    }
    
    filter "system:linux"
        pic "On"
        systemversion "latest"
        staticruntime "On"
    
        files {
            glfw_dir .. "/src/x11_init.c",
            glfw_dir .. "/src/x11_monitor.c",
            glfw_dir .. "/src/x11_window.c",
            glfw_dir .. "/src/xkb_unicode.c",
            glfw_dir .. "/src/posix_time.c",
            glfw_dir .. "/src/posix_thread.c",
            glfw_dir .. "/src/glx_context.c",
            glfw_dir .. "/src/egl_context.c",
            glfw_dir .. "/src/osmesa_context.c",
            glfw_dir .. "/src/linux_joystick.c",
        }
    
        defines { "_GLFW_X11" }
    
    filter "system:windows"
        systemversion "latest"
        staticruntime "On"
    
        files {
            glfw_dir .. "/src/win32_init.c",
            glfw_dir .. "/src/win32_module.c",
            glfw_dir .. "/src/win32_joystick.c",
            glfw_dir .. "/src/win32_monitor.c",
            glfw_dir .. "/src/win32_time.h",
            glfw_dir .. "/src/win32_time.c",
            glfw_dir .. "/src/win32_thread.h",
            glfw_dir .. "/src/win32_thread.c",
            glfw_dir .. "/src/win32_window.c",
            glfw_dir .. "/src/wgl_context.c",
            glfw_dir .. "/src/egl_context.c",
            glfw_dir .. "/src/osmesa_context.c",
        }
    
        defines {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
    
    filter "configurations:Release"
        runtime "Release"
        optimize "On"

filter {}