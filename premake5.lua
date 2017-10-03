#!lua

solution "Vulkan2D"
	configurations { "Debug", "Release" }
	
	platforms { "x86", "x86_64" }
		
	project "HeaderGenerator"
		kind "ConsoleApp"
		language "C++"
		defines { "BYTE_ORDER=1", "LLVM_ON_WIN32=1" }
		files { "Source/Tools/HeaderGenerator/**.h", "Source/Tools/HeaderGenerator/**.cpp"}
		flags { "C++14" }		
		includedirs { "include" }
		objdir "build"
		targetdir "build"
		
		--This is broken for some reason
		--links { "libs/libboost_filesystem.a", "libs/libboost_system.a", "libs/liblibclang.dll.a" }
		
		--BUILD SYSTEM CONFIGURATION--
		configuration "gmake"
			linkoptions { "libs/libboost_filesystem.a", "libs/libboost_system.a", "libs/liblibclang.dll.a" }
			
		configuration "vs*"
			--For some reason refuses to build if I don't keep the exact original filenames
			links {
			"libs/VS/Release/libboost_filesystem-vc140-mt-1_61.lib",
			"libs/VS/Release/libboost_filesystem.lib",
			"libs/VS/Release/libboost_system-vc140-mt-1_61.lib",
			"libs/VS/Release/libboost_system.lib",
			"libs/VS/libclang.lib" }
	
	project "Engine"
		kind "ConsoleApp"
		language "C++"
		defines { "GLEW_STATIC" }
		dependson { "HeaderGenerator" }
		files { "Source/Core/**.h", "Source/Core/**.cpp", "glew.c",
				--Should be cleaned up in the future to be a separate project
				"Examples/**.h", "Examples/**.cpp" }
		--flags { "C++14" }
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
						  "libs/libLinearMath.a", "libs/libboost_filesystem.lib", "libs/libboost_system.lib" }
			
		configuration { "vs*", "Debug" }
			buildoptions { "/std:c++latest" }
			links { "libs/VS/SDL2main.lib", "libs/VS/SDL2.lib", "libs/VS/liblua.lib", "libs/VS/vulkan-1.lib", "libs/VS/libBulletDynamics.lib", "libs/VS/libBulletCollision.lib",
					"libs/VS/libLinearMath.lib", "libs/VS/libboost_filesystem-vc140-mt-gd-1_61.lib", "libs/VS/libboost_filesystem.lib", "libs/VS/libboost_system-vc140-mt-gd-1_61.lib", "libs/VS/libboost_system.lib" }
		
		configuration { "vs*", "Release" }
		
		--TARGET CONFIGURATION--
		configuration "Debug"
			debugdir "."
			symbols "On"

			--[[
		--HEADERGENERATOR PREPROCESS--
		if _ACTION == 'vs2017' then
			filter 'files:**.h'
				buildcommands {
					path.translate('build/HeaderGenerator.exe build/include "%{file.relpath}"')
				}
				buildoutputs {
					'build/include/' .. '%{file.relpath}.generated.h'
				}
		else
			filter 'files:**.h'
				buildcommands {
					'build/HeaderGenerator build/include "%{file.relpath}"'
				}
				buildoutputs {
					'build/include/' .. '%{file.relpath}.generated.h'
				}
			
		end
		]]--