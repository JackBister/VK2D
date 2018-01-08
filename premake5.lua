#!lua

function os.winSdkVersion()
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
end

solution "Vulkan2D"
	configurations { "Debug", "Release" }
	
	platforms { "x86", "x86_64" }

    filter {"system:windows", "action:vs*"}
		systemversion(os.winSdkVersion() .. ".0")

	project "Engine"
		kind "ConsoleApp"
		language "C++"
		defines { "GLEW_STATIC" }
		dependson { "HeaderGenerator" }
		files { "Source/Core/**.h", "Source/Core/**.cpp", "glew.c",
				"shaders/*",
				--Should be cleaned up in the future to be a separate project
				"Examples/**.h", "Examples/**.cpp" }
		includedirs { "include", "build/include", "build/include/Source", "Source" }
		objdir "build"
		removefiles { "Source/Core/Rendering/render.cpp", "Source/Core/Rendering/render_opengl.cpp", "Source/Core/Rendering/render_vulkan.cpp" }
		targetdir "build"
		
		--PLATFORM CONFIGURATION--
		configuration "windows"
			links { "opengl32" }
		
		--BUILD SYSTEM CONFIGURATION--
		configuration "gmake"
			buildoptions { "-std=c++17" }
			linkoptions { "libs/SDL2main.lib", "libs/SDL2.lib", "libs/liblua.lib", "libs/vulkan-1.lib", "libs/libBulletDynamics.a", "libs/libBulletCollision.a",
						  "libs/libLinearMath.a"
			}
			
		configuration { "vs*", "Debug" }
			buildoptions { "/std:c++latest" }
			links { "libs/VS/SDL2main.lib", "libs/VS/SDL2.lib", "libs/VS/liblua.lib", "libs/VS/vulkan-1.lib", "libs/VS/libBulletDynamics.lib", "libs/VS/libBulletCollision.lib",
					"libs/VS/libLinearMath.lib", "libs/VS/librttr_core_s_d.lib"
			}
		
		configuration { "vs*", "Release" }
		
		--TARGET CONFIGURATION--
		configuration "Debug"
			debugdir "."
			symbols "On"

		filter { 'files:**.vert', 'action:vs2017' }
			buildmessage 'Compiling %{file.relpath} to SPIR-V'
			buildcommands {
				'@echo on',
				path.translate('glslc -o "%{file.relpath}.spv" "%{file.relpath}"'),
				'xxd -i "%{file.relpath}.spv" > "Source/Core/Rendering/Shaders/%{file.name}.spv.h"'
			}
			buildoutputs {
				'%{file.relpath}.spv',
				'Source/Core/Rendering/Shaders/%{file.name}.spv.h'
			}
		
		filter { 'files:**.frag', 'action:vs2017' }
			buildmessage 'Compiling %{file.relpath} to SPIR-V'
			buildcommands {
				'@echo on',
				path.translate('glslc -o "%{file.relpath}.spv" "%{file.relpath}"'),
				'xxd -i "%{file.relpath}.spv" > "Source/Core/Rendering/Shaders/%{file.name}.spv.h"'
			}
			buildoutputs {
				'%{file.relpath}.spv',
				'Source/Core/Rendering/Shaders/%{file.name}.spv.h'
			}
