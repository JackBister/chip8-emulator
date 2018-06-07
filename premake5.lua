function os.winSdkVersion()
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
end

workspace "Emulator"
    configurations { "Debug", "Release" }
    platforms { "x86", "x64" }

    filter {"system:windows", "action:vs*"}
		systemversion(os.winSdkVersion() .. ".0")

    local platformdirectory = "installed/%{cfg.platform}-%{cfg.system}/"
    local staticPlatformDirectory = "installed/%{cfg.platform}-%{cfg.system}-static/"

project "Core"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    objdir "build"
    targetdir "build/%{cfg.buildcfg}"
    sysincludedirs { "include",  staticPlatformDirectory .. "include", platformdirectory .. "include" }
    includedirs { "src" }

    filter "Debug"
        libdirs { staticPlatformDirectory .. "debug/lib", platformdirectory .. "debug/lib" }
        links { "SDL2d" }
        optimize "Off"
        symbols "On"

    filter "Release"
        libdirs { staticPlatformDirectory .. "lib", platformdirectory .. "lib" }
        links { "SDL2" }
        optimize "Full"

    files { "src/**.hh", "src/**.cc"}
