workspace "buildx"
    configurations { "debug", "release" }

project "bx"
    kind "ConsoleApp"
    language "C"
    cdialect "C11"

    files {
        "src/**.c"
    }

    includedirs {
        "src"
    }

    filter "action:gmake2"
        buildoptions {
            "-Wpedantic",
            "-Wall",
            "-Wextra",
            "-Werror"
        }

    filter "configurations:debug"
        defines { "DEBUG" }
        targetdir "bin/debug"
        symbols "On"
        optimize "Debug"
    
    filter "configurations:release"
        targetdir "bin/release"
        optimize "Full"

