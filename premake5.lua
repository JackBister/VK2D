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

	local platformdirectory = "installed/%{cfg.platform}-%{cfg.system}/"
	local staticPlatformDirectory = "installed/%{cfg.platform}-%{cfg.system}-static/"

	project "Engine"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"

		defines { "GLEW_STATIC" }
		exceptionhandling "Off"

		files { "Source/Core/**.h", "Source/Core/**.cpp",
				"shaders/*",
				--Should be cleaned up in the future to be a separate project
				"Examples/**.h", "Examples/**.cpp" }
		buildlog "build/build.log"
		debugargs { "main.scene" }
		debugdir "Examples/FlappyPong"
		implibdir "build"
		implibname "Engine"
		implibextension ".lib"
		includedirs { "Source" }
		objdir "build"
		targetdir "build"

		links { "opengl32", "vulkan-1" }

		sysincludedirs { "include",  staticPlatformDirectory .. "include", platformdirectory .. "include" }

		filter "Debug"
			libdirs { staticPlatformDirectory .. "debug/lib", platformdirectory .. "debug/lib" }
			links { "glew32d", "SDL2d",  "BulletDynamics_Debug", "BulletCollision_Debug", "LinearMath_Debug" }
			symbols "On"

		filter "Release"
			libdirs { staticPlatformDirectory .. "lib", platformdirectory .. "lib" }
			links { "glew32", "SDL2", "BulletDynamics", "BulletCollision", "LinearMath" }

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


	project "FlappyPong"
		dependson "Engine"
		--configurations { "Debug DLL", "Release DLL", "Debug Static", "Release Static" }
		language "C++"
		cppdialect "C++17"

		exceptionhandling "Off"

		files { "Examples/FlappyPong/Source/**.h", "Examples/FlappyPong/Source/**.cpp" }
		buildlog "Examples/FlappyPong/build/build.log"
		includedirs { "Examples/FlappyPong/Source" }
		libdirs "build"
		objdir "Examples/FlappyPong/build"
		targetdir "Examples/FlappyPong/build"

		sysincludedirs { "Source", "include",  staticPlatformDirectory .. "include", platformdirectory .. "include" }

		filter "configurations:Debug"
			kind "SharedLib"
			symbols "On"
		filter "configurations:Release"
			kind "SharedLib"

--[[
		filter "configurations:Debug Static"
			kind "StaticLib"
			symbols "On"
		filter "configurations:Release Static"
			kind "StaticLib"

		filter "kind:StaticLib"
			defines { "VK2D_LIB" }
]]
		filter "kind:SharedLib"
			defines { "VK2D_DLL" }
			links { "Engine.lib" }

