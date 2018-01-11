#!lua

function os.winSdkVersion()
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
end

solution "Vulkan2D"
	configurations { "Debug", "Release" }
	
	platforms { "x86", "x64" }

    filter {"system:windows", "action:vs*"}
		systemversion(os.winSdkVersion() .. ".0")

	project "Engine"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"

		defines { "GLEW_STATIC" }
		files { "Source/Core/**.h", "Source/Core/**.cpp", "glew.c",
				"shaders/*",
				--Should be cleaned up in the future to be a separate project
				"Examples/**.h", "Examples/**.cpp" }
		buildlog "build/build.log"
		includedirs { "include", "Source" }
		objdir "build"
		targetdir "build"

		local libdirectory = "installed/%{cfg.platform}-%{cfg.system}/"

		links { "lua", "opengl32", "vulkan-1" }

		filter "Debug"
			libdirs { libdirectory .. "debug/lib" }
			links { "SDL2d",  "BulletDynamics_Debug", "BulletCollision_Debug", "LinearMath_Debug", "rttr_core_d" }
			symbols "On"

		filter "Release"
			libdirs { libdirectory .. "lib" }
			links { "SDL2", "BulletDynamics", "BulletCollision", "LinearMath", "rttr_core" }

		--CUSTOM BUILD COMMANDS--
		filter { 'files:**.vert' }
			buildmessage 'Compiling %{file.relpath} to SPIR-V'
			buildcommands {
				path.translate('glslc -o "%{file.relpath}.spv" "%{file.relpath}"'),
				'xxd -i "%{file.relpath}.spv" > "Source/Core/Rendering/Shaders/%{file.name}.spv.h"'
			}
			buildoutputs {
				'%{file.relpath}.spv',
				'Source/Core/Rendering/Shaders/%{file.name}.spv.h'
			}
		
		filter { 'files:**.frag' }
			buildmessage 'Compiling %{file.relpath} to SPIR-V'
			buildcommands {
				path.translate('glslc -o "%{file.relpath}.spv" "%{file.relpath}"'),
				'xxd -i "%{file.relpath}.spv" > "Source/Core/Rendering/Shaders/%{file.name}.spv.h"'
			}
			buildoutputs {
				'%{file.relpath}.spv',
				'Source/Core/Rendering/Shaders/%{file.name}.spv.h'
			}
