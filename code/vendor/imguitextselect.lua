return {
    include = function()
        includedirs { "../vendor/imguitextselect/" }
    end,

    run = function()
        language "C++"
        cppdialect "C++20"
        kind "StaticLib"

        includedirs {
            "../vendor/imgui/",
            "../vendor/utfcpp/source/",
        }

        files_project "../vendor/imguitextselect/" {
            "textselect.hpp",
            "textselect.cpp",
        }
    end
}
