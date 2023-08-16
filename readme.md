# VK2D

This is still heavily in development so some commits will be stupid, commit history may be rewritten and there is no API stability at all.

## Requirements

The engine uses C++17 features such as std::filesystem and std::variant, and therefore requires a fairly recent compiler.

The following libraries are currently used by the engine:

- [assimp](https://github.com/assimp/assimp)
- [Bullet3](https://github.com/bulletphysics/bullet3)
- [GLEW](http://glew.sourceforge.net/)
- [GLM](https://github.com/g-truc/glm)
- [imgui](https://github.com/ocornut/imgui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue)
- [nlohmann::json](https://github.com/nlohmann/json)
- [SDL](https://www.libsdl.org/index.php)
- [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB)
- [shaderc](https://github.com/google/shaderc)
- [stb](https://github.com/nothings/stb)
- [tinyobjloader](https://github.com/syoyo/tinyobjloader)

These are all clonde into the `Source/ThirdParty` directory using git submodules and then built together with the engine. This means that the first build after cloning will be slow because all these libraries need to be built.

When it comes to graphics APIs, the engine currently targets:

- Vulkan 1.0
- OpenGL 4.6 - SPIR-V support was standardized in 4.6, and the engine only supports SPIR-V shaders as of now.

You must have the following installed in order to build:

- CMake >= 3.16
- Python >= 3.11

## Build instructions

1. Clone the project: `git clone https://github.com/JackBister/VK2D.git`
2. Download dependencies: `git submodule update --init`
3. Enter the directory: `cd VK2D`
4. Create the build directory: `mkdir build`
5. Enter the build directory: `cd build`
6. Run CMake to generate project files, in this case using Visual Studio: `cmake -G "Visual Studio 17 2022" -A x64`
7. Run the build: `msbuild ALL_BUILD.vcxproj`
