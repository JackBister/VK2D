# VK2D
This is still heavily in development so some commits will be stupid, commit history may be rewritten and there is no API stability at all.

## Requirements
The engine uses C++17 features such as std::filesystem and std::variant, and therefore requires a fairly recent compiler.

The following libraries are currently used by the engine:

* [Bullet3](https://github.com/bulletphysics/bullet3)
* [GLEW](http://glew.sourceforge.net/)
* [GLM](https://github.com/g-truc/glm)
* [imgui](https://github.com/ocornut/imgui)
* [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue)
* [nlohmann::json](https://github.com/nlohmann/json)
* [SDL](https://www.libsdl.org/index.php)
* [stb](https://github.com/nothings/stb)

During a pre-build step, GLSL shaders are compiled to SPIR-V and transformed into headers included directly by the engine. For this compilation step, the following are used:

* [glslc](https://github.com/google/shaderc/tree/master/glslc)
* xxd - unfortunately I have not been able to get a WSL version of this to work on Windows with Visual Studio, so it needs to be one compiled for Windows if you're building in VS.

When it comes to graphics APIs, the engine currently targets:

* Vulkan 1.0
* OpenGL 4.6 - SPIR-V support was standardized in 4.6, and the engine only supports SPIR-V shaders as of now.

## Installing the required libraries on Windows
On Windows, I have been using [vcpkg](https://github.com/Microsoft/vcpkg) to install all the required libraries. The following command line should give you everything you need:
```
vcpkg install --triplet=x64-windows bullet3 concurrentqueue glew glm imgui nlohmann-json sdl2 sdl2[vulkan] stb
```

One thing to note if using vcpkg is that currently the SDL package specifically disables Vulkan support, so you need to manually edit the ports file.
