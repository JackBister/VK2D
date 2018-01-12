# VK2D
This is still heavily in development so some commits will be stupid, commit history may be rewritten and there is no API stability at all.

## Requirements
The engine uses C++17 features such as std::filesystem and std::variant, and therefore requires a fairly recent compiler.

The following libraries are currently used by the engine:

* [Bullet3 2.87](https://github.com/bulletphysics/bullet3)
* [GLEW 2.1.0](http://glew.sourceforge.net/)
* [GLM](https://github.com/g-truc/glm)
* [Lua 5.3](https://www.lua.org/)
* [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue)
* [nlohmann::json](https://github.com/nlohmann/json)
* [SDL 2.0.7](https://www.libsdl.org/index.php)
* [rttr 0.9.5](https://github.com/rttrorg/rttr)

During a pre-build step, GLSL shaders are compiled to SPIR-V and transformed into headers included directly by the engine. For this compilation step, the following are used:

* [glslc](https://github.com/google/shaderc/tree/master/glslc)
* xxd - unfortunately I have not been able to get a WSL version of this to work on Windows with Visual Studio, so it needs to be one compiled for Windows if you're building in VS.

When it comes to graphics APIs, the engine currently targets:

* Vulkan 1.0
* OpenGL 4.6 - SPIR-V support was standardized in 4.6, and the engine only supports SPIR-V shaders as of now.

## Installing the required libraries on Windows
On Windows, I have been using [vcpkg](https://github.com/Microsoft/vcpkg) to install all the required libraries and then copying the "installed" folder from the vcpkg root directory into the root directory of the engine. I never got the "vcpkg integrate" thing working and it feels like a weird thing to rely on either way.

One thing to note if using vcpkg is that currently the SDL package specifically disables Vulkan support, so you need to manually edit the ports file.

If you don't want to use vcpkg, it's still possible to build by copying the .lib files into the folder structure used by vcpkg.

## Building
A version of [Premake](https://github.com/premake/premake-core) newer than Premake5-alpha12 is required to build on Windows. On Linux/Mac, older versions may suffice.

### Windows
Building is as simple as running `.\premake5.exe vs2017`, opening the generated .sln file and pressing "Build Solution".
